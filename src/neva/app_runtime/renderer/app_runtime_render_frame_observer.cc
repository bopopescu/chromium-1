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

#include "neva/app_runtime/renderer/app_runtime_render_frame_observer.h"

#include "base/memory/memory_pressure_listener.h"
#include "content/public/common/web_preferences.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_settings.h"
#include "third_party/blink/public/web/web_view.h"
#include "ui/base/resource/resource_bundle.h"

namespace app_runtime {

AppRuntimeRenderFrameObserver::AppRuntimeRenderFrameObserver(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame),
      dom_suspended_(false),
      binding_(this) {
  render_frame->GetAssociatedInterfaceRegistry()->AddInterface(base::Bind(
      &AppRuntimeRenderFrameObserver::BindRequest, base::Unretained(this)));
}

AppRuntimeRenderFrameObserver::~AppRuntimeRenderFrameObserver() = default;

void AppRuntimeRenderFrameObserver::BindRequest(
    mojom::AppRuntimeWebViewClientAssociatedRequest request) {
  binding_.Bind(std::move(request));
}

void AppRuntimeRenderFrameObserver::OnDestruct() {
}

bool AppRuntimeRenderFrameObserver::OnMessageReceived(
    const IPC::Message& message) {
  return false;
}

void AppRuntimeRenderFrameObserver::SetBackgroundColor(int32_t r,
                                                       int32_t g,
                                                       int32_t b,
                                                       int32_t a) {
  SkColor color = SkColorSetARGB(a, r, g, b);
  render_frame()->GetRenderView()->GetWebFrameWidget()
      ->SetBaseBackgroundColor(color);
}

void AppRuntimeRenderFrameObserver::SuspendDOM() {
  if (dom_suspended_)
    return;
  dom_suspended_ = true;

  render_frame()->GetRenderView()->GetWebView()->WillEnterModalLoop();
}

void AppRuntimeRenderFrameObserver::ResumeDOM() {
  if (!dom_suspended_)
    return;
  dom_suspended_ = false;

  render_frame()->GetRenderView()->GetWebView()->DidExitModalLoop();
}

void AppRuntimeRenderFrameObserver::ResetStateToMarkNextPaintForContainer() {
  render_frame()->ResetStateToMarkNextPaintForContainer();
}

void AppRuntimeRenderFrameObserver::SetVisibilityState(
    blink::mojom::PageVisibilityState visibility_state) {
  render_frame()->GetRenderView()->GetWebView()->SetVisibilityState(
      visibility_state,
      visibility_state == blink::mojom::PageVisibilityState::kLaunching);
}

void AppRuntimeRenderFrameObserver::ChangeLocale(const std::string& locale) {
  // Set LANGUAGE environment variable to required locale, so glib could return
  // it throught function g_get_language_names.
  // Correct way will be to set locale by std::setlocale function, but it
  // works only if corresponding locale is installed in linux, that is not
  // always the case.
  // This way works as glib does not check if locale in LANGUAGE variable is
  // actually installed in linux.
  setenv("LANGUAGE", locale.c_str(), 1);

  ui::ResourceBundle::GetSharedInstance().ReloadLocaleResources(locale);
  ui::ResourceBundle::GetSharedInstance().ReloadFonts();
}

void AppRuntimeRenderFrameObserver::SetNetworkQuietTimeout(double timeout) {
  render_frame()->GetRenderView()->GetWebView()->GetSettings()->
      SetNetworkQuietTimeout(timeout);
}

void AppRuntimeRenderFrameObserver::SetDisallowScrollbarsInMainFrame(
    bool disallow) {
  render_frame()->GetRenderView()->GetWebView()->GetSettings()->
      SetDisallowScrollbarsInMainFrame(disallow);
}

void AppRuntimeRenderFrameObserver::DidClearWindowObject() {
  mojom::AppRuntimeWebViewHostAssociatedPtr interface;
  render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(
      &interface);
  interface->DidClearWindowObject();
}

void AppRuntimeRenderFrameObserver::GrantLoadLocalResources() {
  render_frame()->GetWebFrame()->GetDocument().GrantLoadLocalResources();
}

void AppRuntimeRenderFrameObserver::OnNotifyMemoryPressure(int level) {
  switch (level) {
    case 1:
      base::MemoryPressureListener::NotifyMemoryPressure(
          base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE);
      break;
    case 2:
      base::MemoryPressureListener::NotifyMemoryPressure(
          base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL);
      break;
    default:
      break;
  }
}

}  // namespace app_runtime
