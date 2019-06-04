// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/power_utils.h"

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ash {

namespace power_utils {

using PowerUtilsTest = testing::Test;

TEST_F(PowerUtilsTest, SplitTimeIntoHoursAndMinutes) {
  int hours = 0, minutes = 0;
  SplitTimeIntoHoursAndMinutes(base::TimeDelta::FromSeconds(0), &hours,
                               &minutes);
  EXPECT_EQ(0, hours);
  EXPECT_EQ(0, minutes);

  SplitTimeIntoHoursAndMinutes(base::TimeDelta::FromSeconds(60), &hours,
                               &minutes);
  EXPECT_EQ(0, hours);
  EXPECT_EQ(1, minutes);

  SplitTimeIntoHoursAndMinutes(base::TimeDelta::FromSeconds(3600), &hours,
                               &minutes);
  EXPECT_EQ(1, hours);
  EXPECT_EQ(0, minutes);

  SplitTimeIntoHoursAndMinutes(base::TimeDelta::FromSeconds(3600 + 60), &hours,
                               &minutes);
  EXPECT_EQ(1, hours);
  EXPECT_EQ(1, minutes);

  SplitTimeIntoHoursAndMinutes(base::TimeDelta::FromSeconds(7 * 3600 + 23 * 60),
                               &hours, &minutes);
  EXPECT_EQ(7, hours);
  EXPECT_EQ(23, minutes);

  // Check that minutes are rounded.
  SplitTimeIntoHoursAndMinutes(
      base::TimeDelta::FromSeconds(2 * 3600 + 3 * 60 + 30), &hours, &minutes);
  EXPECT_EQ(2, hours);
  EXPECT_EQ(4, minutes);

  SplitTimeIntoHoursAndMinutes(
      base::TimeDelta::FromSeconds(2 * 3600 + 3 * 60 + 29), &hours, &minutes);
  EXPECT_EQ(2, hours);
  EXPECT_EQ(3, minutes);

  // Check that times close to hour boundaries aren't incorrectly rounded such
  // that they display 60 minutes: http://crbug.com/368261
  SplitTimeIntoHoursAndMinutes(base::TimeDelta::FromSecondsD(3599.9), &hours,
                               &minutes);
  EXPECT_EQ(1, hours);
  EXPECT_EQ(0, minutes);

  SplitTimeIntoHoursAndMinutes(base::TimeDelta::FromSecondsD(3600.1), &hours,
                               &minutes);
  EXPECT_EQ(1, hours);
  EXPECT_EQ(0, minutes);
}

TEST_F(PowerUtilsTest, ShouldDisplayBatteryTime) {
  EXPECT_FALSE(ShouldDisplayBatteryTime(base::TimeDelta::FromSeconds(-1)));
  EXPECT_FALSE(ShouldDisplayBatteryTime(base::TimeDelta::FromSeconds(0)));
  EXPECT_FALSE(ShouldDisplayBatteryTime(base::TimeDelta::FromSeconds(59)));
  EXPECT_TRUE(ShouldDisplayBatteryTime(base::TimeDelta::FromSeconds(60)));
  EXPECT_TRUE(ShouldDisplayBatteryTime(base::TimeDelta::FromSeconds(600)));
  EXPECT_TRUE(ShouldDisplayBatteryTime(base::TimeDelta::FromSeconds(3600)));

  // Matches the constant in power_utils.cc.
  base::TimeDelta max_displayed_battery_time = base::TimeDelta::FromDays(1);
  EXPECT_TRUE(ShouldDisplayBatteryTime(max_displayed_battery_time));
  EXPECT_FALSE(ShouldDisplayBatteryTime(max_displayed_battery_time +
                                        base::TimeDelta::FromSeconds(1)));
}

}  // namespace power_utils

}  // namespace ash
