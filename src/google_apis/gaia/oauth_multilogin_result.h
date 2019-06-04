// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_APIS_GAIA_OAUTH_MULTILOGIN_RESULT_H_
#define GOOGLE_APIS_GAIA_OAUTH_MULTILOGIN_RESULT_H_

#include <string>
#include <unordered_map>

#include "base/time/time.h"
#include "base/values.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_constants.h"
#include "net/url_request/url_request_status.h"
#include "url/gurl.h"

// Values for the 'status' field of multilogin responses. Used for UMA logging,
// do not remove or reorder values.
enum class OAuthMultiloginResponseStatus {
  // Status could not be parsed.
  kUnknownStatus = 0,

  // The request was processed successfully, and the rest of this object
  // contains the cookies to set across domains. The HTTP status code will be
  // 200.
  kOk = 1,

  // Something happened while processing the request that made it fail. It is
  // suspected to be a transient issue, so the client may retry at a later time
  // with exponential backoff. The HTTP status code will be 503.
  kRetry = 2,

  // The input parameters were not as expected (wrong header format, missing
  // parameters, etc). Retrying without changing input parameters will not work.
  // The HTTP status code will be 400.
  kInvalidInput = 3,

  // At least one provided token could not be used to authenticate the
  // corresponding user. This includes the case where the provided Gaia ID does
  // not match with the corresponding OAuth token. The HTTP status code will be
  // 403.
  kInvalidTokens = 4,

  // An error occurred while processing the request, and retrying is not
  // expected to work. The HTTP status code will be 500.
  kError = 5,

  kMaxValue = kError,
};

// Parses the status field of the response.
OAuthMultiloginResponseStatus ParseOAuthMultiloginResponseStatus(
    const std::string& status);

class OAuthMultiloginResult {
 public:
  // Parses cookies and status from JSON response. Maps status to
  // GoogleServiceAuthError::State values or sets error to
  // UNEXPECTED_SERVER_RESPONSE if JSON string cannot be parsed.
  OAuthMultiloginResult(const std::string& raw_data);

  OAuthMultiloginResult(const GoogleServiceAuthError& error);
  OAuthMultiloginResult(const OAuthMultiloginResult& other);
  OAuthMultiloginResult& operator=(const OAuthMultiloginResult& other);
  ~OAuthMultiloginResult();

  std::vector<net::CanonicalCookie> cookies() const { return cookies_; }
  std::vector<std::string> failed_accounts() const { return failed_accounts_; }
  GoogleServiceAuthError error() const { return error_; }

 private:
  FRIEND_TEST_ALL_PREFIXES(OAuthMultiloginResultTest, TryParseCookiesFromValue);

  // Response body that has a form of JSON contains protection characters
  // against XSSI that have to be removed. See go/xssi.
  static base::StringPiece StripXSSICharacters(const std::string& data);

  // Maps status in JSON response to one of the GoogleServiceAuthError state
  // values.
  void TryParseStatusFromValue(base::DictionaryValue* dictionary_value);

  void TryParseCookiesFromValue(base::DictionaryValue* dictionary_value);

  // If error is INVALID_GAIA_CREDENTIALS response is expected to have a list of
  // failed accounts for which tokens are not valid.
  void TryParseFailedAccountsFromValue(base::DictionaryValue* dictionary_value);

  std::vector<net::CanonicalCookie> cookies_;
  std::vector<std::string> failed_accounts_;
  GoogleServiceAuthError error_;
};

#endif  // GOOGLE_APIS_GAIA_OAUTH_MULTILOGIN_RESULT_H_
