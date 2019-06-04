// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_ARC_ARC_APP_ITEM_H_
#define CHROME_BROWSER_UI_APP_LIST_ARC_ARC_APP_ITEM_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/ui/app_list/app_context_menu_delegate.h"
#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "chrome/browser/ui/app_list/chrome_app_list_item.h"
#include "components/arc/metrics/arc_metrics_constants.h"

class ArcAppContextMenu;
class Profile;

// ArcAppItem represents an ARC app in app list.
class ArcAppItem : public ChromeAppListItem,
                   public app_list::AppContextMenuDelegate {
 public:
  static const char kItemType[];

  ArcAppItem(Profile* profile,
             AppListModelUpdater* model_updater,
             const app_list::AppListSyncableService::SyncItem* sync_item,
             const std::string& id,
             const std::string& name);
  ~ArcAppItem() override;

  void SetName(const std::string& name);

  // ChromeAppListItem overrides:
  void Activate(int event_flags) override;
  void GetContextMenuModel(GetMenuModelCallback callback) override;
  const char* GetItemType() const override;

  // app_list::AppContextMenuDelegate overrides:
  void ExecuteLaunchCommand(int event_flags) override;

 private:
  // ChromeAppListItem overrides:
  app_list::AppContextMenu* GetAppContextMenu() override;

  void Launch(int event_flags, arc::UserInteractionType interaction);

  std::unique_ptr<ArcAppContextMenu> context_menu_;

  DISALLOW_COPY_AND_ASSIGN(ArcAppItem);
};

#endif  // CHROME_BROWSER_UI_APP_LIST_ARC_ARC_APP_ITEM_H_
