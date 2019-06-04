// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_NOTIFICATION_PRESENTER_H_
#define CHROMEOS_COMPONENTS_TETHER_NOTIFICATION_PRESENTER_H_

#include "base/macros.h"
#include "base/observer_list.h"
#include "chromeos/network/network_state.h"
#include "components/cryptauth/remote_device_ref.h"

namespace chromeos {

namespace tether {

class NotificationPresenter {
 public:
  enum class PotentialHotspotNotificationState {
    SINGLE_HOTSPOT_NEARBY_SHOWN,
    MULTIPLE_HOTSPOTS_NEARBY_SHOWN,
    NO_HOTSPOT_NOTIFICATION_SHOWN
  };

  NotificationPresenter() {}
  virtual ~NotificationPresenter() {}

  // Notifies the user that a nearby device can potentially provide a tether
  // hotspot, and shows the signal strength with a blue icon.
  virtual void NotifyPotentialHotspotNearby(
      cryptauth::RemoteDeviceRef remote_device,
      int signal_strength) = 0;

  // Notifies the user that multiple nearby devices can potentially provide
  // tether hotspots.
  virtual void NotifyMultiplePotentialHotspotsNearby() = 0;

  // Returns the state of the "potential hotspot(s)" notification.
  virtual PotentialHotspotNotificationState
  GetPotentialHotspotNotificationState() = 0;

  // Removes the notification created by either NotifyPotentialHotspotNearby()
  // or NotifyMultiplePotentialHotspotsNearby(), or does nothing if that
  // notification is not currently displayed.
  virtual void RemovePotentialHotspotNotification() = 0;

  // Notifies the user that the device they are connecting to requires
  // first time setup and must be interacted with.
  virtual void NotifySetupRequired(const std::string& device_name,
                                   int signal_strength) = 0;

  // Removes the notification created by NotifyFirstTimeSetupRequired(), or does
  // nothing if that notification is not currently displayed.
  virtual void RemoveSetupRequiredNotification() = 0;

  // Notifies the user that the connection attempt has failed.
  virtual void NotifyConnectionToHostFailed() = 0;

  // Removes the notification created by NotifyConnectionToHostFailed(), or does
  // nothing if that notification is not currently displayed.
  virtual void RemoveConnectionToHostFailedNotification() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(NotificationPresenter);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_NOTIFICATION_PRESENTER_H_
