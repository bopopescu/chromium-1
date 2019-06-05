// Copyright 2016-2018 LG Electronics, Inc.
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

#ifndef NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_CONTENT_BROWSER_CLIENT_H_
#define NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_CONTENT_BROWSER_CLIENT_H_

#include "content/public/browser/content_browser_client.h"
#include "neva/app_runtime/browser/app_runtime_browser_main_parts.h"
#include "services/service_manager/public/cpp/binder_registry.h"

#if defined(USE_NEVA_EXTENSIONS)
namespace extensions {
class DefaultShellBrowserMainDelegate;
class Extension;
}  // namespace extensions
#endif

namespace net {
class NetworkDelegate;
}  // namespace net

namespace app_runtime {

class AppRuntimeBrowserMainExtraParts;
class AppRuntimeQuotaPermissionDelegate;
class URLRequestContextFactory;

class AppRuntimeContentBrowserClient : public content::ContentBrowserClient {
 public:
  explicit AppRuntimeContentBrowserClient(
      net::NetworkDelegate* delegate,
      AppRuntimeQuotaPermissionDelegate* quota_permission_delegate);
  ~AppRuntimeContentBrowserClient() override;

  void SetBrowserExtraParts(
      AppRuntimeBrowserMainExtraParts* browser_extra_parts);

  // content::ContentBrowserClient implementations.
  content::BrowserMainParts* CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) override;

  void AllowCertificateError(
      content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      content::ResourceType resource_type,
      bool strict_enforcement,
      bool expired_previous_decision,
      const base::Callback<void(content::CertificateRequestResultType)>&
          callback) override;

  void HandleServiceRequest(
      const std::string& service_name,
      service_manager::mojom::ServiceRequest request) override;

  content::WebContentsViewDelegate* GetWebContentsViewDelegate(
      content::WebContents* web_contents) override;

  content::DevToolsManagerDelegate* GetDevToolsManagerDelegate() override;

  bool ShouldEnableStrictSiteIsolation() override;

  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;

  void OverrideWebkitPrefs(content::RenderViewHost* render_view_host,
                           content::WebPreferences* prefs) override;
  content::QuotaPermissionContext* CreateQuotaPermissionContext() override;
  void GetQuotaSettings(
      content::BrowserContext* context,
      content::StoragePartition* partition,
      storage::OptionalQuotaSettingsCallback callback) override;

  void GetAdditionalAllowedSchemesForFileSystem(
      std::vector<std::string>* additional_schemes) override;

  void NavigationRequestStarted(
      int frame_tree_node_id,
      const GURL& url,
      std::unique_ptr<net::HttpRequestHeaders>* extra_headers,
      int* extra_load_flags) override;

  AppRuntimeBrowserMainParts* GetMainParts() { return main_parts_; }

  void SetDoNotTrack(bool dnt) { do_not_track_ = dnt; }
  bool DoNotTrack() { return do_not_track_; }

#if defined(ENABLE_PLUGINS)
  bool PluginLoaded() const { return plugin_loaded_; }
  void SetPluginLoaded(bool loaded) { plugin_loaded_ = loaded; }
#endif
#if defined(USE_NEVA_EXTENSIONS)
  void RenderProcessWillLaunch(
      content::RenderProcessHost* host,
      service_manager::mojom::ServiceRequest* service_request) override;
  void LoadExtensions(content::BrowserContext* browser_context);
#endif

  void SetV8SnapshotPath(int child_process_id, const std::string& path);
  void SetV8ExtraFlags(int child_process_id, const std::string& flags);

 private:
  class MainURLRequestContextGetter;

  AppRuntimeBrowserMainExtraParts* browser_extra_parts_ = nullptr;
  std::unique_ptr<URLRequestContextFactory> url_request_context_factory_;
  AppRuntimeBrowserMainParts* main_parts_ = nullptr;

  AppRuntimeQuotaPermissionDelegate* quota_permission_delegate_ = nullptr;
  bool do_not_track_ = false;

#if defined(ENABLE_PLUGINS)
  bool plugin_loaded_ = false;
#endif
#if defined(USE_NEVA_EXTENSIONS)
  extensions::DefaultShellBrowserMainDelegate* shell_browser_main_delegate_ =
      nullptr;
#endif

  std::map<int, std::string> v8_snapshot_pathes_;
  std::map<int, std::string> v8_extra_flags_;
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_CONTENT_BROWSER_CLIENT_H_
