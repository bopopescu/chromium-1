// Copyright 2019 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "neva/pal_service/webos/system_servicebridge_delegate_webos.h"

#include "base/callback.h"
#include "neva/pal_service/webos/luna/luna_names.h"

namespace pal {
namespace webos {

SystemServiceBridgeDelegateWebOS::SystemServiceBridgeDelegateWebOS(
        std::string name,
        std::string appid,
        Response callback)
    : callback_(std::move(callback))
    , weak_factory_(this) {
  luna::Client::Params params;
  params.bus = luna::Bus::Private;
  params.name = std::move(name);
  params.appid = std::move(appid);
  luna_client_ = luna::GetSharedClient(params);
}

SystemServiceBridgeDelegateWebOS::~SystemServiceBridgeDelegateWebOS() {
  Cancel();
}

void SystemServiceBridgeDelegateWebOS::Call(std::string uri,
                                            std::string payload) {
  if (!luna_client_)
    return;

  if (luna::IsSubscription(payload)) {
    unsigned token;
    const bool subscribed = luna_client_->Subscribe(
        uri,
        payload,
        base::BindRepeating(
            &SystemServiceBridgeDelegateWebOS::OnResponse,
            weak_factory_.GetWeakPtr()),
        &token);
    if (subscribed)
      subscription_tokens_.push_back(token);
  } else {
    luna_client_->Call(
        uri,
        payload,
        base::BindOnce(
            &SystemServiceBridgeDelegateWebOS::OnResponse,
            weak_factory_.GetWeakPtr()),
        std::string("{}"));
  }
}

void SystemServiceBridgeDelegateWebOS::Cancel() {
  if (luna_client_) {
    for (const auto token : subscription_tokens_)
      luna_client_->Unsubscribe(token);
    luna_client_->CancelWaitingCalls();
  }
}

void SystemServiceBridgeDelegateWebOS::OnResponse(const std::string& json) {
  callback_.Run(json);
}

}  // namespace webos
}  // namespace pal
