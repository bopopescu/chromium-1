// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/sync_manager_impl.h"

#include <stddef.h>

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/metrics/histogram_macros.h"
#include "base/observer_list.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/values.h"
#include "components/sync/base/cancelation_signal.h"
#include "components/sync/base/experiments.h"
#include "components/sync/base/invalidation_interface.h"
#include "components/sync/base/model_type.h"
#include "components/sync/base/nigori.h"
#include "components/sync/engine/configure_reason.h"
#include "components/sync/engine/engine_components_factory.h"
#include "components/sync/engine/engine_util.h"
#include "components/sync/engine/net/http_post_provider_factory.h"
#include "components/sync/engine/polling_constants.h"
#include "components/sync/engine_impl/cycle/directory_type_debug_info_emitter.h"
#include "components/sync/engine_impl/loopback_server/loopback_connection_manager.h"
#include "components/sync/engine_impl/model_type_connector_proxy.h"
#include "components/sync/engine_impl/net/sync_server_connection_manager.h"
#include "components/sync/engine_impl/sync_scheduler.h"
#include "components/sync/engine_impl/syncer_types.h"
#include "components/sync/engine_impl/uss_migrator.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync/syncable/base_node.h"
#include "components/sync/syncable/directory.h"
#include "components/sync/syncable/directory_backing_store.h"
#include "components/sync/syncable/entry.h"
#include "components/sync/syncable/read_node.h"
#include "components/sync/syncable/read_transaction.h"
#include "components/sync/syncable/write_node.h"
#include "components/sync/syncable/write_transaction.h"

namespace syncer {

using syncable::ImmutableWriteTransactionInfo;
using syncable::SPECIFICS;
using syncable::UNIQUE_POSITION;

namespace {

sync_pb::SyncEnums::GetUpdatesOrigin GetOriginFromReason(
    ConfigureReason reason) {
  switch (reason) {
    case CONFIGURE_REASON_RECONFIGURATION:
      return sync_pb::SyncEnums::RECONFIGURATION;
    case CONFIGURE_REASON_MIGRATION:
      return sync_pb::SyncEnums::MIGRATION;
    case CONFIGURE_REASON_NEW_CLIENT:
      return sync_pb::SyncEnums::NEW_CLIENT;
    case CONFIGURE_REASON_NEWLY_ENABLED_DATA_TYPE:
    case CONFIGURE_REASON_CRYPTO:
    case CONFIGURE_REASON_CATCH_UP:
      return sync_pb::SyncEnums::NEWLY_SUPPORTED_DATATYPE;
    case CONFIGURE_REASON_PROGRAMMATIC:
      return sync_pb::SyncEnums::PROGRAMMATIC;
    case CONFIGURE_REASON_UNKNOWN:
      NOTREACHED();
  }
  return sync_pb::SyncEnums::UNKNOWN_ORIGIN;
}

}  // namespace

SyncManagerImpl::SyncManagerImpl(
    const std::string& name,
    network::NetworkConnectionTracker* network_connection_tracker)
    : name_(name),
      network_connection_tracker_(network_connection_tracker),
      change_delegate_(nullptr),
      initialized_(false),
      observing_network_connectivity_changes_(false),
      weak_ptr_factory_(this) {
  // Pre-fill |notification_info_map_|.
  for (int i = FIRST_REAL_MODEL_TYPE; i < MODEL_TYPE_COUNT; ++i) {
    notification_info_map_.insert(
        std::make_pair(ModelTypeFromInt(i), NotificationInfo()));
  }
}

SyncManagerImpl::~SyncManagerImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!initialized_);
}

SyncManagerImpl::NotificationInfo::NotificationInfo() : total_count(0) {}
SyncManagerImpl::NotificationInfo::~NotificationInfo() {}

base::DictionaryValue* SyncManagerImpl::NotificationInfo::ToValue() const {
  base::DictionaryValue* value = new base::DictionaryValue();
  value->SetInteger("totalCount", total_count);
  value->SetString("payload", payload);
  return value;
}

bool SyncManagerImpl::VisiblePositionsDiffer(
    const syncable::EntryKernelMutation& mutation) const {
  const syncable::EntryKernel& a = mutation.original;
  const syncable::EntryKernel& b = mutation.mutated;
  if (!b.ShouldMaintainPosition())
    return false;
  if (!a.ref(UNIQUE_POSITION).Equals(b.ref(UNIQUE_POSITION)))
    return true;
  if (a.ref(syncable::PARENT_ID) != b.ref(syncable::PARENT_ID))
    return true;
  return false;
}

bool SyncManagerImpl::VisiblePropertiesDiffer(
    const syncable::EntryKernelMutation& mutation,
    Cryptographer* cryptographer) const {
  const syncable::EntryKernel& a = mutation.original;
  const syncable::EntryKernel& b = mutation.mutated;
  const sync_pb::EntitySpecifics& a_specifics = a.ref(SPECIFICS);
  const sync_pb::EntitySpecifics& b_specifics = b.ref(SPECIFICS);
  DCHECK_EQ(GetModelTypeFromSpecifics(a_specifics),
            GetModelTypeFromSpecifics(b_specifics));
  ModelType model_type = GetModelTypeFromSpecifics(b_specifics);
  // Suppress updates to items that aren't tracked by any browser model.
  if (model_type < FIRST_REAL_MODEL_TYPE ||
      !a.ref(syncable::UNIQUE_SERVER_TAG).empty()) {
    return false;
  }
  if (a.ref(syncable::IS_DIR) != b.ref(syncable::IS_DIR))
    return true;
  if (!AreSpecificsEqual(cryptographer, a.ref(syncable::SPECIFICS),
                         b.ref(syncable::SPECIFICS))) {
    return true;
  }
  // We only care if the name has changed if neither specifics is encrypted
  // (encrypted nodes blow away the NON_UNIQUE_NAME).
  if (!a_specifics.has_encrypted() && !b_specifics.has_encrypted() &&
      a.ref(syncable::NON_UNIQUE_NAME) != b.ref(syncable::NON_UNIQUE_NAME))
    return true;
  if (VisiblePositionsDiffer(mutation))
    return true;
  return false;
}

ModelTypeSet SyncManagerImpl::InitialSyncEndedTypes() {
  DCHECK(initialized_);
  return model_type_registry_->GetInitialSyncEndedTypes();
}

ModelTypeSet SyncManagerImpl::GetTypesWithEmptyProgressMarkerToken(
    ModelTypeSet types) {
  ModelTypeSet result;
  for (ModelType type : types) {
    sync_pb::DataTypeProgressMarker marker;
    directory()->GetDownloadProgress(type, &marker);

    if (marker.token().empty())
      result.Put(type);
  }
  return result;
}

void SyncManagerImpl::ConfigureSyncer(ConfigureReason reason,
                                      ModelTypeSet to_download,
                                      SyncFeatureState sync_feature_state,
                                      const base::Closure& ready_task) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!ready_task.is_null());
  DCHECK(initialized_);

  // Don't download non-blocking types that have already completed initial sync.
  to_download.RemoveAll(
      model_type_registry_->GetInitialSyncDoneNonBlockingTypes());

  DVLOG(1) << "Configuring -"
           << "\n\t"
           << "types to download: " << ModelTypeSetToString(to_download);
  ConfigurationParams params(GetOriginFromReason(reason), to_download,
                             ready_task);

  scheduler_->Start(SyncScheduler::CONFIGURATION_MODE, base::Time());
  scheduler_->ScheduleConfiguration(params);
  if (sync_feature_state != SyncFeatureState::INITIALIZING) {
    cycle_context_->set_is_sync_feature_enabled(sync_feature_state ==
                                                SyncFeatureState::ON);
  }
}

void SyncManagerImpl::Init(InitArgs* args) {
  DCHECK(!initialized_);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(args->post_factory);
  DCHECK(!args->short_poll_interval.is_zero());
  DCHECK(!args->long_poll_interval.is_zero());
  if (!args->enable_local_sync_backend) {
    DCHECK(!args->credentials.account_id.empty());
  }
  DCHECK(args->cancelation_signal);
  DVLOG(1) << "SyncManager starting Init...";

  weak_handle_this_ = MakeWeakHandle(weak_ptr_factory_.GetWeakPtr());

  change_delegate_ = args->change_delegate;

  AddObserver(&js_sync_manager_observer_);
  SetJsEventHandler(args->event_handler);

  AddObserver(&debug_info_event_listener_);

  database_path_ = args->database_location.Append(
      syncable::Directory::kSyncDatabaseFilename);
  report_unrecoverable_error_function_ =
      args->report_unrecoverable_error_function;

  allstatus_.SetHasKeystoreKey(
      !args->restored_keystore_key_for_bootstrapping.empty());
  sync_encryption_handler_ = std::make_unique<SyncEncryptionHandlerImpl>(
      &share_, args->encryptor, args->restored_key_for_bootstrapping,
      args->restored_keystore_key_for_bootstrapping,
      base::BindRepeating(&Nigori::GenerateScryptSalt));
  sync_encryption_handler_->AddObserver(this);
  sync_encryption_handler_->AddObserver(&debug_info_event_listener_);
  sync_encryption_handler_->AddObserver(&js_sync_encryption_handler_observer_);

  base::FilePath absolute_db_path = database_path_;
  DCHECK(absolute_db_path.IsAbsolute());

  std::unique_ptr<syncable::DirectoryBackingStore> backing_store =
      args->engine_components_factory->BuildDirectoryBackingStore(
          EngineComponentsFactory::STORAGE_ON_DISK,
          args->credentials.account_id, absolute_db_path);

  DCHECK(backing_store);
  share_.directory = std::make_unique<syncable::Directory>(
      std::move(backing_store), args->unrecoverable_error_handler,
      report_unrecoverable_error_function_, sync_encryption_handler_.get(),
      sync_encryption_handler_->GetCryptographerUnsafe());
  share_.sync_credentials = args->credentials;

  // UserShare is accessible to a lot of code that doesn't need access to the
  // sync token so clear sync_token from the UserShare.
  share_.sync_credentials.sync_token = "";

  DVLOG(1) << "Username: " << args->credentials.email;
  DVLOG(1) << "AccountId: " << args->credentials.account_id;
  if (!OpenDirectory(args->credentials.account_id)) {
    NotifyInitializationFailure();
    LOG(ERROR) << "Sync manager initialization failed!";
    return;
  }

  // Now that we have opened the Directory we can restore any previously saved
  // nigori specifics.
  if (args->saved_nigori_state) {
    sync_encryption_handler_->RestoreNigori(*args->saved_nigori_state);
    args->saved_nigori_state.reset();
  }

  if (args->enable_local_sync_backend) {
    VLOG(1) << "Running against local sync backend.";
    allstatus_.SetLocalBackendFolder(
        args->local_sync_backend_folder.AsUTF8Unsafe());
    connection_manager_ = std::make_unique<LoopbackConnectionManager>(
        args->cancelation_signal, args->local_sync_backend_folder);
  } else {
    connection_manager_ = std::make_unique<SyncServerConnectionManager>(
        args->service_url.host() + args->service_url.path(),
        args->service_url.EffectiveIntPort(),
        args->service_url.SchemeIsCryptographic(), args->post_factory.release(),
        args->cancelation_signal);
  }
  connection_manager_->set_client_id(directory()->cache_guid());
  connection_manager_->AddListener(this);

  std::string sync_id = directory()->cache_guid();

  DVLOG(1) << "Setting sync client ID: " << sync_id;
  allstatus_.SetSyncId(sync_id);
  DVLOG(1) << "Setting invalidator client ID: " << args->invalidator_client_id;
  allstatus_.SetInvalidatorClientId(args->invalidator_client_id);

  model_type_registry_ = std::make_unique<ModelTypeRegistry>(
      args->workers, &share_, this, base::Bind(&MigrateDirectoryData),
      args->cancelation_signal);
  sync_encryption_handler_->AddObserver(model_type_registry_.get());

  // Build a SyncCycleContext and store the worker in it.
  DVLOG(1) << "Sync is bringing up SyncCycleContext.";
  std::vector<SyncEngineEventListener*> listeners;
  listeners.push_back(&allstatus_);
  listeners.push_back(this);
  cycle_context_ = args->engine_components_factory->BuildContext(
      connection_manager_.get(), directory(), args->extensions_activity,
      listeners, &debug_info_event_listener_, model_type_registry_.get(),
      args->invalidator_client_id, args->short_poll_interval,
      args->long_poll_interval);
  scheduler_ = args->engine_components_factory->BuildScheduler(
      name_, cycle_context_.get(), args->cancelation_signal,
      args->enable_local_sync_backend);

  scheduler_->Start(SyncScheduler::CONFIGURATION_MODE, base::Time());

  initialized_ = true;

  if (!args->enable_local_sync_backend) {
    network_connection_tracker_->AddNetworkConnectionObserver(this);
    observing_network_connectivity_changes_ = true;

    UpdateCredentials(args->credentials);
  } else {
    scheduler_->OnCredentialsUpdated();
  }

  NotifyInitializationSuccess();
}

void SyncManagerImpl::NotifyInitializationSuccess() {
  for (auto& observer : observers_) {
    observer.OnInitializationComplete(
        MakeWeakHandle(weak_ptr_factory_.GetWeakPtr()),
        MakeWeakHandle(debug_info_event_listener_.GetWeakPtr()), true,
        InitialSyncEndedTypes());
  }
}

void SyncManagerImpl::NotifyInitializationFailure() {
  for (auto& observer : observers_) {
    observer.OnInitializationComplete(
        MakeWeakHandle(weak_ptr_factory_.GetWeakPtr()),
        MakeWeakHandle(debug_info_event_listener_.GetWeakPtr()), false,
        ModelTypeSet());
  }
}

void SyncManagerImpl::OnPassphraseRequired(
    PassphraseRequiredReason reason,
    const KeyDerivationParams& key_derivation_params,
    const sync_pb::EncryptedData& pending_keys) {
  // Does nothing.
}

void SyncManagerImpl::OnPassphraseAccepted() {
  // Does nothing.
}

void SyncManagerImpl::OnBootstrapTokenUpdated(
    const std::string& bootstrap_token,
    BootstrapTokenType type) {
  if (type == KEYSTORE_BOOTSTRAP_TOKEN)
    allstatus_.SetHasKeystoreKey(true);
}

void SyncManagerImpl::OnEncryptedTypesChanged(ModelTypeSet encrypted_types,
                                              bool encrypt_everything) {
  allstatus_.SetEncryptedTypes(encrypted_types);
}

void SyncManagerImpl::OnEncryptionComplete() {
  // Does nothing.
}

void SyncManagerImpl::OnCryptographerStateChanged(
    Cryptographer* cryptographer) {
  allstatus_.SetCryptographerReady(cryptographer->is_ready());
  allstatus_.SetCryptoHasPendingKeys(cryptographer->has_pending_keys());
  allstatus_.SetKeystoreMigrationTime(
      sync_encryption_handler_->migration_time());
}

void SyncManagerImpl::OnPassphraseTypeChanged(
    PassphraseType type,
    base::Time explicit_passphrase_time) {
  allstatus_.SetPassphraseType(type);
  allstatus_.SetKeystoreMigrationTime(
      sync_encryption_handler_->migration_time());
}

void SyncManagerImpl::OnLocalSetPassphraseEncryption(
    const SyncEncryptionHandler::NigoriState& nigori_state) {}

void SyncManagerImpl::StartSyncingNormally(
    base::Time last_poll_time) {
  // Start the sync scheduler.
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  scheduler_->Start(SyncScheduler::NORMAL_MODE, last_poll_time);
}

void SyncManagerImpl::StartConfiguration() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  scheduler_->Start(SyncScheduler::CONFIGURATION_MODE, base::Time());
}

syncable::Directory* SyncManagerImpl::directory() {
  return share_.directory.get();
}

const SyncScheduler* SyncManagerImpl::scheduler() const {
  return scheduler_.get();
}

bool SyncManagerImpl::GetHasInvalidAuthTokenForTest() const {
  return connection_manager_->HasInvalidAuthToken();
}

bool SyncManagerImpl::OpenDirectory(const std::string& username) {
  DCHECK(!initialized_) << "Should only happen once";

  // Set before Open().
  change_observer_ = MakeWeakHandle(js_mutation_event_observer_.AsWeakPtr());
  WeakHandle<syncable::TransactionObserver> transaction_observer(
      MakeWeakHandle(js_mutation_event_observer_.AsWeakPtr()));

  syncable::DirOpenResult open_result = syncable::NOT_INITIALIZED;
  open_result = directory()->Open(username, this, transaction_observer);
  if (open_result != syncable::OPENED) {
    LOG(ERROR) << "Could not open share for:" << username;
    return false;
  }

  // Unapplied datatypes (those that do not have initial sync ended set) get
  // re-downloaded during any configuration. But, it's possible for a datatype
  // to have a progress marker but not have initial sync ended yet, making
  // it a candidate for migration. This is a problem, as the DataTypeManager
  // does not support a migration while it's already in the middle of a
  // configuration. As a result, any partially synced datatype can stall the
  // DTM, waiting for the configuration to complete, which it never will due
  // to the migration error. In addition, a partially synced nigori will
  // trigger the migration logic before the backend is initialized, resulting
  // in crashes. We therefore detect and purge any partially synced types as
  // part of initialization.
  PurgePartiallySyncedTypes();
  return true;
}

void SyncManagerImpl::PurgePartiallySyncedTypes() {
  ModelTypeSet partially_synced_types = ModelTypeSet::All();
  partially_synced_types.RemoveAll(directory()->InitialSyncEndedTypes());
  partially_synced_types.RemoveAll(
      GetTypesWithEmptyProgressMarkerToken(ModelTypeSet::All()));

  DVLOG(1) << "Purging partially synced types "
           << ModelTypeSetToString(partially_synced_types);
  UMA_HISTOGRAM_COUNTS_1M("Sync.PartiallySyncedTypes",
                          partially_synced_types.Size());
  directory()->PurgeEntriesWithTypeIn(partially_synced_types, ModelTypeSet(),
                                      ModelTypeSet());
}

void SyncManagerImpl::PurgeDisabledTypes(ModelTypeSet to_purge,
                                         ModelTypeSet to_journal,
                                         ModelTypeSet to_unapply) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(initialized_);
  DVLOG(1) << "Purging disabled types:\n\t"
           << "types to purge: " << ModelTypeSetToString(to_purge) << "\n\t"
           << "types to journal: " << ModelTypeSetToString(to_journal) << "\n\t"
           << "types to unapply: " << ModelTypeSetToString(to_unapply);
  DCHECK(to_purge.HasAll(to_journal));
  DCHECK(to_purge.HasAll(to_unapply));
  directory()->PurgeEntriesWithTypeIn(to_purge, to_journal, to_unapply);
}

void SyncManagerImpl::UpdateCredentials(const SyncCredentials& credentials) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(initialized_);
  DCHECK(!credentials.account_id.empty());
  cycle_context_->set_account_name(credentials.email);

  observing_network_connectivity_changes_ = true;
  if (!connection_manager_->SetAuthToken(credentials.sync_token))
    return;  // Auth token is known to be invalid, so exit early.

  scheduler_->OnCredentialsUpdated();

  // TODO(zea): pass the credential age to the debug info event listener.
}

void SyncManagerImpl::InvalidateCredentials() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  connection_manager_->SetAuthToken(std::string());
}

void SyncManagerImpl::AddObserver(SyncManager::Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.AddObserver(observer);
}

void SyncManagerImpl::RemoveObserver(SyncManager::Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

void SyncManagerImpl::ShutdownOnSyncThread() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Prevent any in-flight method calls from running.  Also
  // invalidates |weak_handle_this_| and |change_observer_|.
  weak_ptr_factory_.InvalidateWeakPtrs();
  js_mutation_event_observer_.InvalidateWeakPtrs();

  scheduler_.reset();
  cycle_context_.reset();

  if (model_type_registry_)
    sync_encryption_handler_->RemoveObserver(model_type_registry_.get());

  model_type_registry_.reset();

  if (sync_encryption_handler_) {
    sync_encryption_handler_->RemoveObserver(&debug_info_event_listener_);
    sync_encryption_handler_->RemoveObserver(this);
  }

  SetJsEventHandler(WeakHandle<JsEventHandler>());
  RemoveObserver(&js_sync_manager_observer_);

  RemoveObserver(&debug_info_event_listener_);

  // |connection_manager_| may end up being null here in tests (in synchronous
  // initialization mode).
  //
  // TODO(akalin): Fix this behavior.
  if (connection_manager_)
    connection_manager_->RemoveListener(this);
  connection_manager_.reset();

  network_connection_tracker_->RemoveNetworkConnectionObserver(this);
  observing_network_connectivity_changes_ = false;

  if (initialized_ && directory()) {
    directory()->SaveChanges();
  }

  share_.directory.reset();

  change_delegate_ = nullptr;

  initialized_ = false;

  // We reset these here, since only now we know they will not be
  // accessed from other threads (since we shut down everything).
  change_observer_.Reset();
  weak_handle_this_.Reset();
}

void SyncManagerImpl::OnConnectionChanged(network::mojom::ConnectionType type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!observing_network_connectivity_changes_) {
    DVLOG(1) << "Network change dropped.";
    return;
  }
  DVLOG(1) << "Network change detected.";
  scheduler_->OnConnectionStatusChange(type);
}

void SyncManagerImpl::OnServerConnectionEvent(
    const ServerConnectionEvent& event) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (event.connection_code == HttpResponse::SERVER_CONNECTION_OK) {
    for (auto& observer : observers_) {
      observer.OnConnectionStatusChange(CONNECTION_OK);
    }
  }

  if (event.connection_code == HttpResponse::SYNC_AUTH_ERROR) {
    observing_network_connectivity_changes_ = false;
    for (auto& observer : observers_) {
      observer.OnConnectionStatusChange(CONNECTION_AUTH_ERROR);
    }
  }

  if (event.connection_code == HttpResponse::SYNC_SERVER_ERROR) {
    for (auto& observer : observers_) {
      observer.OnConnectionStatusChange(CONNECTION_SERVER_ERROR);
    }
  }
}

void SyncManagerImpl::HandleTransactionCompleteChangeEvent(
    ModelTypeSet models_with_changes) {
  // This notification happens immediately after the transaction mutex is
  // released. This allows work to be performed without blocking other threads
  // from acquiring a transaction.
  if (!change_delegate_)
    return;

  // Call commit.
  for (ModelType type : models_with_changes) {
    change_delegate_->OnChangesComplete(type);
    change_observer_.Call(
        FROM_HERE, &SyncManager::ChangeObserver::OnChangesComplete, type);
  }
}

ModelTypeSet SyncManagerImpl::HandleTransactionEndingChangeEvent(
    const ImmutableWriteTransactionInfo& write_transaction_info,
    syncable::BaseTransaction* trans) {
  // This notification happens immediately before a syncable WriteTransaction
  // falls out of scope. It happens while the channel mutex is still held,
  // and while the transaction mutex is held, so it cannot be re-entrant.
  if (!change_delegate_ || change_records_.empty())
    return ModelTypeSet();

  // This will continue the WriteTransaction using a read only wrapper.
  // This is the last chance for read to occur in the WriteTransaction
  // that's closing. This special ReadTransaction will not close the
  // underlying transaction.
  ReadTransaction read_trans(GetUserShare(), trans);

  ModelTypeSet models_with_changes;
  for (ChangeRecordMap::const_iterator it = change_records_.begin();
       it != change_records_.end(); ++it) {
    DCHECK(!it->second.Get().empty());
    ModelType type = ModelTypeFromInt(it->first);
    change_delegate_->OnChangesApplied(
        type, trans->directory()->GetTransactionVersion(type), &read_trans,
        it->second);
    change_observer_.Call(FROM_HERE,
                          &SyncManager::ChangeObserver::OnChangesApplied, type,
                          write_transaction_info.Get().id, it->second);
    models_with_changes.Put(type);
  }
  change_records_.clear();
  return models_with_changes;
}

void SyncManagerImpl::HandleCalculateChangesChangeEventFromSyncApi(
    const ImmutableWriteTransactionInfo& write_transaction_info,
    syncable::BaseTransaction* trans,
    std::vector<int64_t>* entries_changed) {
  // We have been notified about a user action changing a sync model.
  LOG_IF(WARNING, !change_records_.empty())
      << "CALCULATE_CHANGES called with unapplied old changes.";

  // The mutated model type, or UNSPECIFIED if nothing was mutated.
  ModelTypeSet mutated_model_types;

  const syncable::ImmutableEntryKernelMutationMap& mutations =
      write_transaction_info.Get().mutations;
  for (auto it = mutations.Get().begin(); it != mutations.Get().end(); ++it) {
    if (!it->second.mutated.ref(syncable::IS_UNSYNCED)) {
      continue;
    }

    ModelType model_type =
        GetModelTypeFromSpecifics(it->second.mutated.ref(SPECIFICS));
    if (model_type < FIRST_REAL_MODEL_TYPE) {
      NOTREACHED() << "Permanent or underspecified item changed via syncapi.";
      continue;
    }

    // Found real mutation.
    if (model_type != UNSPECIFIED) {
      mutated_model_types.Put(model_type);
      entries_changed->push_back(it->second.mutated.ref(syncable::META_HANDLE));
    }
  }

  // Nudge if necessary.
  if (!mutated_model_types.Empty()) {
    if (weak_handle_this_.IsInitialized()) {
      weak_handle_this_.Call(FROM_HERE,
                             &SyncManagerImpl::RequestNudgeForDataTypes,
                             FROM_HERE, mutated_model_types);
    } else {
      NOTREACHED();
    }
  }
}

void SyncManagerImpl::SetExtraChangeRecordData(
    int64_t id,
    ModelType type,
    ChangeReorderBuffer* buffer,
    Cryptographer* cryptographer,
    const syncable::EntryKernel& original,
    bool existed_before,
    bool exists_now) {
  // If this is a deletion and the datatype was encrypted, we need to decrypt it
  // and attach it to the buffer.
  if (!exists_now && existed_before) {
    sync_pb::EntitySpecifics original_specifics(original.ref(SPECIFICS));
    if (type == PASSWORDS) {
      // Passwords must use their own legacy ExtraPasswordChangeRecordData.
      std::unique_ptr<sync_pb::PasswordSpecificsData> data(
          DecryptPasswordSpecifics(original_specifics, cryptographer));
      if (!data) {
        NOTREACHED();
        return;
      }
      buffer->SetExtraDataForId(id, new ExtraPasswordChangeRecordData(*data));
    } else if (original_specifics.has_encrypted()) {
      // All other datatypes can just create a new unencrypted specifics and
      // attach it.
      const sync_pb::EncryptedData& encrypted = original_specifics.encrypted();
      if (!cryptographer->Decrypt(encrypted, &original_specifics)) {
        NOTREACHED();
        return;
      }
    }
    buffer->SetSpecificsForId(id, original_specifics);
  }
}

void SyncManagerImpl::HandleCalculateChangesChangeEventFromSyncer(
    const ImmutableWriteTransactionInfo& write_transaction_info,
    syncable::BaseTransaction* trans,
    std::vector<int64_t>* entries_changed) {
  // We only expect one notification per sync step, so change_buffers_ should
  // contain no pending entries.
  LOG_IF(WARNING, !change_records_.empty())
      << "CALCULATE_CHANGES called with unapplied old changes.";

  ChangeReorderBuffer change_buffers[MODEL_TYPE_COUNT];

  Cryptographer* crypto = directory()->GetCryptographer(trans);
  const syncable::ImmutableEntryKernelMutationMap& mutations =
      write_transaction_info.Get().mutations;
  for (auto it = mutations.Get().begin(); it != mutations.Get().end(); ++it) {
    bool existed_before = !it->second.original.ref(syncable::IS_DEL);
    bool exists_now = !it->second.mutated.ref(syncable::IS_DEL);

    // Omit items that aren't associated with a model.
    ModelType type =
        GetModelTypeFromSpecifics(it->second.mutated.ref(SPECIFICS));
    if (type < FIRST_REAL_MODEL_TYPE)
      continue;

    int64_t handle = it->first;
    if (exists_now && !existed_before)
      change_buffers[type].PushAddedItem(handle);
    else if (!exists_now && existed_before)
      change_buffers[type].PushDeletedItem(handle);
    else if (exists_now && existed_before &&
             VisiblePropertiesDiffer(it->second, crypto))
      change_buffers[type].PushUpdatedItem(handle);

    SetExtraChangeRecordData(handle, type, &change_buffers[type], crypto,
                             it->second.original, existed_before, exists_now);
  }

  ReadTransaction read_trans(GetUserShare(), trans);
  for (int i = FIRST_REAL_MODEL_TYPE; i < MODEL_TYPE_COUNT; ++i) {
    if (!change_buffers[i].IsEmpty()) {
      if (change_buffers[i].GetAllChangesInTreeOrder(&read_trans,
                                                     &(change_records_[i]))) {
        for (size_t j = 0; j < change_records_[i].Get().size(); ++j)
          entries_changed->push_back((change_records_[i].Get())[j].id);
      }
      if (change_records_[i].Get().empty())
        change_records_.erase(i);
    }
  }
}

void SyncManagerImpl::RequestNudgeForDataTypes(
    const base::Location& nudge_location,
    ModelTypeSet types) {
  debug_info_event_listener_.OnNudgeFromDatatype(*(types.begin()));

  scheduler_->ScheduleLocalNudge(types, nudge_location);
}

void SyncManagerImpl::NudgeForInitialDownload(ModelType type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  scheduler_->ScheduleInitialSyncNudge(type);
}

void SyncManagerImpl::NudgeForCommit(ModelType type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RequestNudgeForDataTypes(FROM_HERE, ModelTypeSet(type));
}

void SyncManagerImpl::NudgeForRefresh(ModelType type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RefreshTypes(ModelTypeSet(type));
}

void SyncManagerImpl::OnSyncCycleEvent(const SyncCycleEvent& event) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Only send an event if this is due to a cycle ending and this cycle
  // concludes a canonical "sync" process; that is, based on what is known
  // locally we are "all happy" and up to date.  There may be new changes on
  // the server, but we'll get them on a subsequent sync.
  //
  // Notifications are sent at the end of every sync cycle, regardless of
  // whether we should sync again.
  if (event.what_happened == SyncCycleEvent::SYNC_CYCLE_ENDED) {
    if (!initialized_) {
      DVLOG(1) << "OnSyncCycleCompleted not sent because sync api is not "
               << "initialized";
      return;
    }

    DVLOG(1) << "Sending OnSyncCycleCompleted";
    for (auto& observer : observers_) {
      observer.OnSyncCycleCompleted(event.snapshot);
    }
  }
}

void SyncManagerImpl::OnActionableError(const SyncProtocolError& error) {
  for (auto& observer : observers_) {
    observer.OnActionableError(error);
  }
}

void SyncManagerImpl::OnRetryTimeChanged(base::Time) {}

void SyncManagerImpl::OnThrottledTypesChanged(ModelTypeSet) {}

void SyncManagerImpl::OnBackedOffTypesChanged(ModelTypeSet) {}

void SyncManagerImpl::OnMigrationRequested(ModelTypeSet types) {
  for (auto& observer : observers_) {
    observer.OnMigrationRequested(types);
  }
}

void SyncManagerImpl::OnProtocolEvent(const ProtocolEvent& event) {
  protocol_event_buffer_.RecordProtocolEvent(event);
  for (auto& observer : observers_) {
    observer.OnProtocolEvent(event);
  }
}

void SyncManagerImpl::SetJsEventHandler(
    const WeakHandle<JsEventHandler>& event_handler) {
  js_sync_manager_observer_.SetJsEventHandler(event_handler);
  js_mutation_event_observer_.SetJsEventHandler(event_handler);
  js_sync_encryption_handler_observer_.SetJsEventHandler(event_handler);
}

void SyncManagerImpl::SetInvalidatorEnabled(bool invalidator_enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DVLOG(1) << "Invalidator enabled state is now: " << invalidator_enabled;
  allstatus_.SetNotificationsEnabled(invalidator_enabled);
  scheduler_->SetNotificationsEnabled(invalidator_enabled);
}

void SyncManagerImpl::OnIncomingInvalidation(
    ModelType type,
    std::unique_ptr<InvalidationInterface> invalidation) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  allstatus_.IncrementNotificationsReceived();
  scheduler_->ScheduleInvalidationNudge(type, std::move(invalidation),
                                        FROM_HERE);
}

void SyncManagerImpl::RefreshTypes(ModelTypeSet types) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (types.Empty()) {
    LOG(WARNING) << "Sync received refresh request with no types specified.";
  } else {
    scheduler_->ScheduleLocalRefreshRequest(types, FROM_HERE);
  }
}

SyncStatus SyncManagerImpl::GetDetailedStatus() const {
  return allstatus_.status();
}

void SyncManagerImpl::SaveChanges() {
  directory()->SaveChanges();
}

UserShare* SyncManagerImpl::GetUserShare() {
  DCHECK(initialized_);
  return &share_;
}

ModelTypeConnector* SyncManagerImpl::GetModelTypeConnector() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return model_type_registry_.get();
}

std::unique_ptr<ModelTypeConnector>
SyncManagerImpl::GetModelTypeConnectorProxy() {
  DCHECK(initialized_);
  return std::make_unique<ModelTypeConnectorProxy>(
      base::SequencedTaskRunnerHandle::Get(),
      model_type_registry_->AsWeakPtr());
}

const std::string SyncManagerImpl::cache_guid() {
  DCHECK(initialized_);
  return directory()->cache_guid();
}

bool SyncManagerImpl::ReceivedExperiment(Experiments* experiments) {
  ReadTransaction trans(FROM_HERE, GetUserShare());
  ReadNode nigori_node(&trans);
  if (nigori_node.InitTypeRoot(NIGORI) != BaseNode::INIT_OK) {
    DVLOG(1) << "Couldn't find Nigori node.";
    return false;
  }
  bool found_experiment = false;

  ReadNode favicon_sync_node(&trans);
  if (favicon_sync_node.InitByClientTagLookup(EXPERIMENTS, kFaviconSyncTag) ==
      BaseNode::INIT_OK) {
    experiments->favicon_sync_limit =
        favicon_sync_node.GetExperimentsSpecifics()
            .favicon_sync()
            .favicon_sync_limit();
    found_experiment = true;
  }

  ReadNode pre_commit_update_avoidance_node(&trans);
  if (pre_commit_update_avoidance_node.InitByClientTagLookup(
          EXPERIMENTS, kPreCommitUpdateAvoidanceTag) == BaseNode::INIT_OK) {
    cycle_context_->set_server_enabled_pre_commit_update_avoidance(
        pre_commit_update_avoidance_node.GetExperimentsSpecifics()
            .pre_commit_update_avoidance()
            .enabled());
    // We don't bother setting found_experiment.  The frontend doesn't need to
    // know about this.
  }

  return found_experiment;
}

bool SyncManagerImpl::HasUnsyncedItemsForTest() {
  return model_type_registry_->HasUnsyncedItems();
}

SyncEncryptionHandler* SyncManagerImpl::GetEncryptionHandler() {
  return sync_encryption_handler_.get();
}

std::vector<std::unique_ptr<ProtocolEvent>>
SyncManagerImpl::GetBufferedProtocolEvents() {
  return protocol_event_buffer_.GetBufferedProtocolEvents();
}

void SyncManagerImpl::RegisterDirectoryTypeDebugInfoObserver(
    TypeDebugInfoObserver* observer) {
  model_type_registry_->RegisterDirectoryTypeDebugInfoObserver(observer);
}

void SyncManagerImpl::UnregisterDirectoryTypeDebugInfoObserver(
    TypeDebugInfoObserver* observer) {
  model_type_registry_->UnregisterDirectoryTypeDebugInfoObserver(observer);
}

bool SyncManagerImpl::HasDirectoryTypeDebugInfoObserver(
    TypeDebugInfoObserver* observer) {
  return model_type_registry_->HasDirectoryTypeDebugInfoObserver(observer);
}

void SyncManagerImpl::RequestEmitDebugInfo() {
  model_type_registry_->RequestEmitDebugInfo();
}

void SyncManagerImpl::ClearServerData(const base::Closure& callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  scheduler_->Start(SyncScheduler::CLEAR_SERVER_DATA_MODE, base::Time());
  ClearParams params(callback);
  scheduler_->ScheduleClearServerData(params);
}

void SyncManagerImpl::OnCookieJarChanged(bool account_mismatch,
                                         bool empty_jar) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  cycle_context_->set_cookie_jar_mismatch(account_mismatch);
  cycle_context_->set_cookie_jar_empty(empty_jar);
}

void SyncManagerImpl::OnMemoryDump(base::trace_event::ProcessMemoryDump* pmd) {
  directory()->OnMemoryDump(pmd);
}

}  // namespace syncer
