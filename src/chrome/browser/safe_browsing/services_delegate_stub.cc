// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/services_delegate_stub.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "components/safe_browsing/android/remote_database_manager.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/preferences/public/mojom/tracked_preference_validation_delegate.mojom.h"

namespace safe_browsing {

// static
std::unique_ptr<ServicesDelegate> ServicesDelegate::Create(
    SafeBrowsingService* safe_browsing_service) {
  return base::WrapUnique(new ServicesDelegateStub);
}

// static
std::unique_ptr<ServicesDelegate> ServicesDelegate::CreateForTest(
    SafeBrowsingService* safe_browsing_service,
    ServicesDelegate::ServicesCreator* services_creator) {
  NOTREACHED();
  return base::WrapUnique(new ServicesDelegateStub);
}

ServicesDelegateStub::ServicesDelegateStub() {}

ServicesDelegateStub::~ServicesDelegateStub() {}

void ServicesDelegateStub::InitializeCsdService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {}

const scoped_refptr<SafeBrowsingDatabaseManager>&
ServicesDelegateStub::database_manager() const {
  return database_manager_;
}

void ServicesDelegateStub::Initialize() {
  if (!database_manager_set_for_tests_) {
    database_manager_ =
        base::WrapRefCounted(new RemoteSafeBrowsingDatabaseManager());
  }
}
void ServicesDelegateStub::SetDatabaseManagerForTest(
    SafeBrowsingDatabaseManager* database_manager) {
  database_manager_set_for_tests_ = true;
  database_manager_ = database_manager;
}

void ServicesDelegateStub::ShutdownServices() {}

void ServicesDelegateStub::RefreshState(bool enable) {}

void ServicesDelegateStub::ProcessResourceRequest(
    const ResourceRequestInfo* request) {}

std::unique_ptr<prefs::mojom::TrackedPreferenceValidationDelegate>
ServicesDelegateStub::CreatePreferenceValidationDelegate(Profile* profile) {
  return nullptr;
}

void ServicesDelegateStub::RegisterDelayedAnalysisCallback(
    const DelayedAnalysisCallback& callback) {}

void ServicesDelegateStub::AddDownloadManager(
    content::DownloadManager* download_manager) {}

ClientSideDetectionService* ServicesDelegateStub::GetCsdService() {
  return nullptr;
}

DownloadProtectionService* ServicesDelegateStub::GetDownloadService() {
  return nullptr;
}

void ServicesDelegateStub::StartOnIOThread(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const V4ProtocolConfig& v4_config) {
  database_manager_->StartOnIOThread(url_loader_factory, v4_config);
}

void ServicesDelegateStub::StopOnIOThread(bool shutdown) {
  database_manager_->StopOnIOThread(shutdown);
}

void ServicesDelegateStub::CreatePasswordProtectionService(Profile* profile) {}
void ServicesDelegateStub::RemovePasswordProtectionService(Profile* profile) {}
PasswordProtectionService* ServicesDelegateStub::GetPasswordProtectionService(
    Profile* profile) const {
  NOTIMPLEMENTED();
  return nullptr;
}

}  // namespace safe_browsing
