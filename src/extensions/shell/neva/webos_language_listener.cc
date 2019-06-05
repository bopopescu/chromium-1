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

#include "extensions/shell/neva/webos_language_listener.h"

#include "base/json/json_reader.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/renderer_preferences.h"

namespace {

const char kIdentifier[] = "com.webos.app.neva.browser";
const char kGetLanguageMethod[] =
    "palm://com.webos.settingsservice/getSystemSettings";
const char kGetLanguageRequest[] =
    R"JSON({"keys":["localeInfo"], "subscribe":true})JSON";

std::initializer_list<base::StringPiece> kUILanguagePath =
    {"settings", "localeInfo", "locales", "UI"};

}  // namepsace

namespace webos {

LanguageListener::LanguageListener(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  lsm_ = LunaServiceManager::GetManager(kIdentifier);
  if (lsm_)
    lsm_->Call(kGetLanguageMethod, kGetLanguageRequest, this);
}

LanguageListener::~LanguageListener() {
  if (lsm_)
    lsm_->Cancel(this);
}

void LanguageListener::ServiceResponse(const char* body) {
  content::WebContents* contents = web_contents();
  if (!contents)
    return;

  std::unique_ptr<base::Value> root(base::JSONReader::Read(body));
  if (!root.get())
    return;

  const base::Value* language = root->FindPath(kUILanguagePath);
  if (!language)
    return;

  auto* rendererPrefs(contents->GetMutableRendererPrefs());
  if (!rendererPrefs->accept_languages.compare(language->GetString()))
    return;

  rendererPrefs->accept_languages = language->GetString();

  content::RenderViewHost* rvh = contents->GetRenderViewHost();
  if (!rvh)
    return;
  rvh->SyncRendererPrefs();
}

}  // namespace webos