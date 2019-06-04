// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_TEST_TEST_LAYOUT_MANAGER_H_
#define UI_VIEWS_TEST_TEST_LAYOUT_MANAGER_H_

#include "base/macros.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/layout/layout_manager.h"

namespace views {
namespace test {

// A stub layout manager that returns a specific preferred size and height for
// width.
class TestLayoutManager : public LayoutManager {
 public:
  TestLayoutManager();
  ~TestLayoutManager() override;

  void SetPreferredSize(const gfx::Size& size) { preferred_size_ = size; }

  void set_preferred_height_for_width(int height) {
    preferred_height_for_width_ = height;
  }

  // LayoutManager:
  void Layout(View* host) override;
  gfx::Size GetPreferredSize(const View* host) const override;
  int GetPreferredHeightForWidth(const View* host, int width) const override;

 private:
  // The return value of GetPreferredSize();
  gfx::Size preferred_size_;

  // The return value for GetPreferredHeightForWidth().
  int preferred_height_for_width_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestLayoutManager);
};

}  // namespace test
}  // namespace views

#endif  // UI_VIEWS_TEST_TEST_LAYOUT_MANAGER_H_
