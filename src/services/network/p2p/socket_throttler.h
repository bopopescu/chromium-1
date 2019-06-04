// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_P2P_SOCKET_THROTTLER_H_
#define SERVICES_NETWORK_P2P_SOCKET_THROTTLER_H_

#include <stddef.h>

#include <memory>

#include "base/component_export.h"
#include "base/macros.h"

namespace rtc {
class DataRateLimiter;
}

namespace network {

// A very simple message throtller. User of this class must drop the packet if
// DropNextPacket returns false for that packet. This method verifies the
// current sendrate against the required sendrate.

class COMPONENT_EXPORT(NETWORK_SERVICE) P2PMessageThrottler {
 public:
  P2PMessageThrottler();
  virtual ~P2PMessageThrottler();

  bool DropNextPacket(size_t packet_len);
  void SetSendIceBandwidth(int bandwith_kbps);

 private:
  std::unique_ptr<rtc::DataRateLimiter> rate_limiter_;

  DISALLOW_COPY_AND_ASSIGN(P2PMessageThrottler);
};

}  // namespace network

#endif  // SERVICES_NETWORK_P2P_SOCKET_THROTTLER_H_
