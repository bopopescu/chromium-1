// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_PLATFORM_NOTIFICATION_SERVICE_H_
#define CONTENT_PUBLIC_BROWSER_PLATFORM_NOTIFICATION_SERVICE_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "content/common/content_export.h"
#include "content/public/browser/notification_database_data.h"
#include "third_party/blink/public/platform/modules/permissions/permission_status.mojom.h"

class GURL;

namespace blink {
struct NotificationResources;
struct PlatformNotificationData;
}  // namespace blink

namespace content {

class BrowserContext;

// The service using which notifications can be presented to the user. There
// should be a unique instance of the PlatformNotificationService depending
// on the browsing context being used.
class CONTENT_EXPORT PlatformNotificationService {
 public:
  virtual ~PlatformNotificationService() {}

  using DisplayedNotificationsCallback =
      base::Callback<void(std::unique_ptr<std::set<std::string>>,
                          bool /* supports synchronization */)>;

  // Displays the notification described in |notification_data| to the user.
  // This method must be called on the UI thread.
  virtual void DisplayNotification(
      BrowserContext* browser_context,
      const std::string& notification_id,
      const GURL& origin,
      const blink::PlatformNotificationData& notification_data,
      const blink::NotificationResources& notification_resources) = 0;

  // Displays the persistent notification described in |notification_data| to
  // the user. This method must be called on the UI thread.
  virtual void DisplayPersistentNotification(
      BrowserContext* browser_context,
      const std::string& notification_id,
      const GURL& service_worker_origin,
      const GURL& origin,
      const blink::PlatformNotificationData& notification_data,
      const blink::NotificationResources& notification_resources) = 0;

  // Closes the notification identified by |notification_id|. This method must
  // be called on the UI thread.
  virtual void CloseNotification(BrowserContext* browser_context,
                                 const std::string& notification_id) = 0;

  // Closes the persistent notification identified by |notification_id|. This
  // method must be called on the UI thread.
  virtual void ClosePersistentNotification(
      BrowserContext* browser_context,
      const std::string& notification_id) = 0;

  // Retrieves the ids of all currently displaying notifications and
  // posts |callback| with the result.
  virtual void GetDisplayedNotifications(
      BrowserContext* browser_context,
      const DisplayedNotificationsCallback& callback) = 0;

  // Reads the value of the next persistent notification ID from the profile and
  // increments the value, as it is called once per notification write.
  virtual int64_t ReadNextPersistentNotificationId(
      BrowserContext* browser_context) = 0;

  // Records a given notification to UKM.
  virtual void RecordNotificationUkmEvent(
      BrowserContext* browser_context,
      const NotificationDatabaseData& data) = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_PLATFORM_NOTIFICATION_SERVICE_H_
