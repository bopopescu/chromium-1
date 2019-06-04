// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/system/sys_info.h"
#include "content/public/browser/gpu_data_manager.h"
#include "gpu/config/gpu_info.h"
#include "jni/SystemInfoFeedbackSource_jni.h"

using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace chrome {
namespace android {

ScopedJavaLocalRef<jstring> JNI_SystemInfoFeedbackSource_GetCpuArchitecture(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
  return ConvertUTF8ToJavaString(env,
                                 base::SysInfo::OperatingSystemArchitecture());
}

ScopedJavaLocalRef<jstring> JNI_SystemInfoFeedbackSource_GetGpuVendor(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
  gpu::GPUInfo info = content::GpuDataManager::GetInstance()->GetGPUInfo();

  return ConvertUTF8ToJavaString(env, info.active_gpu().vendor_string);
}

ScopedJavaLocalRef<jstring> JNI_SystemInfoFeedbackSource_GetGpuModel(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
  gpu::GPUInfo info = content::GpuDataManager::GetInstance()->GetGPUInfo();
  return ConvertUTF8ToJavaString(env, info.active_gpu().device_string);
}

int JNI_SystemInfoFeedbackSource_GetAvailableMemoryMB(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
  return base::saturated_cast<int>(
      base::SysInfo::AmountOfAvailablePhysicalMemory() / 1024 / 1024);
}

int JNI_SystemInfoFeedbackSource_GetTotalMemoryMB(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
  return base::SysInfo::AmountOfPhysicalMemoryMB();
}

}  // namespace android
}  // namespace chrome
