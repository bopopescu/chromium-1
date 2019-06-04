// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "content/browser/code_cache/generated_code_cache_context.h"
#include "base/files/file_path.h"
#include "base/task/post_task.h"
#include "content/browser/code_cache/generated_code_cache.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/blink/public/common/features.h"

namespace content {

GeneratedCodeCacheContext::GeneratedCodeCacheContext() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

void GeneratedCodeCacheContext::Initialize(const base::FilePath& path,
                                           int max_bytes) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  base::PostTaskWithTraits(
      FROM_HERE, {BrowserThread::IO},
      base::BindOnce(&GeneratedCodeCacheContext::InitializeOnIO, this, path,
                     max_bytes));
}

void GeneratedCodeCacheContext::InitializeOnIO(const base::FilePath& path,
                                               int max_bytes) {
  generated_js_code_cache_.reset(
      new GeneratedCodeCache(path.AppendASCII("js"), max_bytes,
                             GeneratedCodeCache::CodeCacheType::kJavaScript));

  // Only create the Wasm cache if it's enabled.
  if (base::FeatureList::IsEnabled(blink::features::kWasmCodeCache)) {
    generated_wasm_code_cache_.reset(new GeneratedCodeCache(
        path.AppendASCII("wasm"), max_bytes,
        GeneratedCodeCache::CodeCacheType::kWebAssembly));
  }
}

GeneratedCodeCache* GeneratedCodeCacheContext::generated_js_code_cache() const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return generated_js_code_cache_.get();
}

GeneratedCodeCache* GeneratedCodeCacheContext::generated_wasm_code_cache()
    const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return generated_wasm_code_cache_.get();
}

GeneratedCodeCacheContext::~GeneratedCodeCacheContext() = default;

}  // namespace content
