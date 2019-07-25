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

#include "neva/injection/common/webos/luna_service_mgr.h"

#include <glib.h>
#include <lunaservice.h>
#include <random>
#include <stdlib.h>
#include <unistd.h>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/values.h"

namespace {

const char random_char() {
  const std::string random_src =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  std::mt19937 rd{std::random_device{}()};
  std::uniform_int_distribution<int> mrand{0, random_src.size() - 1};
  return random_src[mrand(rd)];
}

std::string unique_id_with_suffix(std::string& id) {
  std::string unique_id = id + "-%%%%%%%%";
  for (auto& ch : unique_id) {
    if (ch == '%')
      ch = random_char();
  }
  return unique_id;
}

}  // namespace

// static
std::mutex LunaServiceManager::storage_lock_;
std::unordered_map<std::string, std::weak_ptr<LunaServiceManager>>
    LunaServiceManager::storage_;

// static
static bool message_filter(LSHandle* sh, LSMessage* reply, void* ctx) {
  const char* payload = LSMessageGetPayload(reply);

  LunaServiceManagerListener* listener =
    static_cast<LunaServiceManagerListener*>(ctx);

  if (listener) {
    listener->ServiceResponse(payload);
    return true;
  }

  return false;
}

class LSErrorSafe: public LSError {
 public:
  LSErrorSafe() {
    LSErrorInit(this);
  };
  ~LSErrorSafe() {
    LSErrorFree(this);
  }
};

LunaServiceManager::LunaServiceManager(const std::string& id)
    : sh_(NULL),
      initialized_(false),
      identifier_(id) {
  Init();
}

LunaServiceManager::~LunaServiceManager() {
  if (sh_) {
    bool retVal;
    LSErrorSafe lserror;
    retVal = LSUnregister(sh_, &lserror);
    if (!retVal) {
      LOG(ERROR) << "LSUnregisterPalmService ERROR "
                 << lserror.error_code << ": " << lserror.message << " ("
                 << lserror.func << " @ " << lserror.file << ":"
                 << lserror.line << ")";
    }
  }
}

std::shared_ptr<LunaServiceManager> LunaServiceManager::GetManager(
    const std::string& identifier) {
  std::lock_guard<std::mutex> lock(storage_lock_);
  std::shared_ptr<LunaServiceManager> manager;
  auto iter = storage_.find(identifier);
  if (iter != storage_.end())
    manager = iter->second.lock();
  if (!manager) {
    manager.reset(new LunaServiceManager(identifier));
    if (manager->Initialized())
      storage_[identifier] = manager;
    else
      manager.reset();
  }
  return manager;
}

void log_error(LSErrorSafe& lserror) {
  LOG(ERROR) << "Cannot initialize LunaServiceManager ERROR "
             << lserror.error_code << ": " << lserror.message << " ("
             << lserror.func << " @ " << lserror.file << ":"
             << lserror.line << ")";
}

void LunaServiceManager::Init() {
  bool init;
  LSErrorSafe lserror;

  if (initialized_)
    return;

  bool is_browser = identifier_.find("com.webos.app.enactbrowser") == 0;
  bool is_phone = identifier_.find("com.palm.app.phone") == 0;

  std::string identifier = unique_id_with_suffix(identifier_);
  if (is_browser)
    init = LSRegister(identifier_.c_str(), &sh_, &lserror);
  else
    init = LSRegisterApplicationService(identifier.c_str(),
                                        identifier_.c_str(),
                                        &sh_, &lserror);
  if (!init) {
    log_error(lserror);
    return;
  }

  init = LSGmainContextAttach(sh_, g_main_context_default(), &lserror);
  if (!init) {
    log_error(lserror);
    return;
  }

  init = LSGmainSetPriority(sh_,
                            is_phone ? G_PRIORITY_HIGH : G_PRIORITY_DEFAULT,
                            &lserror);
  if (!init) {
    log_error(lserror);
    return;
  }

  initialized_ = true;
  return;
}

unsigned long LunaServiceManager::Call(
    const char* uri,
    const char* payload,
    LunaServiceManagerListener* inListener) {
  return LunaServiceManager::CallFromApplication(uri, payload, "", inListener);
}

unsigned long LunaServiceManager::CallFromApplication(
    const char* uri,
    const char* payload,
    const char* app_id,
    LunaServiceManagerListener* inListener) {
  bool retVal;
  LSErrorSafe lserror;
  LSMessageToken token = 0;

  bool subscription = false;

  std::unique_ptr<base::Value> json = base::JSONReader::Read(payload);
  if (json && json->is_dict()) {
    const base::Value* subscribe_value =
        json->FindKeyOfType("subscribe", base::Value::Type::BOOLEAN);
    subscription = subscribe_value && subscribe_value->GetBool();

    if (!subscription) {
      const base::Value* watch_value =
          json->FindKeyOfType("watch", base::Value::Type::BOOLEAN);
      subscription = watch_value && watch_value->GetBool();
    }
  }

  if (subscription)
    retVal = LSCallFromApplication(sh_, uri, payload, app_id, message_filter,
                                   inListener, &token, &lserror);
  else
    retVal =
        LSCallFromApplicationOneReply(sh_, uri, payload, app_id, message_filter,
                                      inListener, &token, &lserror);
  if (retVal)
    inListener->SetListenerToken(token);
  else
    token = 0;

  return token;
}

void LunaServiceManager::Cancel(LunaServiceManagerListener* inListener) {
  bool retVal;
  LSErrorSafe lserror;

  if (!inListener || !inListener->GetListenerToken())
    return;

  if (!LSCallCancel(sh_, inListener->GetListenerToken(), &lserror)) {
    LOG(ERROR) << "LSCallCancel ERROR "
               << lserror.error_code << ": " << lserror.message << " ("
               << lserror.func << " @ " << lserror.file << ":"
               << lserror.line << ")";
  }

  inListener->SetListenerToken(0);
}
