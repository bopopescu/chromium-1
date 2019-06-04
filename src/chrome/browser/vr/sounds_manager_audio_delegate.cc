// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/sounds_manager_audio_delegate.h"
#include "media/audio/sounds/sounds_manager.h"

namespace vr {

SoundsManagerAudioDelegate::SoundsManagerAudioDelegate() {}

SoundsManagerAudioDelegate::~SoundsManagerAudioDelegate() {
  ResetSounds();
}

void SoundsManagerAudioDelegate::ResetSounds() {
  // Because SoundsManager cannot replace a registered sound, start fresh
  // with a new manager if needed.
  if (!sounds_.empty()) {
    media::SoundsManager::Shutdown();
    sounds_.clear();
  }
}

bool SoundsManagerAudioDelegate::RegisterSound(
    SoundId id,
    std::unique_ptr<std::string> data) {
  DCHECK_NE(id, kSoundNone);
  DCHECK(sounds_.find(id) == sounds_.end());

  if (sounds_.empty())
    media::SoundsManager::Create();

  sounds_[id] = std::move(data);
  return media::SoundsManager::Get()->Initialize(id, *sounds_[id]);
}

void SoundsManagerAudioDelegate::PlaySound(SoundId id) {
  if (sounds_.find(id) != sounds_.end())
    media::SoundsManager::Get()->Play(id);
}

}  // namespace vr
