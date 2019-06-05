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

#ifndef NEVA_APP_RUNTIME_PUBLIC_APP_RUNTIME_EXPORT_H_
#define NEVA_APP_RUNTIME_PUBLIC_APP_RUNTIME_EXPORT_H_

#if defined(COMPONENT_BUILD) || defined(USE_CBE)
#if defined(APP_RUNTIME_IMPLEMENTATION)
#define APP_RUNTIME_EXPORT __attribute__((visibility("default")))
#else
#define APP_RUNTIME_EXPORT
#endif  // defined(APP_RUNTIME_IMPLEMENTATION)
#else
#define APP_RUNTIME_EXPORT
#endif  // defined(COMPONENT_BUILD) || defined(USE_CBE)

#endif  // NEVA_APP_RUNTIME_PUBLIC_APP_RUNTIME_EXPORT_H_
