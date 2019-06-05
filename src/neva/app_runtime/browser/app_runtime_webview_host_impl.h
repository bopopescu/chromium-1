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

#ifndef NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_WEBVIEW_HOST_IMPL_H_
#define NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_WEBVIEW_HOST_IMPL_H_

#include "content/public/browser/web_contents_binding_set.h"
#include "neva/app_runtime/public/mojom/app_runtime_webview.mojom.h"

namespace content {

class WebContents;

}  // namespace content

namespace app_runtime {

class WebViewDelegate;

class AppRuntimeWebViewHostImpl : public mojom::AppRuntimeWebViewHost {
 public:
  AppRuntimeWebViewHostImpl(content::WebContents* web_contents);
  ~AppRuntimeWebViewHostImpl() override;

  void SetDelegate(WebViewDelegate* delegate);

  // mojom::AppRuntimeWebViewHost implementation.
  void DidLoadingEnd() override;
  void DidFirstPaint() override;
  void DidFirstContentfulPaint() override;
  void DidFirstTextPaint() override;
  void DidFirstImagePaint() override;
  void DidFirstMeaningfulPaint() override;
  void DidNonFirstMeaningfulPaint() override;
  void DidClearWindowObject() override;

 private:
  content::WebContentsFrameBindingSet<mojom::AppRuntimeWebViewHost> bindings_;
  WebViewDelegate* webview_delegate_ = nullptr;
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_WEBVIEW_HOST_IMPL_H_
