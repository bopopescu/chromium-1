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

#ifndef NEVA_INJECTION_WEBOSSYSTEM_WEBOS_SERVICEBRIDGE_INJECTION_H_
#define NEVA_INJECTION_WEBOSSYSTEM_WEBOS_SERVICEBRIDGE_INJECTION_H_

#include "gin/arguments.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "neva/pal_service/public/mojom/system_servicebridge.mojom.h"

#include <memory>
#include <set>
#include <string>

namespace injections {

class WebOSServiceBridgeInjection
    : public gin::Wrappable<WebOSServiceBridgeInjection>
    , public pal::mojom::SystemServiceBridgeClient {
 public:
  static gin::WrapperInfo kWrapperInfo;

  WebOSServiceBridgeInjection();
  ~WebOSServiceBridgeInjection();

  // To handle luna call in webOSSystem.onclose callback
  static std::set<WebOSServiceBridgeInjection*> waiting_responses_;
  static bool is_closing_;

 private:
  void Call(gin::Arguments* args);
  void DoCall(std::string uri, std::string payload);
  void Cancel();

  void SetupIdentifier();
  void CloseNotify();

  void OnConnect(
      pal::mojom::SystemServiceBridgeClientAssociatedRequest request);
  void Response(const std::string& payload) override;

  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  std::string identifier_;
  mojo::AssociatedBinding<pal::mojom::SystemServiceBridgeClient>
      client_binding_;
  pal::mojom::SystemServiceBridgePtr system_bridge_;
};

}  // namespace injections

#endif  // NEVA_INJECTION_WEBOSSYSTEM_WEBOS_SERVICEBRIDGE_INJECTION_H_
