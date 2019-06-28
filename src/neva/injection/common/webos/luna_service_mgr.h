// Copyright 2014-2018 LG Electronics, Inc.
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

#ifndef NEVA_INJECTION_COMMON_WEBOS_LUNA_SERVICE_MGR_H_
#define NEVA_INJECTION_COMMON_WEBOS_LUNA_SERVICE_MGR_H_

#include <lunaservice.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "base/macros.h"

struct LunaServiceManagerListener {
  LunaServiceManagerListener()
      : listener_token_(LSMESSAGE_TOKEN_INVALID) { }
  virtual ~LunaServiceManagerListener() { }
  virtual void ServiceResponse(const char* body) = 0;

  LSMessageToken GetListenerToken() { return listener_token_; }
  void SetListenerToken(LSMessageToken token) { listener_token_ = token; }
 private:
  LSMessageToken listener_token_;
};

class LunaServiceManager {
 public:
  static std::shared_ptr<LunaServiceManager> GetManager(const std::string&);
  ~LunaServiceManager();

  unsigned long Call(const char* uri,
                     const char* payload,
                     LunaServiceManagerListener*);
  unsigned long CallFromApplication(const char* uri,
                                    const char* payload,
                                    const char* app_id,
                                    LunaServiceManagerListener*);
  void Cancel(LunaServiceManagerListener*);
  bool Initialized() { return initialized_; }

 private:
  LunaServiceManager(const std::string&);
  void Init();

  LSHandle* sh_;
  std::string identifier_;
  bool initialized_;
  static std::mutex storage_lock_;
  static std::unordered_map<std::string, std::weak_ptr<LunaServiceManager>> storage_;

  DISALLOW_COPY_AND_ASSIGN(LunaServiceManager);
};

#endif  // NEVA_INJECTION_COMMON_WEBOS_LUNA_SERVICE_MGR_H_
