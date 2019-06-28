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

#include "extensions/shell/neva/webos_register_app.h"

#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "content/public/browser/web_contents.h"
#include "extensions/common/switches.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"

namespace {

const char kMessage[] = "message";
const char kRegisterAppMethod[] =
    "palm://com.webos.applicationManager/registerNativeApp";
const char kRegisterAppRequest[] =
    R"JSON({"subscribe":true})JSON";

}  // namepsace

namespace webos {

RegisterApp::RegisterApp(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  std::string app_id;
  base::CommandLine* cmd = base::CommandLine::ForCurrentProcess();
  if (!cmd->HasSwitch(extensions::switches::kWebOSAppId)) {
    LOG(ERROR) << __func__ << "(): no webOS-application identifier specified";
    return;
  } else {
    app_id = cmd->GetSwitchValueASCII(extensions::switches::kWebOSAppId);
  }
  lsm_ = LunaServiceManager::GetManager(app_id);
  if (lsm_) {
    lsm_->CallFromApplication(kRegisterAppMethod, kRegisterAppRequest,
                              app_id.c_str(), this);
  }
}

RegisterApp::~RegisterApp() {
  if (lsm_)
    lsm_->Cancel(this);
}

void RegisterApp::ServiceResponse(const char* body) {
  content::WebContents* contents = web_contents();
  if (!contents)
    return;

  std::unique_ptr<base::Value> root(base::JSONReader::Read(body));
  if (!root.get())
    return;

  base::DictionaryValue* dictionary_value = nullptr;
  if (!root->GetAsDictionary(&dictionary_value))
    return;

  std::string message;
  if (dictionary_value->GetString(kMessage, &message)) {
    if (message == "relaunch") {
      aura::Window* top_window = contents->GetTopLevelNativeWindow();
      if (top_window && top_window->GetHost())
        top_window->GetHost()->ToggleFullscreen();
    }
  }
}

}  // namespace webos
