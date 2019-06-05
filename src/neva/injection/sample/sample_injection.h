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

#ifndef NEVA_INJECTION_SAMPLE_SAMPLE_INJECTION_H_
#define NEVA_INJECTION_SAMPLE_SAMPLE_INJECTION_H_

#include <string>

#include "base/compiler_specific.h"
#include "neva/injection/common/public/renderer/injection_data_manager.h"
#include "neva/injection/common/public/renderer/injection_install.h"
#include "v8/include/v8.h"


#if defined(COMPONENT_BUILD)
#define SAMPLE_INJECTION_EXPORT __attribute__((visibility("default")))
#else
#define SAMPLE_INJECTION_EXPORT
#endif  // defined(COMPONENT_BUILD)

namespace blink {
class WebLocalFrame;
}  // namespace blink

namespace injections {

class SAMPLE_INJECTION_EXPORT SampleInjectionExtension {
 public:
  static const char kInjectionName[];

  static void Install(blink::WebLocalFrame* frame);
  static void Uninstall(blink::WebLocalFrame* frame);
  static void DispatchValueChanged(blink::WebLocalFrame* frame,
                                   const std::string& val);

 private:
  static v8::MaybeLocal<v8::Object> CreateSampleObject(
      v8::Isolate* isolate, v8::Local<v8::Object> global);
  static void RunSupplementJS(
      v8::Isolate* isolate, v8::Local<v8::Context> context);
};

}  // namespace injections

#endif  // NEVA_INJECTION_SAMPLE_SAMPLE_INJECTION_H_
