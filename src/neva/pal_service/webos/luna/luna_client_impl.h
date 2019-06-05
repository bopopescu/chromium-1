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

#ifndef NEVA_PAL_SERVICE_WEBOS_LUNA_LUNA_CLIENT_IMPL_H_
#define NEVA_PAL_SERVICE_WEBOS_LUNA_LUNA_CLIENT_IMPL_H_

#include "neva/pal_service/webos/luna/luna_client.h"
#include <lunaservice.h>
#include <map>

namespace pal {
namespace luna {

class ClientImpl : public Client {
 public:
  ClientImpl(const Params& params);
  ~ClientImpl() override;

  bool IsInitialized() const override;
  Bus GetBusType() const override;
  std::string GetName() const override;
  std::string GetAppId() const override;

  bool Call(std::string uri,
            std::string param,
            OnceResponse callback = base::DoNothing(),
            std::string on_cancel_value = std::string()) override;

  bool Subscribe(std::string uri,
                 std::string param,
                 RepeatingResponse callback,
                 unsigned* token) override;

  void Unsubscribe(unsigned token) override;

  void CancelWaitingCalls() override;

 private:
  struct Response {
    Client::OnceResponse callback;
    ClientImpl* ptr = nullptr;
    LSMessageToken token;
    std::string uri;
    std::string param;
    std::string on_cancel_value;
  };

  struct Subscription {
    Client::RepeatingResponse callback;
    LSMessageToken token;
    std::string uri;
    std::string param;
  };

  void CancelAllSubscriptions();
  void CancelAllResponses();

  static bool HandleResponse(LSHandle* sh, LSMessage* reply, void* ctx);
  static bool HandleSubscribe(LSHandle* sh, LSMessage* reply, void* ctx);

  Params params_;
  LSHandle* handle_ = nullptr;
  GMainContext* context_ = nullptr;

  std::map<Response*, std::unique_ptr<Response>> responses_;
  std::map<unsigned, std::unique_ptr<Subscription>> subscriptions_;
};

}  // luna
}  // pal

#endif  // NEVA_PAL_SERVICE_WEBOS_LUNA_LUNA_CLIENT_IMPL_H_
