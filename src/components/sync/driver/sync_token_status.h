// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_SYNC_TOKEN_STATUS_H_
#define COMPONENTS_SYNC_DRIVER_SYNC_TOKEN_STATUS_H_

#include "base/time/time.h"
#include "components/sync/engine/connection_status.h"
#include "google_apis/gaia/google_service_auth_error.h"

namespace syncer {

// Status of sync server connection, sync token and token request.
struct SyncTokenStatus {
  SyncTokenStatus();

  // Sync server connection status reported by the sync engine.
  base::Time connection_status_update_time;
  ConnectionStatus connection_status = CONNECTION_NOT_ATTEMPTED;

  // The last times when an OAuth2 access token was requested and received.
  base::Time token_request_time;
  base::Time token_receive_time;

  // Whether we currently have an OAuth2 access token.
  bool has_token = false;

  // The error returned by OAuth2TokenService for the last token request.
  GoogleServiceAuthError last_get_token_error;

  // The time when the next token request is scheduled, or a null time if no
  // request is scheduled.
  base::Time next_token_request_time;
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DRIVER_SYNC_TOKEN_STATUS_H_
