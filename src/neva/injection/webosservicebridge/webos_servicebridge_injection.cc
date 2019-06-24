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

#include "neva/injection/webosservicebridge/webos_servicebridge_injection.h"

#include "base/rand_util.h"
#include "content/public/child/child_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/renderer/render_frame.h"
#include "gin/dictionary.h"
#include "gin/function_template.h"
#include "gin/handle.h"
#include "neva/injection/common/gin/function_template_neva.h"
#include "neva/injection/grit/injection_resources.h"
#include "neva/pal_service/public/mojom/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

const char kCallMethodName[] = "call";
const char kCancelMethodName[] = "cancel";
const char kOnServiceCallbackName[] = "onservicecallback";
const char kMethodInvocationAsConstructorOnly[] =
    "webOSServiceBridge function must be invoked as a constructor only";

std::string GetServiceNameWithPID(const std::string& name) {
  std::string result(name);
  return result.append("-").append(std::to_string(getpid()));
}

}  // anonymous namespace

namespace injections {

gin::WrapperInfo WebOSServiceBridgeInjection::kWrapperInfo = {
    gin::kEmbedderNativeGin};

bool WebOSServiceBridgeInjection::is_closing_ = false;
std::set<WebOSServiceBridgeInjection*>
    WebOSServiceBridgeInjection::waiting_responses_;
v8::Persistent<v8::ObjectTemplate>
    WebOSServiceBridgeInjection::request_template_;

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
      GetServiceNameWithPID(identifier_), identifier_,
      base::BindRepeating(&WebOSServiceBridgeInjection::OnConnect,
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

  system_bridge_->Call(std::move(uri), std::move(payload));
  if (WebOSServiceBridgeInjection::is_closing_)
    waiting_responses_.insert(this);
}

void WebOSServiceBridgeInjection::Cancel() {
  if (identifier_.empty())
    return;

  system_bridge_->Cancel();
  waiting_responses_.erase(this);
}

void WebOSServiceBridgeInjection::OnServiceCallback(
    const gin::Arguments& args) {}

void WebOSServiceBridgeInjection::OnConnect(
    pal::mojom::SystemServiceBridgeClientAssociatedRequest request) {
  client_binding_.Bind(std::move(request));
}

void WebOSServiceBridgeInjection::CallJSHandler(const std::string& body) {
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
  v8::Local<v8::Value> argv[] = {gin::StringToV8(isolate, body)};
  func->Call(wrapper, argc, argv);
}

void WebOSServiceBridgeInjection::Response(pal::mojom::ResponseStatus status,
                                           const std::string& body) {
  if (status == pal::mojom::ResponseStatus::kSuccess)
    CallJSHandler(body);

  if (!WebOSServiceBridgeInjection::is_closing_)
    return;

  waiting_responses_.erase(this);

  if (waiting_responses_.empty())
    CloseNotify();
}

gin::ObjectTemplateBuilder
WebOSServiceBridgeInjection::GetObjectTemplateBuilder(v8::Isolate* isolate) {
  return gin::Wrappable<WebOSServiceBridgeInjection>::GetObjectTemplateBuilder(
             isolate)
      .SetMethod(kCallMethodName, &WebOSServiceBridgeInjection::Call)
      .SetMethod(kCancelMethodName, &WebOSServiceBridgeInjection::Cancel)
      .SetMethod(kOnServiceCallbackName,
                 &WebOSServiceBridgeInjection::OnServiceCallback);
}

void WebOSServiceBridgeInjection::SetupIdentifier() {
  const char* data = "webOSSystem.getIdentifier()";

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::MaybeLocal<v8::Object> maybe_wrapper = GetWrapper(isolate);
  v8::Local<v8::Object> wrapper;
  if (!maybe_wrapper.ToLocal(&wrapper))
    return;

  auto context = wrapper->CreationContext();
  v8::Local<v8::String> source = gin::StringToV8(isolate, data);
  v8::MaybeLocal<v8::Script> script = v8::Script::Compile(context, source);
  if (script.IsEmpty())
    return;

  v8::MaybeLocal<v8::Value> result = script.ToLocalChecked()->Run(context);
  if (!result.IsEmpty() && result.ToLocalChecked()->IsString())
    identifier_ = gin::V8ToString(isolate, result.ToLocalChecked());
}

void WebOSServiceBridgeInjection::CloseNotify() {
  const char* data = "webOSSystem.onCloseNotify(\"didRunOnCloseCallback\")";

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::MaybeLocal<v8::Object> maybe_wrapper = GetWrapper(isolate);
  v8::Local<v8::Object> wrapper;
  if (!maybe_wrapper.ToLocal(&wrapper))
    return;

  auto context = wrapper->CreationContext();
  v8::Context::Scope context_scope(wrapper->CreationContext());
  v8::Local<v8::String> source = gin::StringToV8(isolate, data);
  v8::MaybeLocal<v8::Script> script = v8::Script::Compile(context, source);
  if (!script.IsEmpty())
    script.ToLocalChecked()->Run(context);
}

const char WebOSServiceBridgeInjectionExtension::kInjectionName[] =
    "v8/webosservicebridge";
const char WebOSServiceBridgeInjectionExtension::kObsoleteName[] =
    "v8/palmservicebridge";

void WebOSServiceBridgeInjectionExtension::Install(
    blink::WebLocalFrame* frame) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);
  v8::Local<v8::Object> global = context->Global();

  v8::Local<v8::FunctionTemplate> templ = gin::CreateConstructorTemplate(
      isolate,
      base::Bind(
          &WebOSServiceBridgeInjection::WebOSServiceBridgeConstructorCallback));
  global->Set(gin::StringToSymbol(isolate, "webOSServiceBridge"),
              templ->GetFunction());

  const std::string extra_objects_js(
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_WEBOSSERVICEBRIDGE_INJECTION_JS));

  v8::Local<v8::Script> local_script;
  v8::MaybeLocal<v8::Script> script = v8::Script::Compile(
      context, gin::StringToV8(isolate, extra_objects_js.c_str()));
  if (script.ToLocal(&local_script))
    local_script->Run(context);
}

// static
void WebOSServiceBridgeInjection::WebOSServiceBridgeConstructorCallback(
    gin::Arguments* args) {
  if (!args->IsConstructCall()) {
    args->isolate()->ThrowException(v8::Exception::Error(
        gin::StringToV8(args->isolate(), kMethodInvocationAsConstructorOnly)));
    return;
  }

  v8::Isolate* isolate = args->isolate();
  v8::HandleScope handle_scope(isolate);
  gin::Handle<WebOSServiceBridgeInjection> wrapper =
      gin::CreateHandle(isolate, new WebOSServiceBridgeInjection());
  if (!wrapper.IsEmpty())
    args->Return(wrapper.ToV8());
}

void WebOSServiceBridgeInjectionExtension::Uninstall(
    blink::WebLocalFrame* frame) {
  const std::string extra_objects_js(
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_WEBOSSERVICEBRIDGE_ROLLBACK_JS));

  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);
  v8::Local<v8::Script> local_script;
  v8::MaybeLocal<v8::Script> script = v8::Script::Compile(
      context, gin::StringToV8(isolate, extra_objects_js.c_str()));

  if (script.ToLocal(&local_script))
    local_script->Run(context);
}

}  // namespace injections
