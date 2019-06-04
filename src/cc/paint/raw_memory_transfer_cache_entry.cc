// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/raw_memory_transfer_cache_entry.h"

#include <string.h>

namespace cc {

ClientRawMemoryTransferCacheEntry::ClientRawMemoryTransferCacheEntry(
    std::vector<uint8_t> data)
    : id_(s_next_id_.GetNext()), data_(std::move(data)) {}
ClientRawMemoryTransferCacheEntry::~ClientRawMemoryTransferCacheEntry() =
    default;

// static
base::AtomicSequenceNumber ClientRawMemoryTransferCacheEntry::s_next_id_;

size_t ClientRawMemoryTransferCacheEntry::SerializedSize() const {
  return data_.size();
}

uint32_t ClientRawMemoryTransferCacheEntry::Id() const {
  return id_;
}

bool ClientRawMemoryTransferCacheEntry::Serialize(
    base::span<uint8_t> data) const {
  if (data.size() < data_.size())
    return false;

  memcpy(data.data(), data_.data(), data_.size());
  return true;
}

ServiceRawMemoryTransferCacheEntry::ServiceRawMemoryTransferCacheEntry() =
    default;
ServiceRawMemoryTransferCacheEntry::~ServiceRawMemoryTransferCacheEntry() =
    default;

size_t ServiceRawMemoryTransferCacheEntry::CachedSize() const {
  return data_.size();
}

bool ServiceRawMemoryTransferCacheEntry::Deserialize(
    GrContext* context,
    base::span<const uint8_t> data) {
  data_ = std::vector<uint8_t>(data.begin(), data.end());
  return true;
}

}  // namespace cc
