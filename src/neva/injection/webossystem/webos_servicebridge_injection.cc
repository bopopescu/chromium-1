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

#include "neva/injection/webossystem/webos_servicebridge_injection.h"

#include "base/rand_util.h"
#include "content/public/child/child_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/renderer/render_frame.h"
#include "gin/dictionary.h"
#include "gin/function_template.h"
#include "gin/handle.h"
#include "neva/injection/common/gin/function_template_neva.h"
#include "neva/pal_service/public/mojom/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {

const char kCallMethodName[] = "call";
const char kCancelMethodName[] = "cancel";
const char kOnServiceCallbackName[] = "onservicecallback";


std::string GetServiceNameWithPID(const std::string& name) {
  std::string result(name);
  return result.append("-").append(std::to_string(getpid()));
}

}  // anonymous namespace

namespace injections {

gin::WrapperInfo WebOSServiceBridgeInjection::kWrapperInfo = {
  gin::kEmbedderNativeGin };

bool WebOSServiceBridgeInjection::is_closing_ = false;
std::set<WebOSServiceBridgeInjection*>
    WebOSServiceBridgeInjection::waiting_responses_;

WebOSServiceBridgeInjection::WebOSServiceBridgeInjection()
    : client_binding_(this) {
  SetupIdentifier();

  if (identifier_.empty())
    return;

  service_manager::Connector* connector =
      content::ChildThread::Get()->GetConnector();

  if (!connector)
    return;

  pal::mojom::SystemServiceBridgeProviderPtr provider;
  connector->BindInterface(pal::mojom::kServiceName,
                           mojo::MakeRequest(&provider));

  provider->GetSystemServiceBridge(mojo::MakeRequest(&system_bridge_));
  system_bridge_->Connect(
      GetServiceNameWithPID(identifier_),
      identifier_,
      base::BindRepeating(
          &WebOSServiceBridgeInjection::OnConnect,
          base::Unretained(this)));
}

WebOSServiceBridgeInjection::~WebOSServiceBridgeInjection() {
  Cancel();
}

void WebOSServiceBridgeInjection::Call(gin::Arguments* args) {
  std::string uri;
  std::string payload;
  if (args->GetNext(&uri) && args->GetNext(&payload))
    DoCall(std::move(uri), std::move(payload));
  else
    DoCall(std::string(""), std::string(""));
}

void WebOSServiceBridgeInjection::DoCall(std::string uri, std::string payload) {
  if (identifier_.empty())
    return;

  waiting_responses_.insert(this);
  system_bridge_->Call(std::move(uri), std::move(payload));
}

void WebOSServiceBridgeInjection::Cancel() {
  if (identifier_.empty())
    return;

  system_bridge_->Cancel();
  waiting_responses_.erase(this);
}

void WebOSServiceBridgeInjection::OnConnect(
    pal::mojom::SystemServiceBridgeClientAssociatedRequest request) {
  client_binding_.Bind(std::move(request));
}

void WebOSServiceBridgeInjection::Response(const std::string& body) {
  bool shouldCallCloseNotify = false;
  if (WebOSServiceBridgeInjection::is_closing_) {
    waiting_responses_.erase(this);
    if (waiting_responses_.empty())
      shouldCallCloseNotify = true;
  }

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::MaybeLocal<v8::Object> maybe_wrapper = GetWrapper(isolate);
  v8::Local<v8::Object> wrapper;
  if (!maybe_wrapper.ToLocal(&wrapper))
    return;

  v8::Context::Scope context_scope(wrapper->CreationContext());
  v8::Local<v8::String> callback_key(
      v8::String::NewFromUtf8(isolate, kOnServiceCallbackName));

  if (!wrapper->Has(callback_key))
    return;

  v8::Local<v8::Value> func_value = wrapper->Get(callback_key);
  if (!func_value->IsFunction())
    return;

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(func_value);

  const int argc = 1;
  v8::Local<v8::Value> argv[] = { gin::StringToV8(isolate, body) };
  func->Call(wrapper, argc, argv);

  if (shouldCallCloseNotify) {
    // This function should be executed after context_scope(context)
    // Unless there will be libv8.so crash
    CloseNotify();
  }
}

gin::ObjectTemplateBuilder
    WebOSServiceBridgeInjection::GetObjectTemplateBuilder(
        v8::Isolate* isolate) {
   return gin::Wrappable<WebOSServiceBridgeInjection>::
      GetObjectTemplateBuilder(isolate)
        .SetMethod(kCallMethodName, &WebOSServiceBridgeInjection::Call)
        .SetMethod(kCancelMethodName, &WebOSServiceBridgeInjection::Cancel);
}

void WebOSServiceBridgeInjection::SetupIdentifier() {
  const char* data = "webOSSystem.getIdentifier()";
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::String> source = gin::StringToV8(isolate, data);
  v8::MaybeLocal<v8::Script> script = v8::Script::Compile(context, source);
  if (script.IsEmpty())
    return;

  v8::MaybeLocal<v8::Value> result = script.ToLocalChecked()->Run(context);
  if (!result.IsEmpty() && result.ToLocalChecked()->IsString())
    identifier_ = gin::V8ToString(isolate, result.ToLocalChecked());
}

void WebOSServiceBridgeInjection::CloseNotify() {
  if (!WebOSServiceBridgeInjection::is_closing_ ||
      !WebOSServiceBridgeInjection::waiting_responses_.empty())
    return;

  const char* data = "webOSSystem.onCloseNotify(\"didRunOnCloseCallback\")";
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::String> source = gin::StringToV8(isolate, data);
  v8::MaybeLocal<v8::Script> script = v8::Script::Compile(context, source);
  if (!script.IsEmpty())
    script.ToLocalChecked()->Run(context);
}

}  // namespace injections
