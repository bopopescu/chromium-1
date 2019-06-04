// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SEARCH_ENGINES_SEARCH_ENGINES_TEST_UTIL_H_
#define COMPONENTS_SEARCH_ENGINES_SEARCH_ENGINES_TEST_UTIL_H_

#include <memory>
#include <string>

struct TemplateURLData;

namespace sync_preferences {
class TestingPrefServiceSyncable;
}

// Generates a TemplateURLData structure useful for tests filled with values
// autogenerated from |provider_name|.
std::unique_ptr<TemplateURLData> GenerateDummyTemplateURLData(
    const std::string& keyword);

// Checks that the two TemplateURLs are similar. Does not check the id, the
// date_created or the last_modified time.  Neither pointer should be null.
void ExpectSimilar(const TemplateURLData* expected,
                   const TemplateURLData* actual);

// Writes default search engine |extension_data| into the extension-controlled
// preference in |prefs|.
void SetExtensionDefaultSearchInPrefs(
    sync_preferences::TestingPrefServiceSyncable* prefs,
    const TemplateURLData& extension_data);

// Removes the extension-controlled default search engine preference from
// |prefs|.
void RemoveExtensionDefaultSearchFromPrefs(
    sync_preferences::TestingPrefServiceSyncable* prefs);

#endif  // COMPONENTS_SEARCH_ENGINES_SEARCH_ENGINES_TEST_UTIL_H_
