// Copyright 2017-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "media/blink/neva/mediaplayerneva_factory.h"

#include "media/blink/neva/webos/mediaplayer_ums.h"
#include "net/base/mime_util.h"

namespace media {

// also see media/base/mime_util_internal.cc(webos_codecs).
const char kPrefixOfWebOSMediaMimeType[] = "service/webos-*";

MediaPlayerNevaFactory::MediaPlayerType
MediaPlayerNevaFactory::GetMediaPlayerType(const std::string& mime_type) {
  if (net::MatchesMimeType(kPrefixOfWebOSMediaMimeType, mime_type)) {
      return MediaPlayerTypeNone;
  }
  // If url has no webOS prefix than we should handle it as a normal
  // LoadTypeURL
  return MediaPlayerTypeUMS;
}

MediaPlayerNeva* MediaPlayerNevaFactory::CreateMediaPlayerNeva(
    MediaPlayerNevaClient* client,
    const MediaPlayerType media_type,
    const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner) {
  switch (media_type) {
    case MediaPlayerNevaFactory::MediaPlayerTypeUMS:
      return new MediaPlayerUMS(client, main_task_runner);
      break;
    default:
      break;
  }
  return nullptr;
}

}  // namespace media
