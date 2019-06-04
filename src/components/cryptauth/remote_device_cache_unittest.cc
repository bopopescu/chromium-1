// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/remote_device_cache.h"

#include <algorithm>

#include "components/cryptauth/remote_device_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cryptauth {

class RemoteDeviceCacheTest : public testing::Test {
 protected:
  RemoteDeviceCacheTest()
      : test_remote_device_list_(CreateRemoteDeviceListForTest(5)),
        test_remote_device_ref_list_(CreateRemoteDeviceRefListForTest(5)){};

  // testing::Test:
  void SetUp() override {
    cache_ = RemoteDeviceCache::Factory::Get()->BuildInstance();
  }

  void VerifyCacheRemoteDevices(
      RemoteDeviceRefList expected_remote_device_ref_list) {
    RemoteDeviceRefList remote_device_ref_list = cache_->GetRemoteDevices();
    std::sort(remote_device_ref_list.begin(), remote_device_ref_list.end(),
              [](const auto& device_1, const auto& device_2) {
                return device_1 < device_2;
              });

    EXPECT_EQ(expected_remote_device_ref_list, remote_device_ref_list);
  }

  const RemoteDeviceList test_remote_device_list_;
  const RemoteDeviceRefList test_remote_device_ref_list_;
  std::unique_ptr<RemoteDeviceCache> cache_;

  DISALLOW_COPY_AND_ASSIGN(RemoteDeviceCacheTest);
};

TEST_F(RemoteDeviceCacheTest, TestNoRemoteDevices) {
  VerifyCacheRemoteDevices(RemoteDeviceRefList());
  EXPECT_EQ(base::nullopt, cache_->GetRemoteDevice(
                               test_remote_device_ref_list_[0].GetDeviceId()));
}

TEST_F(RemoteDeviceCacheTest, TestSetAndGetRemoteDevices) {
  cache_->SetRemoteDevices(test_remote_device_list_);

  VerifyCacheRemoteDevices(test_remote_device_ref_list_);
  EXPECT_EQ(
      test_remote_device_ref_list_[0],
      cache_->GetRemoteDevice(test_remote_device_ref_list_[0].GetDeviceId()));
}

TEST_F(RemoteDeviceCacheTest,
       TestSetRemoteDevices_RemoteDeviceRefsRemainValidAfterCacheRemoval) {
  cache_->SetRemoteDevices(test_remote_device_list_);

  VerifyCacheRemoteDevices(test_remote_device_ref_list_);

  cache_->SetRemoteDevices(RemoteDeviceList());

  VerifyCacheRemoteDevices(test_remote_device_ref_list_);
}

TEST_F(RemoteDeviceCacheTest,
       TestSetRemoteDevices_RemoteDeviceRefsRemainValidAfterValidCacheUpdate) {
  // Store the device with a last update time of 1000.
  cryptauth::RemoteDevice remote_device =
      cryptauth::CreateRemoteDeviceForTest();
  remote_device.last_update_time_millis = 1000;
  cache_->SetRemoteDevices({remote_device});

  cryptauth::RemoteDeviceRef remote_device_ref =
      *cache_->GetRemoteDevice(remote_device.GetDeviceId());
  EXPECT_EQ(remote_device.name, remote_device_ref.name());

  // Update the device's name and update time. Since the incoming remote device
  // has a newer update time, the entry should successfully update.
  remote_device.name = "new name";
  remote_device.last_update_time_millis = 2000;
  cache_->SetRemoteDevices({remote_device});

  EXPECT_EQ(remote_device.name, remote_device_ref.name());
}

// Currently disabled; will be re-enabled when https://crbug.com/856746 is
// fixed.
TEST_F(
    RemoteDeviceCacheTest,
    DISABLED_TestSetRemoteDevices_RemoteDeviceCacheDoesNotUpdateWithStaleRemoteDevice) {
  // Store the device with a last update time of 1000.
  cryptauth::RemoteDevice remote_device =
      cryptauth::CreateRemoteDeviceForTest();
  remote_device.last_update_time_millis = 1000;
  cache_->SetRemoteDevices({remote_device});

  cryptauth::RemoteDeviceRef remote_device_ref =
      *cache_->GetRemoteDevice(remote_device.GetDeviceId());
  EXPECT_EQ(remote_device.name, remote_device_ref.name());

  // Update the device's name and update time, this time reducing the
  // last update time to 500. Since this is less than 1000, adding the
  // device to the cache should not cause it to overwrite the previous
  // entry, since this entry is older.
  std::string prev_name = remote_device.name;
  remote_device.last_update_time_millis = 500;
  remote_device.name = "new name";
  cache_->SetRemoteDevices({remote_device});

  EXPECT_EQ(prev_name, remote_device_ref.name());
}

}  // namespace cryptauth
