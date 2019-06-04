// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/gestures/gesture_provider_aura.h"

#include <memory>

#include "base/test/scoped_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event_utils.h"

namespace ui {

class GestureProviderAuraTest : public testing::Test,
                                public GestureProviderAuraClient {
 public:
  GestureProviderAuraTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}

  ~GestureProviderAuraTest() override {}

  void OnGestureEvent(GestureConsumer* raw_input_consumer,
                      GestureEvent* event) override {}

  void SetUp() override {
    consumer_.reset(new GestureConsumer());
    provider_.reset(new GestureProviderAura(consumer_.get(), this));
  }

  void TearDown() override { provider_.reset(); }

  GestureProviderAura* provider() { return provider_.get(); }

 private:
  std::unique_ptr<GestureConsumer> consumer_;
  std::unique_ptr<GestureProviderAura> provider_;
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(GestureProviderAuraTest, IgnoresExtraPressEvents) {
  base::TimeTicks time = ui::EventTimeForNow();
  TouchEvent press1(
      ET_TOUCH_PRESSED, gfx::Point(10, 10), time,
      PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 0));
  EXPECT_TRUE(provider()->OnTouchEvent(&press1));

  time += base::TimeDelta::FromMilliseconds(10);
  TouchEvent press2(
      ET_TOUCH_PRESSED, gfx::Point(30, 40), time,
      PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 0));
  EXPECT_FALSE(provider()->OnTouchEvent(&press2));
}

TEST_F(GestureProviderAuraTest, IgnoresExtraMoveOrReleaseEvents) {
  base::TimeTicks time = ui::EventTimeForNow();
  TouchEvent press1(
      ET_TOUCH_PRESSED, gfx::Point(10, 10), time,
      PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 0));
  EXPECT_TRUE(provider()->OnTouchEvent(&press1));

  time += base::TimeDelta::FromMilliseconds(10);
  TouchEvent release1(
      ET_TOUCH_RELEASED, gfx::Point(30, 40), time,
      PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 0));
  EXPECT_TRUE(provider()->OnTouchEvent(&release1));

  time += base::TimeDelta::FromMilliseconds(10);
  TouchEvent release2(
      ET_TOUCH_RELEASED, gfx::Point(30, 45), time,
      PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 0));
  EXPECT_FALSE(provider()->OnTouchEvent(&release1));

  time += base::TimeDelta::FromMilliseconds(10);
  TouchEvent move1(ET_TOUCH_MOVED, gfx::Point(70, 75), time,
                   PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 0));
  EXPECT_FALSE(provider()->OnTouchEvent(&move1));
}

TEST_F(GestureProviderAuraTest, DoesntStallOnCancelAndRelease) {
  base::TimeTicks time = ui::EventTimeForNow();

  TouchEvent touch_press(
      ET_TOUCH_PRESSED, gfx::Point(10, 10), time,
      PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 0));
  EXPECT_TRUE(provider()->OnTouchEvent(&touch_press));
  time += base::TimeDelta::FromMilliseconds(10);

  TouchEvent pen_press1(
      ET_TOUCH_PRESSED, gfx::Point(20, 20), time,
      PointerDetails(ui::EventPointerType::POINTER_TYPE_PEN, 1));
  EXPECT_TRUE(provider()->OnTouchEvent(&pen_press1));
  time += base::TimeDelta::FromMilliseconds(10);

  TouchEvent touch_cancel(
      ET_TOUCH_CANCELLED, gfx::Point(30, 30), time,
      PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 0));
  EXPECT_TRUE(provider()->OnTouchEvent(&touch_cancel));
  time += base::TimeDelta::FromMilliseconds(10);

  TouchEvent pen_release1(
      ET_TOUCH_RELEASED, gfx::Point(40, 40), time,
      PointerDetails(ui::EventPointerType::POINTER_TYPE_PEN, 1));
  EXPECT_FALSE(provider()->OnTouchEvent(&pen_release1));
  time += base::TimeDelta::FromMilliseconds(10);

  TouchEvent pen_press2(
      ET_TOUCH_PRESSED, gfx::Point(10, 10), time,
      PointerDetails(ui::EventPointerType::POINTER_TYPE_PEN, 0));
  EXPECT_TRUE(provider()->OnTouchEvent(&pen_press2));
  time += base::TimeDelta::FromMilliseconds(10);

  TouchEvent pen_release2(
      ET_TOUCH_RELEASED, gfx::Point(10, 10), time,
      PointerDetails(ui::EventPointerType::POINTER_TYPE_PEN, 0));
  EXPECT_TRUE(provider()->OnTouchEvent(&pen_release2));
}

TEST_F(GestureProviderAuraTest, IgnoresIdenticalMoveEvents) {
  const float kRadiusX = 20.f;
  const float kRadiusY = 30.f;
  const float kAngle = 0.321f;
  const float kForce = 40.f;
  const int kTouchId0 = 5;
  const int kTouchId1 = 3;

  PointerDetails pointer_details1(EventPointerType::POINTER_TYPE_TOUCH,
                                  kTouchId0);
  base::TimeTicks time = ui::EventTimeForNow();
  TouchEvent press0_1(ET_TOUCH_PRESSED, gfx::Point(9, 10), time,
                      pointer_details1);
  EXPECT_TRUE(provider()->OnTouchEvent(&press0_1));

  PointerDetails pointer_details2(EventPointerType::POINTER_TYPE_TOUCH,
                                  kTouchId1);
  TouchEvent press1_1(ET_TOUCH_PRESSED, gfx::Point(40, 40), time,
                      pointer_details2);
  EXPECT_TRUE(provider()->OnTouchEvent(&press1_1));

  time += base::TimeDelta::FromMilliseconds(10);
  pointer_details1 =
      PointerDetails(EventPointerType::POINTER_TYPE_TOUCH, kTouchId0, kRadiusX,
                     kRadiusY, kForce, kAngle);
  TouchEvent move0_1(ET_TOUCH_MOVED, gfx::Point(10, 10), time, pointer_details1,
                     0);
  EXPECT_TRUE(provider()->OnTouchEvent(&move0_1));

  pointer_details2 =
      PointerDetails(EventPointerType::POINTER_TYPE_TOUCH, kTouchId1, kRadiusX,
                     kRadiusY, kForce, kAngle);
  TouchEvent move1_1(ET_TOUCH_MOVED, gfx::Point(100, 200), time,
                     pointer_details2, 0);
  EXPECT_TRUE(provider()->OnTouchEvent(&move1_1));

  time += base::TimeDelta::FromMilliseconds(10);
  TouchEvent move0_2(ET_TOUCH_MOVED, gfx::Point(10, 10), time, pointer_details1,
                     0);
  // Nothing has changed, so ignore the move.
  EXPECT_FALSE(provider()->OnTouchEvent(&move0_2));

  TouchEvent move1_2(ET_TOUCH_MOVED, gfx::Point(100, 200), time,
                     pointer_details2, 0);
  // Nothing has changed, so ignore the move.
  EXPECT_FALSE(provider()->OnTouchEvent(&move1_2));

  time += base::TimeDelta::FromMilliseconds(10);
  TouchEvent move0_3(ET_TOUCH_MOVED, gfx::Point(), time, pointer_details1, 0);
  move0_3.set_location_f(gfx::PointF(70, 75.1f));
  move0_3.set_root_location_f(gfx::PointF(70, 75.1f));
  // Position has changed, so don't ignore the move.
  EXPECT_TRUE(provider()->OnTouchEvent(&move0_3));

  time += base::TimeDelta::FromMilliseconds(10);
  pointer_details2.radius_y += 1;
  TouchEvent move0_4(ET_TOUCH_MOVED, gfx::Point(), time, pointer_details2, 0);
  move0_4.set_location_f(gfx::PointF(70, 75.1f));
  move0_4.set_root_location_f(gfx::PointF(70, 75.1f));
}

// TODO(jdduke): Test whether event marked as scroll trigger.

}  // namespace ui
