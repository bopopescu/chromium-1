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

#include "neva/pal_service/webos/luna/luna_client_impl.h"

#include "base/logging.h"

#include <glib.h>
#include <memory>
#include <stdlib.h>
#include <unistd.h>

namespace pal {
namespace luna {
namespace {

struct Error : LSError {
  Error() {
    LSErrorInit(this);
  }

  ~Error() {
    LSErrorFree(this);
  }
};

void LogError(Error& error) {
    LOG(ERROR) << error.error_code << ": "
               << error.message << " ("
               << error.func << " @ "
               << error.file << ":"
               << error.line << ")";
}

}  // namespace

ClientImpl::ClientImpl(const Params& params)
  : params_(params) {
  Error error;
  bool registered = false;

  if (params.appid.empty()) {
    registered = LSRegisterPubPriv(
        params.name.c_str(),
        &handle_,
        static_cast<int>(params.bus),
        &error);
  } else {
    registered = LSRegisterApplicationService(
        params.name.c_str(),
        params.appid.c_str(),
        &handle_,
        &error);
  }

  if (!registered) {
    LogError(error);
    return;
  }

  context_ = g_main_context_ref(g_main_context_default());
  if (!LSGmainContextAttach(handle_, context_, &error)) {
    LogError(error);
    if (!LSUnregister(handle_, &error))
      LogError(error);
    g_main_context_unref(context_);
    context_ = nullptr;
    handle_ = nullptr;
  }
}

ClientImpl::~ClientImpl() {
  CancelAllSubscriptions();
  CancelWaitingCalls();

  Error error;
  if (handle_) {
    if (!LSUnregister(handle_, &error))
      LogError(error);
  }
  if (context_)
    g_main_context_unref(context_);
}

bool ClientImpl::IsInitialized() const {
  return handle_ != nullptr;
}

Bus ClientImpl::GetBusType() const {
  return params_.bus;
}

std::string ClientImpl::GetName() const {
  return params_.name;
}

std::string ClientImpl::GetAppId() const {
  return params_.appid;
}

bool ClientImpl::Call(std::string uri,
                      std::string param,
                      OnceResponse callback,
                      std::string on_cancel_value) {
  if (!handle_)
    return false;

  Error error;
  auto response = std::make_unique<Response>();
  response->ptr = this;
  response->callback = std::move(callback);
  response->uri = std::move(uri);
  response->param = std::move(param);
  response->on_cancel_value = std::move(on_cancel_value);

  LSMessageToken token;
  if (!LSCallOneReply(handle_,
                      response->uri.c_str(),
                      response->param.c_str(),
                      HandleResponse,
                      response.get(),
                      &(response->token),
                      &error)) {
    LogError(error);
    std::move(callback).Run(response->on_cancel_value);
    return false;
  }

  responses_[response.get()] = std::move(response);
  return true;
}

bool ClientImpl::Subscribe(std::string uri,
                           std::string param,
                           RepeatingResponse callback,
                           unsigned* token) {
  if (!handle_)
    return false;

  Error error;
  auto subscription = std::make_unique<Subscription>();
  subscription->callback = std::move(callback);
  subscription->uri = std::move(uri);
  subscription->param = std::move(param);

  if (!LSCall(handle_,
              subscription->uri.c_str(),
              subscription->param.c_str(),
              HandleSubscribe,
              subscription.get(),
              &(subscription->token),
              &error)) {
    LogError(error);
    callback.Run("{}");
    return false;
  }

  unsigned ret_token = static_cast<unsigned>(subscription->token);
  subscriptions_[ret_token] = std::move(subscription);
  if (token)
    *token = ret_token;
  return true;
}

void ClientImpl::Unsubscribe(unsigned token) {
  if (!handle_)
    return;

  auto it = subscriptions_.find(token);
  if (it == subscriptions_.end())
    return;

  Error error;
  LSMessageToken key = it->second->token;
  if (!LSCallCancel(handle_, key, &error))
    LOG(INFO) << "[UNSUB] " << key << " fail [" << error.message << "]";

  subscriptions_.erase(it);
}

void ClientImpl::CancelWaitingCalls() {
  if (!handle_)
    return;

  Error error;
  for (auto& response : responses_) {
    LSMessageToken key = response.second->token;
    if (!LSCallCancel(handle_, key, &error))
      LOG(INFO) << "[CANCEL] " << key << " fail [" << error.message << "]";
    std::move(response.second->callback).Run(response.second->on_cancel_value);
  }
  responses_.clear();
}

void ClientImpl::CancelAllSubscriptions() {
  if (!handle_)
    return;

  Error error;
  for (auto& subscription : subscriptions_) {
    LSMessageToken key = subscription.second->token;
    if (!LSCallCancel(handle_, key, &error))
      LOG(INFO) << "[UNSUB] " << key << " fail [" << error.message << "]";
  }
  subscriptions_.clear();
}

// static
bool ClientImpl::HandleResponse(LSHandle* sh, LSMessage* reply, void* ctx) {
  ClientImpl::Response* response = static_cast<ClientImpl::Response*>(ctx);
  if (response && response->ptr) {
    auto self = response->ptr;
    auto it = self->responses_.find(response);
    if (it != self->responses_.end()) {
      LSMessageRef(reply);
      const std::string dump = LSMessageGetPayload(reply);
      std::move(response->callback).Run(dump);
      self->responses_.erase(response);
      LSMessageUnref(reply);
    }
  } else {
    NOTREACHED();
    delete response;
  }

  return true;
}

// static
bool ClientImpl::HandleSubscribe(LSHandle* sh, LSMessage* reply, void* ctx) {
  ClientImpl::Subscription* subscription =
    static_cast<ClientImpl::Subscription*>(ctx);
  if (subscription) {
    LSMessageRef(reply);
    const std::string dump = LSMessageGetPayload(reply);
    subscription->callback.Run(dump);
    LSMessageUnref(reply);
  } else {
    NOTREACHED();
    delete subscription;
  }

  return true;
}

}  // namespace luna
}  // namespace pal
