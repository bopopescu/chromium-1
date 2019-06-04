// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/call_stack_profile_metrics_provider.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/no_destructor.h"
#include "base/synchronization/lock.h"
#include "base/thread_annotations.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "third_party/metrics_proto/chrome_user_metrics_extension.pb.h"

namespace metrics {

namespace {

// Cap the number of pending profiles to avoid excessive performance overhead
// due to profile deserialization when profile uploads are delayed (e.g. due to
// being offline). Capping at this threshold loses approximately 0.5% of
// profiles on canary and dev.
//
// TODO(wittman): Remove this threshold after crbug.com/903972 is fixed.
const size_t kMaxPendingProfiles = 1250;

// PendingProfiles ------------------------------------------------------------

// Singleton class responsible for retaining profiles received from
// CallStackProfileBuilder. These are then sent to UMA on the invocation of
// CallStackProfileMetricsProvider::ProvideCurrentSessionData(). We need to
// store the profiles outside of a CallStackProfileMetricsProvider instance
// since callers may start profiling before the CallStackProfileMetricsProvider
// is created.
//
// Member functions on this class may be called on any thread.
class PendingProfiles {
 public:
  static PendingProfiles* GetInstance();

  // Retrieves all the pending profiles.
  std::vector<SampledProfile> RetrieveProfiles();

  // Enables the collection of profiles by MaybeCollect*Profile if |enabled| is
  // true. Otherwise, clears the currently collected profiles and ignores
  // profiles provided to future invocations of MaybeCollect*Profile.
  void SetCollectionEnabled(bool enabled);

  // Collects |profile|. It may be stored in a serialized form, or ignored,
  // depending on the pre-defined storage capacity and whether collection is
  // enabled. |profile| is not const& because it must be passed with std::move.
  void MaybeCollectProfile(base::TimeTicks profile_start_time,
                           SampledProfile profile);

  // Collects |serialized_profile|. It may be ignored depending on the
  // pre-defined storage capacity and whether collection is enabled.
  // |serialized_profile| is not const& because it must be passed with
  // std::move.
  void MaybeCollectSerializedProfile(base::TimeTicks profile_start_time,
                                     std::string serialized_profile);

  // Allows testing against the initial state multiple times.
  void ResetToDefaultStateForTesting();

 private:
  friend class base::NoDestructor<PendingProfiles>;

  PendingProfiles();
  ~PendingProfiles() = delete;

  // Returns true if collection is enabled for a given profile based on its
  // |profile_start_time|. The |lock_| must be held prior to calling this
  // method.
  bool IsCollectionEnabledForProfile(base::TimeTicks profile_start_time) const
      EXCLUSIVE_LOCKS_REQUIRED(lock_);

  mutable base::Lock lock_;

  // If true, profiles provided to MaybeCollect*Profile should be collected.
  // Otherwise they will be ignored.
  bool collection_enabled_ GUARDED_BY(lock_);

  // The last time collection was disabled. Used to determine if collection was
  // disabled at any point since a profile was started.
  base::TimeTicks last_collection_disable_time_ GUARDED_BY(lock_);

  // The last time collection was enabled. Used to determine if collection was
  // enabled at any point since a profile was started.
  base::TimeTicks last_collection_enable_time_ GUARDED_BY(lock_);

  // The set of completed serialized profiles that should be reported.
  std::vector<std::string> serialized_profiles_ GUARDED_BY(lock_);

  DISALLOW_COPY_AND_ASSIGN(PendingProfiles);
};

// static
PendingProfiles* PendingProfiles::GetInstance() {
  // Singleton for performance rather than correctness reasons.
  static base::NoDestructor<PendingProfiles> instance;
  return instance.get();
}

std::vector<SampledProfile> PendingProfiles::RetrieveProfiles() {
  std::vector<std::string> serialized_profiles;

  {
    base::AutoLock scoped_lock(lock_);
    serialized_profiles.swap(serialized_profiles_);
  }

  // Deserialize all serialized profiles, skipping over any that fail to parse.
  base::ElapsedTimer timer;
  std::vector<SampledProfile> profiles;
  profiles.reserve(serialized_profiles.size());
  for (const auto& serialized_profile : serialized_profiles) {
    SampledProfile profile;
    if (profile.ParseFromArray(serialized_profile.data(),
                               serialized_profile.size())) {
      profiles.push_back(std::move(profile));
    }
  }
  UMA_HISTOGRAM_TIMES("StackSamplingProfiler.DeserializeAllPendingProfilesTime",
                      timer.Elapsed());

  return profiles;
}

void PendingProfiles::SetCollectionEnabled(bool enabled) {
  base::AutoLock scoped_lock(lock_);

  collection_enabled_ = enabled;

  if (!collection_enabled_) {
    serialized_profiles_.clear();
    last_collection_disable_time_ = base::TimeTicks::Now();
  } else {
    last_collection_enable_time_ = base::TimeTicks::Now();
  }
}

bool PendingProfiles::IsCollectionEnabledForProfile(
    base::TimeTicks profile_start_time) const {
  lock_.AssertAcquired();

  // Scenario 1: return false if collection is disabled.
  if (!collection_enabled_)
    return false;

  // Scenario 2: return false if collection is disabled after the start of
  // collection for this profile.
  if (!last_collection_disable_time_.is_null() &&
      last_collection_disable_time_ >= profile_start_time) {
    return false;
  }

  // Scenario 3: return false if collection is disabled before the start of
  // collection and re-enabled after the start. Note that this is different from
  // scenario 1 where re-enabling never happens.
  if (!last_collection_disable_time_.is_null() &&
      !last_collection_enable_time_.is_null() &&
      last_collection_enable_time_ >= profile_start_time) {
    return false;
  }

  return true;
}

void PendingProfiles::MaybeCollectProfile(base::TimeTicks profile_start_time,
                                          SampledProfile profile) {
  {
    base::AutoLock scoped_lock(lock_);

    if (!IsCollectionEnabledForProfile(profile_start_time))
      return;
  }

  // Serialize the profile without holding the lock.
  std::string serialized_profile;
  profile.SerializeToString(&serialized_profile);

  MaybeCollectSerializedProfile(profile_start_time,
                                std::move(serialized_profile));
}

void PendingProfiles::MaybeCollectSerializedProfile(
    base::TimeTicks profile_start_time,
    std::string serialized_profile) {
  base::AutoLock scoped_lock(lock_);

  // There is no room for additional profiles.
  if (serialized_profiles_.size() >= kMaxPendingProfiles)
    return;

  if (IsCollectionEnabledForProfile(profile_start_time))
    serialized_profiles_.push_back(std::move(serialized_profile));
}

void PendingProfiles::ResetToDefaultStateForTesting() {
  base::AutoLock scoped_lock(lock_);

  collection_enabled_ = true;
  last_collection_disable_time_ = base::TimeTicks();
  last_collection_enable_time_ = base::TimeTicks();
  serialized_profiles_.clear();
}

// |collection_enabled_| is initialized to true to collect any profiles that are
// generated prior to creation of the CallStackProfileMetricsProvider. The
// ultimate disposition of these pre-creation collected profiles will be
// determined by the initial recording state provided to
// CallStackProfileMetricsProvider.
PendingProfiles::PendingProfiles() : collection_enabled_(true) {}

}  // namespace

// CallStackProfileMetricsProvider --------------------------------------------

const base::Feature CallStackProfileMetricsProvider::kEnableReporting = {
    "SamplingProfilerReporting", base::FEATURE_ENABLED_BY_DEFAULT};

CallStackProfileMetricsProvider::CallStackProfileMetricsProvider() {}

CallStackProfileMetricsProvider::~CallStackProfileMetricsProvider() {}

// static
void CallStackProfileMetricsProvider::ReceiveProfile(
    base::TimeTicks profile_start_time,
    SampledProfile profile) {
  PendingProfiles::GetInstance()->MaybeCollectProfile(profile_start_time,
                                                      std::move(profile));
}

// static
void CallStackProfileMetricsProvider::ReceiveSerializedProfile(
    base::TimeTicks profile_start_time,
    std::string serialized_profile) {
  PendingProfiles::GetInstance()->MaybeCollectSerializedProfile(
      profile_start_time, std::move(serialized_profile));
}

void CallStackProfileMetricsProvider::OnRecordingEnabled() {
  PendingProfiles::GetInstance()->SetCollectionEnabled(
      base::FeatureList::IsEnabled(kEnableReporting));
}

void CallStackProfileMetricsProvider::OnRecordingDisabled() {
  PendingProfiles::GetInstance()->SetCollectionEnabled(false);
}

void CallStackProfileMetricsProvider::ProvideCurrentSessionData(
    ChromeUserMetricsExtension* uma_proto) {
  std::vector<SampledProfile> profiles =
      PendingProfiles::GetInstance()->RetrieveProfiles();

  DCHECK(base::FeatureList::IsEnabled(kEnableReporting) || profiles.empty());

  for (auto& profile : profiles)
    *uma_proto->add_sampled_profile() = std::move(profile);
}

// static
void CallStackProfileMetricsProvider::ResetStaticStateForTesting() {
  PendingProfiles::GetInstance()->ResetToDefaultStateForTesting();
}

}  // namespace metrics
