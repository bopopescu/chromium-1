// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_NETWORK_UNIFIED_NETWORK_DETAILED_VIEW_CONTROLLER_H_
#define ASH_SYSTEM_NETWORK_UNIFIED_NETWORK_DETAILED_VIEW_CONTROLLER_H_

#include "ash/system/network/tray_network_state_observer.h"
#include "ash/system/unified/detailed_view_controller.h"

namespace ash {

namespace tray {
class NetworkListView;
}  // namespace tray

class DetailedViewDelegate;
class UnifiedSystemTrayController;

// Controller of Network detailed view in UnifiedSystemTray.
class UnifiedNetworkDetailedViewController
    : public DetailedViewController,
      public TrayNetworkStateObserver::Delegate {
 public:
  explicit UnifiedNetworkDetailedViewController(
      UnifiedSystemTrayController* tray_controller);
  ~UnifiedNetworkDetailedViewController() override;

  // DetailedViewControllerBase:
  views::View* CreateView() override;

  // TrayNetworkStateObserver::Delegate:
  void NetworkStateChanged(bool notify_a11y) override;

 private:
  const std::unique_ptr<DetailedViewDelegate> detailed_view_delegate_;
  const std::unique_ptr<TrayNetworkStateObserver> network_state_observer_;

  tray::NetworkListView* view_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(UnifiedNetworkDetailedViewController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_NETWORK_UNIFIED_NETWORK_DETAILED_VIEW_CONTROLLER_H_
