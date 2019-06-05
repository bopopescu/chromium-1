// Copyright 2016-2019 LG Electronics, Inc.
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
#include "injection/common/renderer/injection_observer.h"

#include "content/renderer/render_view_impl.h"
#include "injection/common/public/renderer/injection_install.h"
#include "neva_chromium/content/common/injection_messages.h"

//  If some WEBAPI is enabled include it's headers
#if defined(ENABLE_BROWSER_CONTROL_WEBAPI)
#include "injection/browser_control/browser_control_injection.h"
#endif

#if defined(ENABLE_MEMORYMANAGER_WEBAPI)
#include "injection/memorymanager/memorymanager_injection.h"
#endif

#if defined(ENABLE_SAMPLE_WEBAPI)
#include "injection/sample/sample_injection.h"
#endif

namespace content {

InjectionObserver::InjectionObserver(
    RenderViewImpl* render_view,
    const std::vector<std::string>& preloaded_injections)
    : RenderViewObserver(render_view) {
  // Loads extension for injection
  for (const auto& name : preloaded_injections)
    OnLoadExtension(name);
}

InjectionObserver::~InjectionObserver() {}

bool InjectionObserver::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(InjectionObserver, message)
    IPC_MESSAGE_HANDLER(InjectionMsg_LoadExtension, OnLoadExtension)
    IPC_MESSAGE_HANDLER(InjectionMsg_ClearExtensions, OnClearExtensions)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void InjectionObserver::OnDestruct() {
  delete this;
}

void InjectionObserver::OnLoadExtension(const std::string& name) {
  using namespace injections;
  if (injections_.insert(name).second) {
    InjectionContext context;
    if (injections::GetInjectionInstallAPI(name, &context.api))
      injections_contexts_.push_back(context);
  }
}

void InjectionObserver::OnClearExtensions() {
  for (auto rit = injections_contexts_.crbegin();
       rit != injections_contexts_.crend(); ++rit) {
    if (rit->frame)
      (*rit->api.uninstall_func)(rit->frame);
  }
  injections_.clear();
  injections_contexts_.clear();
}

void InjectionObserver::DidClearWindowObject(blink::WebLocalFrame* frame) {
  for (auto& context : injections_contexts_) {
    context.frame = frame;
    (*context.api.install_func)(frame);
  }
}

}  // namespace content
