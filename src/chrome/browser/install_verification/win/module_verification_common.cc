// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/install_verification/win/module_verification_common.h"

#include "base/files/file_path.h"
#include "base/md5.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/win_util.h"
#include "chrome/browser/install_verification/win/module_info.h"
#include "chrome/browser/install_verification/win/module_list.h"

bool GetLoadedModules(std::set<ModuleInfo>* loaded_modules) {
  std::vector<HMODULE> snapshot;
  if (!base::win::GetLoadedModulesSnapshot(::GetCurrentProcess(), &snapshot))
    return false;

  ModuleList::FromLoadedModuleSnapshot(snapshot)->GetModuleInfoSet(
      loaded_modules);
  return true;
}
