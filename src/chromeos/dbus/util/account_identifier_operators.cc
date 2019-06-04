// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/util/account_identifier_operators.h"

namespace cryptohome {

bool operator<(const AccountIdentifier& l, const AccountIdentifier& r) {
  return l.account_id() < r.account_id();
}

bool operator==(const AccountIdentifier& l, const AccountIdentifier& r) {
  return l.account_id() == r.account_id();
}

}  // namespace cryptohome
