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

#include "neva/pal_service/pal_platform_factory.h"

#include "neva/pal_service/memorymanager_delegate.h"
#include <memory>

namespace pal {

std::unique_ptr<MemoryManagerDelegate>
    PlatformFactory::CreateMemoryManagerDelegate() {
  return std::unique_ptr<MemoryManagerDelegate>();
}

std::unique_ptr<SystemServiceBridgeDelegate>
    PlatformFactory::CreateSystemServiceBridgeDelegate(
        std::string,
        SystemServiceBridgeDelegate::Response) {
  return std::unique_ptr<SystemServiceBridgeDelegate>();
}

}  // namespace pal
