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

#ifndef EXTENSIONS_SHELL_NEVA_WEBOS_REGISTER_APP_H_
#define EXTENSIONS_SHELL_NEVA_WEBOS_REGISTER_APP_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "neva/injection/common/webos/luna_service_mgr.h"

namespace webos {

class RegisterApp : public content::WebContentsUserData<RegisterApp>,
                    public content::WebContentsObserver,
                    private LunaServiceManagerListener {
 public:
  ~RegisterApp() override;

  // LunaServiceManagerListener implementations
  void ServiceResponse(const char* body) override;

 private:
  friend class content::WebContentsUserData<RegisterApp>;

  explicit RegisterApp(content::WebContents* web_contents);

  std::shared_ptr<LunaServiceManager> lsm_;
};

}  // namespace webos

#endif  // EXTENSIONS_SHELL_NEVA_WEBOS_REGISTER_APP_H_
