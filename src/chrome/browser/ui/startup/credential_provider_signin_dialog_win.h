// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_STARTUP_CREDENTIAL_PROVIDER_SIGNIN_DIALOG_WIN_H_
#define CHROME_BROWSER_UI_STARTUP_CREDENTIAL_PROVIDER_SIGNIN_DIALOG_WIN_H_

#include <string>

#include "base/callback.h"
#include "base/memory/scoped_refptr.h"

namespace base {
class CommandLine;
class Value;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace views {
class WebDialogView;
}  // namespace views

// Callback signalled by the dialog when the Gaia sign in flow compltes.
// Parameters are:
// 1. A base::Value that is of type DICTIONARY. An empty dictionary signals an
// error during the signin process
// 2. The signed in user's access token.
// 3. The signed in user's refresh token.
// 4. A URL loader that will be used by various OAuth fetchers.
using HandleGcpwSigninCompleteResult =
    base::OnceCallback<void(base::Value,
                            const std::string&,
                            const std::string&,
                            scoped_refptr<network::SharedURLLoaderFactory>)>;

// Starts the Google Credential Provider for Windows (GCPW) Sign in flow. First
// the function shows a frameless Google account sign in page allowing the user
// to choose an  account to logon to Windows. Once the signin is complete, the
// flow will automatically start requesting additional information required by
// GCPW to complete Windows logon.
void StartGCPWSignin(const base::CommandLine& command_line,
                     content::BrowserContext* context);

// This function displays a dialog window with a Gaia signin page. Once
// the Gaia signin flow is finished, the callback given by
// |signin_complete_handler| will be called with the results of the signin.
// The return value is only valid during the lifetime of the dialog.
views::WebDialogView* ShowCredentialProviderSigninDialog(
    const base::CommandLine& command_line,
    content::BrowserContext* context,
    HandleGcpwSigninCompleteResult signin_complete_handler);

#endif  // CHROME_BROWSER_UI_STARTUP_CREDENTIAL_PROVIDER_SIGNIN_DIALOG_WIN_H_
