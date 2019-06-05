// Copyright 2015-2019 LG Electronics, Inc.
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

#include "neva/app_runtime/webapp_injection_manager.h"

#include "content/public/browser/render_view_host.h"
#include "neva/injection/browser_control/browser_control_injection.h"
#include "neva/injection/memorymanager/memorymanager_injection.h"
#include "neva/injection/sample/sample_injection.h"
#include "neva/injection/webossystem/webossystem_injection.h"
#include "neva/neva_chromium/content/common/injection_messages.h"

namespace app_runtime {
namespace {

std::set<std::string> allowed_injections = {
#if defined(OS_WEBOS)
  std::string(injections::WebOSSystemInjectionExtension::kInjectionName),
  std::string(injections::WebOSSystemInjectionExtension::kObsoleteName),
#endif
#if defined(ENABLE_SAMPLE_WEBAPI)
  std::string(injections::SampleInjectionExtension::kInjectionName),
#endif
#if defined(ENABLE_BROWSER_CONTROL_WEBAPI)
  std::string(injections::BrowserControlInjectionExtension::kInjectionName),
#endif
#if defined(ENABLE_MEMORYMANAGER_WEBAPI)
  std::string(injections::MemoryManagerInjectionExtension::kInjectionName),
#endif
};

}  // namespace

WebAppInjectionManager::WebAppInjectionManager() {}

WebAppInjectionManager::~WebAppInjectionManager() {}

void WebAppInjectionManager::RequestLoadExtension(
    content::RenderViewHost* render_view_host,
    const std::string& injection_name) {
  if (render_view_host && (allowed_injections.count(injection_name) > 0) &&
      loaded_injections_.find(injection_name) == loaded_injections_.end()) {
    loaded_injections_.insert(injection_name);
    render_view_host->Send(
        new InjectionMsg_LoadExtension(render_view_host->GetRoutingID(),
                                       injection_name));
  }
}

void WebAppInjectionManager::RequestReloadExtensions(
    content::RenderViewHost* render_view_host) {
  if (!render_view_host)
    return;

  for (const auto& injection_name : loaded_injections_) {
    render_view_host->Send(
        new InjectionMsg_LoadExtension(render_view_host->GetRoutingID(),
                                       injection_name));
  }
}

void WebAppInjectionManager::RequestClearExtensions(
    content::RenderViewHost* render_view_host) {
  if (render_view_host) {
    loaded_injections_.clear();
    render_view_host->Send(
        new InjectionMsg_ClearExtensions(render_view_host->GetRoutingID()));
  }
}

}  // namespace app_runtime
