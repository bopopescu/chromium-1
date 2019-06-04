// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/renderer/web_test/web_test_content_renderer_client.h"

#include <string>
#include <utility>

#include "base/callback.h"
#include "base/command_line.h"
#include "base/debug/debugger.h"
#include "build/build_config.h"
#include "components/web_cache/renderer/web_cache_impl.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/content_switches.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_view.h"
#include "content/public/test/layouttest_support.h"
#include "content/shell/common/layout_test/layout_test_switches.h"
#include "content/shell/common/shell_switches.h"
#include "content/shell/renderer/shell_render_view_observer.h"
#include "content/shell/renderer/web_test/blink_test_helpers.h"
#include "content/shell/renderer/web_test/blink_test_runner.h"
#include "content/shell/renderer/web_test/test_media_stream_renderer_factory.h"
#include "content/shell/renderer/web_test/test_websocket_handshake_throttle_provider.h"
#include "content/shell/renderer/web_test/web_test_render_frame_observer.h"
#include "content/shell/renderer/web_test/web_test_render_thread_observer.h"
#include "content/shell/test_runner/web_frame_test_proxy.h"
#include "content/shell/test_runner/web_test_interfaces.h"
#include "content/shell/test_runner/web_test_runner.h"
#include "content/shell/test_runner/web_view_test_proxy.h"
#include "media/base/audio_latency.h"
#include "media/base/mime_util.h"
#include "media/media_buildflags.h"
#include "third_party/blink/public/platform/modules/webmidi/web_midi_accessor.h"
#include "third_party/blink/public/platform/web_audio_latency_hint.h"
#include "third_party/blink/public/platform/web_media_stream_center.h"
#include "third_party/blink/public/platform/web_rtc_peer_connection_handler.h"
#include "third_party/blink/public/platform/web_runtime_features.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_plugin_params.h"
#include "third_party/blink/public/web/web_testing_support.h"
#include "third_party/blink/public/web/web_view.h"
#include "ui/gfx/icc_profile.h"
#include "v8/include/v8.h"

using blink::WebAudioDevice;
using blink::WebFrame;
using blink::WebLocalFrame;
using blink::WebMediaStreamCenter;
using blink::WebMIDIAccessor;
using blink::WebMIDIAccessorClient;
using blink::WebPlugin;
using blink::WebPluginParams;
using blink::WebRTCPeerConnectionHandler;
using blink::WebRTCPeerConnectionHandlerClient;
using blink::WebThemeEngine;

namespace content {

WebTestContentRendererClient::WebTestContentRendererClient() {
  EnableWebTestProxyCreation();
  SetWorkerRewriteURLFunction(RewriteLayoutTestsURL);
}

WebTestContentRendererClient::~WebTestContentRendererClient() {}

void WebTestContentRendererClient::RenderThreadStarted() {
  ShellContentRendererClient::RenderThreadStarted();
  shell_observer_.reset(new WebTestRenderThreadObserver());
}

void WebTestContentRendererClient::RenderFrameCreated(
    RenderFrame* render_frame) {
  test_runner::WebFrameTestProxyBase* frame_proxy =
      GetWebFrameTestProxyBase(render_frame);
  frame_proxy->set_web_frame(render_frame->GetWebFrame());
  new WebTestRenderFrameObserver(render_frame);
}

void WebTestContentRendererClient::RenderViewCreated(RenderView* render_view) {
  new ShellRenderViewObserver(render_view);

  test_runner::WebViewTestProxyBase* proxy =
      GetWebViewTestProxyBase(render_view);
  proxy->set_web_view(render_view->GetWebView());
  // TODO(lfg): We should fix the TestProxy to track the WebWidgets on every
  // local root in WebFrameTestProxy instead of having only the WebWidget for
  // the main frame in WebViewTestProxy.
  proxy->web_widget_test_proxy_base()->set_web_widget(
      render_view->GetWebView()->MainFrameWidget());
  proxy->Reset();

  BlinkTestRunner* test_runner = BlinkTestRunner::Get(render_view);
  test_runner->Reset(false /* for_new_test */);
}

std::unique_ptr<WebMIDIAccessor>
WebTestContentRendererClient::OverrideCreateMIDIAccessor(
    WebMIDIAccessorClient* client) {
  test_runner::WebTestInterfaces* interfaces =
      WebTestRenderThreadObserver::GetInstance()->test_interfaces();
  return interfaces->CreateMIDIAccessor(client);
}

WebThemeEngine* WebTestContentRendererClient::OverrideThemeEngine() {
  return WebTestRenderThreadObserver::GetInstance()
      ->test_interfaces()
      ->ThemeEngine();
}

std::unique_ptr<MediaStreamRendererFactory>
WebTestContentRendererClient::CreateMediaStreamRendererFactory() {
  return std::unique_ptr<MediaStreamRendererFactory>(
      new TestMediaStreamRendererFactory());
}

std::unique_ptr<content::WebSocketHandshakeThrottleProvider>
WebTestContentRendererClient::CreateWebSocketHandshakeThrottleProvider() {
  return std::make_unique<TestWebSocketHandshakeThrottleProvider>();
}

void WebTestContentRendererClient::DidInitializeWorkerContextOnWorkerThread(
    v8::Local<v8::Context> context) {
  blink::WebTestingSupport::InjectInternalsObject(context);
}

void WebTestContentRendererClient::
    SetRuntimeFeaturesDefaultsBeforeBlinkInitialization() {
  // We always expose GC to layout tests.
  std::string flags("--expose-gc");
  v8::V8::SetFlagsFromString(flags.c_str(), static_cast<int>(flags.size()));
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kStableReleaseMode)) {
    blink::WebRuntimeFeatures::EnableTestOnlyFeatures(true);
  }
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableFontAntialiasing)) {
    blink::SetFontAntialiasingEnabledForTest(true);
  }
}

bool WebTestContentRendererClient::IsIdleMediaSuspendEnabled() {
  // Disable idle media suspend to avoid layout tests getting into accidentally
  // bad states if they take too long to run.
  return false;
}

bool WebTestContentRendererClient::SuppressLegacyTLSVersionConsoleMessage() {
#if defined(OS_WIN) || defined(OS_MACOSX)
  // Blink uses an outdated test server on Windows and older versions of macOS.
  // Until those are fixed, suppress the warning. See https://crbug.com/747666
  // and https://crbug.com/905831.
  return true;
#else
  return false;
#endif
}

}  // namespace content
