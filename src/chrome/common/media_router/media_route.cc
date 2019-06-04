// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media_router/media_route.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "chrome/common/media_router/media_source.h"

namespace media_router {

// static
MediaRoute::Id MediaRoute::GetMediaRouteId(const std::string& presentation_id,
                                           const MediaSink::Id& sink_id,
                                           const MediaSource& source) {
  // TODO(https://crbug.com/816628): Can the route ID just be the presentation
  // id?
  return base::StringPrintf("urn:x-org.chromium:media:route:%s/%s/%s",
                            presentation_id.c_str(), sink_id.c_str(),
                            source.id().c_str());
}

MediaRoute::MediaRoute(const MediaRoute::Id& media_route_id,
                       const MediaSource& media_source,
                       const MediaSink::Id& media_sink_id,
                       const std::string& description,
                       bool is_local,
                       bool for_display)
    : media_route_id_(media_route_id),
      media_source_(media_source),
      media_sink_id_(media_sink_id),
      description_(description),
      is_local_(is_local),
      for_display_(for_display),
      is_incognito_(false),
      is_local_presentation_(false) {}

MediaRoute::MediaRoute(const MediaRoute& other) = default;

MediaRoute::MediaRoute() {}
MediaRoute::~MediaRoute() = default;

bool MediaRoute::Equals(const MediaRoute& other) const {
  return media_route_id_ == other.media_route_id_;
}

}  // namespace media_router
