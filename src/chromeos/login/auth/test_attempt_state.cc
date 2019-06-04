// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/login/auth/test_attempt_state.h"

#include "components/user_manager/user_type.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

TestAttemptState::TestAttemptState(const UserContext& credentials,
                                   const bool user_is_new)
    : AuthAttemptState(credentials,
                       false,  // unlock
                       false,  // online_complete
                       user_is_new) {
}

TestAttemptState::~TestAttemptState() = default;

void TestAttemptState::PresetOnlineLoginStatus(const AuthFailure& outcome) {
  online_complete_ = true;
  online_outcome_ = outcome;
}

void TestAttemptState::PresetCryptohomeStatus(
    cryptohome::MountError cryptohome_code) {
  cryptohome_complete_ = true;
  cryptohome_code_ = cryptohome_code;
}

bool TestAttemptState::online_complete() {
  return online_complete_;
}

const AuthFailure& TestAttemptState::online_outcome() {
  return online_outcome_;
}

bool TestAttemptState::is_first_time_user() {
  return is_first_time_user_;
}

bool TestAttemptState::cryptohome_complete() {
  return cryptohome_complete_;
}

cryptohome::MountError TestAttemptState::cryptohome_code() {
  return cryptohome_code_;
}

}  // namespace chromeos
