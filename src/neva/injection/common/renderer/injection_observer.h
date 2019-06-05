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

#ifndef NEVA_INJECTION_COMMON_RENDERER_INJECTION_OBSERVER_H_
#define NEVA_INJECTION_COMMON_RENDERER_INJECTION_OBSERVER_H_

#include <list>
#include <set>
#include <string>

#include "content/public/renderer/render_view_observer.h"
#include "neva/injection/common/public/renderer/injection_install.h"

namespace content {

class InjectionObserver : public RenderViewObserver {
 public:
  explicit InjectionObserver(RenderViewImpl* render_view,
                             const std::vector<std::string>& preloaded_injections);
  ~InjectionObserver() override;

  // RenderViewObserver overrides:
  bool OnMessageReceived(const IPC::Message& message) override;
  void DidClearWindowObject(blink::WebLocalFrame* frame) override;
  void OnDestruct() override;

 private:
  // IPC Message handlers:
  void OnLoadExtension(const std::string& name);
  void OnClearExtensions();

  std::set<std::string> injections_;

  struct InjectionContext {
    injections::InstallAPI api;
    blink::WebLocalFrame* frame = nullptr;
  };

  std::list<InjectionContext> injections_contexts_;
  DISALLOW_COPY_AND_ASSIGN(InjectionObserver);
};

}  // namespace content

#endif  // NEVA_INJECTION_COMMON_RENDERER_INJECTION_OBSERVER_H_
