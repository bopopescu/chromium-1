// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_LAUNCHER_SHELF_SPINNER_ITEM_CONTROLLER_H_
#define CHROME_BROWSER_UI_ASH_LAUNCHER_SHELF_SPINNER_ITEM_CONTROLLER_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "ash/public/cpp/shelf_item_delegate.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"

class ShelfSpinnerController;
class LauncherContextMenu;

// ShelfSpinnerItemController displays the icon of an app that cannot be
// launched immediately (due to ARC or Crostini not being ready) on Chrome OS'
// shelf, with an overlaid spinner to provide visual feedback.
class ShelfSpinnerItemController : public ash::ShelfItemDelegate {
 public:
  explicit ShelfSpinnerItemController(const std::string& app_id);

  ~ShelfSpinnerItemController() override;

  virtual void SetHost(const base::WeakPtr<ShelfSpinnerController>& host);

  base::TimeDelta GetActiveTime() const;

  // ash::ShelfItemDelegate:
  void ItemSelected(std::unique_ptr<ui::Event> event,
                    int64_t display_id,
                    ash::ShelfLaunchSource source,
                    ItemSelectedCallback callback) override;
  void ExecuteCommand(bool from_context_menu,
                      int64_t command_id,
                      int32_t event_flags,
                      int64_t display_id) override;
  void GetContextMenu(int64_t display_id,
                      GetMenuModelCallback callback) override;
  void Close() override;

 private:
  base::WeakPtr<ShelfSpinnerController> host_;
  const base::Time start_time_;

  std::unique_ptr<LauncherContextMenu> context_menu_;

  DISALLOW_COPY_AND_ASSIGN(ShelfSpinnerItemController);
};

#endif  // CHROME_BROWSER_UI_ASH_LAUNCHER_SHELF_SPINNER_ITEM_CONTROLLER_H_
