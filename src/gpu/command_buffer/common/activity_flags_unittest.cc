// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/common/activity_flags.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace gpu {

TEST(ActivityFlagsTest, BasicUsage) {
  // Create the host activity flags.
  GpuProcessHostActivityFlags host_flags;
  EXPECT_FALSE(
      host_flags.IsFlagSet(ActivityFlagsBase::FLAG_LOADING_PROGRAM_BINARY));

  // Create the service activity flags from host memory.
  GpuProcessActivityFlags service_flags(host_flags.CloneHandle());

  // Ensure we can set and re-set flags.
  {
    GpuProcessActivityFlags::ScopedSetFlag scoped_set_flag(
        &service_flags, ActivityFlagsBase::FLAG_LOADING_PROGRAM_BINARY);
    EXPECT_TRUE(
        host_flags.IsFlagSet(ActivityFlagsBase::FLAG_LOADING_PROGRAM_BINARY));
  }
  EXPECT_FALSE(
      host_flags.IsFlagSet(ActivityFlagsBase::FLAG_LOADING_PROGRAM_BINARY));
}

TEST(ActivityFlagsTest, NotInitialized) {
  // Get the service activity flags without providing host memory.
  auto buffer = mojo::ScopedSharedBufferHandle();
  GpuProcessActivityFlags service_flags(std::move(buffer));

  // Set/Unset should not crash.
  {
    GpuProcessActivityFlags::ScopedSetFlag scoped_set_flag(
        &service_flags, ActivityFlagsBase::FLAG_LOADING_PROGRAM_BINARY);
  }
}

}  // namespace gpu
