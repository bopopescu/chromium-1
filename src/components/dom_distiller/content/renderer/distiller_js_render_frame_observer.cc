// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/content/renderer/distiller_js_render_frame_observer.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "components/dom_distiller/content/common/distiller_page_notifier_service.mojom.h"
#include "components/dom_distiller/content/renderer/distiller_page_notifier_service_impl.h"
#include "content/public/renderer/render_frame.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "v8/include/v8.h"

namespace dom_distiller {

DistillerJsRenderFrameObserver::DistillerJsRenderFrameObserver(
    content::RenderFrame* render_frame,
    const int distiller_isolated_world_id,
    service_manager::BinderRegistry* registry)
    : RenderFrameObserver(render_frame),
      distiller_isolated_world_id_(distiller_isolated_world_id),
      is_distiller_page_(false),
      weak_factory_(this) {
  registry->AddInterface(base::Bind(
      &DistillerJsRenderFrameObserver::CreateDistillerPageNotifierService,
      weak_factory_.GetWeakPtr()));
}

DistillerJsRenderFrameObserver::~DistillerJsRenderFrameObserver() {}

void DistillerJsRenderFrameObserver::DidStartProvisionalLoad(
    blink::WebDocumentLoader* document_loader,
    bool is_content_initiated) {
  load_active_ = true;
}

void DistillerJsRenderFrameObserver::DidFinishLoad() {
  // If no message about the distilled page was received at this point, there
  // will not be one; stop binding requests for
  // mojom::DistillerPageNotifierService.
  load_active_ = false;
}

void DistillerJsRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int world_id) {
  if (world_id != distiller_isolated_world_id_ || !is_distiller_page_) {
    return;
  }

  native_javascript_handle_.reset(
      new DistillerNativeJavaScript(render_frame()));
  native_javascript_handle_->AddJavaScriptObjectToFrame(context);
}

void DistillerJsRenderFrameObserver::CreateDistillerPageNotifierService(
    mojom::DistillerPageNotifierServiceRequest request) {
  if (!load_active_)
    return;
  mojo::MakeStrongBinding(
      std::make_unique<DistillerPageNotifierServiceImpl>(this),
      std::move(request));
}

void DistillerJsRenderFrameObserver::SetIsDistillerPage() {
  is_distiller_page_ = true;
}

void DistillerJsRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace dom_distiller
