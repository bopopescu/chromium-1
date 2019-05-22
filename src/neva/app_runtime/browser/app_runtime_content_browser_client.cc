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

#include "neva/app_runtime/browser/app_runtime_content_browser_client.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/neva/base_switches.h"
#include "base/strings/utf_string_conversions.h"
#include "cc/base/switches_neva.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/devtools_manager_delegate.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_neva_switches.h"
#include "content/public/common/content_switches.h"
#include "neva/app_runtime/browser/app_runtime_browser_main_parts.h"
#include "neva/app_runtime/browser/app_runtime_devtools_manager_delegate.h"
#include "neva/app_runtime/browser/app_runtime_quota_permission_context.h"
#include "neva/app_runtime/browser/app_runtime_quota_permission_delegate.h"
#include "neva/app_runtime/browser/app_runtime_web_contents_view_delegate_creator.h"
#include "neva/app_runtime/browser/url_request_context_factory.h"
#include "neva/app_runtime/webview.h"
#include "neva/pal_service/pal_service_factory.h"
#include "neva/pal_service/public/mojom/constants.mojom.h"
#include "services/service_manager/sandbox/switches.h"

#if defined(USE_NEVA_EXTENSIONS)
#include "apps/launcher.h"
#include "base/path_service.h"
#include "extensions/browser/guest_view/extensions_guest_view_message_filter.h"
#include "extensions/common/switches.h"
#include "extensions/shell/browser/shell_extension_system.h"
#include "neva/app_runtime/browser/app_runtime_browser_context_adapter.h"
#endif
namespace app_runtime {

#if defined(USE_NEVA_EXTENSIONS)
namespace {

void LoadAppsFromCommandLine(extensions::ShellExtensionSystem* extension_system,
                             content::BrowserContext* browser_context) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(extensions::switches::kLoadApps)) {
    LOG(ERROR) << "No app specified. Use --" << extensions::switches::kLoadApps
               << " to load and launch an app.";
    return;
  }

  base::CommandLine::StringType path_list =
      command_line->GetSwitchValueNative(extensions::switches::kLoadApps);

  base::StringTokenizerT<base::CommandLine::StringType,
                         base::CommandLine::StringType::const_iterator>
      tokenizer(path_list, FILE_PATH_LITERAL(","));

  const extensions::Extension* launch_app = nullptr;
  while (tokenizer.GetNext()) {
    base::FilePath app_absolute_dir =
        base::MakeAbsoluteFilePath(base::FilePath(tokenizer.token()));

    const extensions::Extension* extension =
        extension_system->LoadApp(app_absolute_dir);
    if (extension && !launch_app)
      launch_app = extension;
  }

  if (launch_app) {
    base::FilePath current_directory;
    base::PathService::Get(base::DIR_CURRENT, &current_directory);
    apps::LaunchPlatformAppWithCommandLine(browser_context, launch_app,
                                           *command_line, current_directory,
                                           extensions::SOURCE_COMMAND_LINE);

    URLPattern pattern(extensions::Extension::kValidHostPermissionSchemes);
    pattern.SetMatchAllURLs(true);
    const_cast<extensions::Extension*>(launch_app)
        ->AddWebExtentPattern(pattern);

  } else {
    LOG(ERROR) << "Could not load any apps.";
  }
  return;
}

}  // namespace
#endif
AppRuntimeContentBrowserClient::AppRuntimeContentBrowserClient(
    net::NetworkDelegate* delegate,
    AppRuntimeQuotaPermissionDelegate* quota_permission_delegate)
    : url_request_context_factory_(new URLRequestContextFactory(delegate)),
      quota_permission_delegate_(quota_permission_delegate) {}

AppRuntimeContentBrowserClient::~AppRuntimeContentBrowserClient() {}

void AppRuntimeContentBrowserClient::SetBrowserExtraParts(
    AppRuntimeBrowserMainExtraParts* browser_extra_parts) {
  browser_extra_parts_ = browser_extra_parts;
}

content::BrowserMainParts*
AppRuntimeContentBrowserClient::CreateBrowserMainParts(
    const content::MainFunctionParams& parameters) {
  main_parts_ =
      new AppRuntimeBrowserMainParts(url_request_context_factory_.get());

  if (browser_extra_parts_)
    main_parts_->AddParts(browser_extra_parts_);

  return main_parts_;
}

content::WebContentsViewDelegate*
AppRuntimeContentBrowserClient::GetWebContentsViewDelegate(
    content::WebContents* web_contents) {
  return CreateAppRuntimeWebContentsViewDelegate(web_contents);
}

void AppRuntimeContentBrowserClient::AllowCertificateError(
    content::WebContents* web_contents,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    content::ResourceType resource_type,
    bool strict_enforcement,
    bool expired_previous_decision,
    const base::Callback<void(content::CertificateRequestResultType)>&
        callback) {
  // HCAP requirements: For SSL Certificate error, follows the policy settings
  if (web_contents && web_contents->GetDelegate()) {
    WebView* webView = static_cast<WebView*>(web_contents->GetDelegate());
    switch (webView->GetSSLCertErrorPolicy()) {
      case SSL_CERT_ERROR_POLICY_IGNORE:
        callback.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_CONTINUE);
        return;
      case SSL_CERT_ERROR_POLICY_DENY:
        callback.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY);
        return;
      default:
        break;
    }
  }

  // A certificate error. The user doesn't really have a context for making the
  // right decision, so block the request hard, without adding info bar that
  // provides possibility to show the insecure content.
  callback.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY);
}

void AppRuntimeContentBrowserClient::HandleServiceRequest(
    const std::string& service_name,
    service_manager::mojom::ServiceRequest request) {
  if (service_name == pal::mojom::kServiceName) {
    service_manager::Service::RunAsyncUntilTermination(
        pal::CreatePalService(std::move(request)));
  }
}

bool AppRuntimeContentBrowserClient::ShouldEnableStrictSiteIsolation() {
  // TODO(neva): Temporarily disabled until we support site isolation.
  return false;
}

bool AppRuntimeContentBrowserClient::ShouldIsolateErrorPage(
    bool /* in_main_frame */) {
  // TODO(neva): Temporarily disabled until we support site isolation.
  return false;
}

void AppRuntimeContentBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int child_process_id) {
  command_line->AppendSwitch(service_manager::switches::kNoSandbox);

  // Append v8 snapshot path if exists
  auto iter = v8_snapshot_pathes_.find(child_process_id);
  if (iter != v8_snapshot_pathes_.end()) {
    command_line->AppendSwitchPath(switches::kV8SnapshotBlobPath,
                                   base::FilePath(iter->second));
    v8_snapshot_pathes_.erase(iter);
  }

  // Append v8 extra flags if exists
  iter = v8_extra_flags_.find(child_process_id);
  if (iter != v8_extra_flags_.end()) {
    std::string js_flags = iter->second;
    // If already has, append it also
    if (command_line->HasSwitch(switches::kJavaScriptFlags)) {
      js_flags.append(" ");
      js_flags.append(
          command_line->GetSwitchValueASCII(switches::kJavaScriptFlags));
    }
    command_line->AppendSwitchASCII(switches::kJavaScriptFlags, js_flags);
    v8_extra_flags_.erase(iter);
  }

  // Append native scroll related flags if native scroll is on by appinfo.json
  auto iter_ns = use_native_scroll_map_.find(child_process_id);
  if (iter_ns != use_native_scroll_map_.end()) {
    bool use_native_scroll = iter_ns->second;
    if (use_native_scroll) {
      // Enables EnableNativeScroll, which is only enabled when there is
      // 'useNativeScroll': true in appinfo.json. If this flag is enabled,
      // Duration of the scroll offset animation is modified.
      if (!command_line->HasSwitch(cc::switches::kEnableWebOSNativeScroll))
        command_line->AppendSwitch(cc::switches::kEnableWebOSNativeScroll);

      // Enables SmoothScrolling, which is mandatory to enable
      // CSSOMSmoothScroll.
      if (!command_line->HasSwitch(switches::kEnableSmoothScrolling))
        command_line->AppendSwitch(switches::kEnableSmoothScrolling);

      // Adds CSSOMSmoothScroll to EnableBlinkFeatures.
      std::string enable_blink_features_flags = "CSSOMSmoothScroll";
      if (command_line->HasSwitch(switches::kEnableBlinkFeatures)) {
        enable_blink_features_flags.append(",");
        enable_blink_features_flags.append(
            command_line->GetSwitchValueASCII(switches::kEnableBlinkFeatures));
      }
      command_line->AppendSwitchASCII(switches::kEnableBlinkFeatures,
                                      enable_blink_features_flags);

      // Enables PreferCompositingToLCDText. If this flag is enabled, Compositor
      // thread handles scrolling and disable LCD-text(AntiAliasing) in the
      // scroll area.
      // See PaintLayerScrollableArea.cpp::layerNeedsCompositingScrolling()
      if (!command_line->HasSwitch(switches::kEnablePreferCompositingToLCDText))
        command_line->AppendSwitch(switches::kEnablePreferCompositingToLCDText);

      // Sets kCustomMouseWheelGestureScrollDeltaOnWebOSNativeScroll.
      // If this value is provided from command line argument, then propagate
      // the value to render process. If not, initialize this flag as default
      // value.
      static const int kDefaultGestureScrollDistanceOnNativeScroll = 180;
      // We should find in browser's switch value.
      if (base::CommandLine::ForCurrentProcess()->HasSwitch(
              cc::switches::
                  kCustomMouseWheelGestureScrollDeltaOnWebOSNativeScroll)) {
        std::string propagated_value(
            base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                cc::switches::
                    kCustomMouseWheelGestureScrollDeltaOnWebOSNativeScroll));
        command_line->AppendSwitchASCII(
            cc::switches::
                kCustomMouseWheelGestureScrollDeltaOnWebOSNativeScroll,
            propagated_value);
      } else
        command_line->AppendSwitchASCII(
            cc::switches::
                kCustomMouseWheelGestureScrollDeltaOnWebOSNativeScroll,
            std::to_string(kDefaultGestureScrollDistanceOnNativeScroll));
    }

    use_native_scroll_map_.erase(iter_ns);
  }
}

void AppRuntimeContentBrowserClient::SetUseNativeScroll(
    int child_process_id,
    bool use_native_scroll) {
  use_native_scroll_map_.insert(
      std::pair<int, bool>(child_process_id, use_native_scroll));
}

content::DevToolsManagerDelegate*
AppRuntimeContentBrowserClient::GetDevToolsManagerDelegate() {
  return new AppRuntimeDevToolsManagerDelegate();
}

void AppRuntimeContentBrowserClient::OverrideWebkitPrefs(
    content::RenderViewHost* render_view_host,
    content::WebPreferences* prefs) {
  if (!render_view_host)
    return;

  RenderViewHostDelegate* delegate = render_view_host->GetDelegate();
  if (delegate)
    delegate->OverrideWebkitPrefs(prefs);
}

QuotaPermissionContext*
AppRuntimeContentBrowserClient::CreateQuotaPermissionContext() {
  return new AppRuntimeQuotaPermissionContext(quota_permission_delegate_);
}

void AppRuntimeContentBrowserClient::GetQuotaSettings(
    content::BrowserContext* context,
    content::StoragePartition* partition,
    storage::OptionalQuotaSettingsCallback callback) {
  storage::GetNominalDynamicSettings(
      partition->GetPath(), context->IsOffTheRecord(), std::move(callback));
}

content::GeneratedCodeCacheSettings
AppRuntimeContentBrowserClient::GetGeneratedCodeCacheSettings(
    content::BrowserContext* context) {
  return content::GeneratedCodeCacheSettings(true, 0, context->GetPath());
}

void AppRuntimeContentBrowserClient::GetAdditionalAllowedSchemesForFileSystem(
    std::vector<std::string>* additional_schemes) {
  ContentBrowserClient::GetAdditionalAllowedSchemesForFileSystem(
      additional_schemes);
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableFileAPIDirectoriesAndSystem))
    additional_schemes->push_back(url::kFileScheme);
}


void AppRuntimeContentBrowserClient::NavigationRequestStarted(
    int frame_tree_node_id,
    const GURL& url,
    std::unique_ptr<net::HttpRequestHeaders>* extra_headers,
    int* extra_load_flags) {
  content::WebContents* web_contents =
      content::WebContents::FromFrameTreeNodeId(frame_tree_node_id);
  auto* rendererPrefs = web_contents->GetMutableRendererPrefs();
  if (!rendererPrefs->accept_languages.empty()) {
    std::string accept_language(
        net::HttpUtil::GenerateAcceptLanguageHeader(
            rendererPrefs->accept_languages));
    if (*extra_headers == nullptr)
      *extra_headers = std::make_unique<net::HttpRequestHeaders>();
    extra_headers->get()->SetHeader(net::HttpRequestHeaders::kAcceptLanguage,
                                    accept_language);
  }
}

void AppRuntimeContentBrowserClient::SetV8SnapshotPath(
    int child_process_id,
    const std::string& path) {
  v8_snapshot_pathes_.insert(
      std::make_pair(child_process_id, path));
}

void AppRuntimeContentBrowserClient::SetV8ExtraFlags(int child_process_id,
                                                     const std::string& flags) {
  v8_extra_flags_.insert(std::make_pair(child_process_id, flags));
}

#if defined(USE_NEVA_EXTENSIONS)
void AppRuntimeContentBrowserClient::RenderProcessWillLaunch(
    content::RenderProcessHost* host,
    service_manager::mojom::ServiceRequest* service_request) {
  using namespace extensions;
  int render_process_id = host->GetID();
  BrowserContext* browser_context =
      main_parts_->GetDefaultBrowserContext()->GetBrowserContext();
  host->AddFilter(
      new ExtensionsGuestViewMessageFilter(render_process_id, browser_context));
}

void AppRuntimeContentBrowserClient::LoadExtensions(
    content::BrowserContext* browser_context) {
  extensions::ShellExtensionSystem* extension_system =
      static_cast<extensions::ShellExtensionSystem*>(
          extensions::ExtensionSystem::Get(browser_context));
  extension_system->FinishInitialization();

  LoadAppsFromCommandLine(extension_system, browser_context);
}
#endif
}  // namespace app_runtime
