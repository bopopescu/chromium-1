// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_INPUT_STREAM_H_
#define SERVICES_AUDIO_INPUT_STREAM_H_

#include <memory>
#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/sync_socket.h"
#include "base/unguessable_token.h"
#include "media/mojo/interfaces/audio_data_pipe.mojom.h"
#include "media/mojo/interfaces/audio_input_stream.mojom.h"
#include "media/mojo/interfaces/audio_logging.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/audio/input_controller.h"
#include "services/audio/public/mojom/audio_processing.mojom.h"
#include "services/audio/stream_monitor_coordinator.h"

namespace media {

class AudioManager;
class AudioParameters;

}  // namespace media

namespace audio {

class InputSyncWriter;
class UserInputMonitor;

class InputStream final : public media::mojom::AudioInputStream,
                          public InputController::EventHandler {
 public:
  using CreatedCallback =
      base::OnceCallback<void(media::mojom::ReadOnlyAudioDataPipePtr,
                              bool,
                              const base::Optional<base::UnguessableToken>&)>;
  using DeleteCallback = base::OnceCallback<void(InputStream*)>;

  InputStream(CreatedCallback created_callback,
              DeleteCallback delete_callback,
              media::mojom::AudioInputStreamRequest request,
              media::mojom::AudioInputStreamClientPtr client,
              media::mojom::AudioInputStreamObserverPtr observer,
              media::mojom::AudioLogPtr log,
              media::AudioManager* manager,
              std::unique_ptr<UserInputMonitor> user_input_monitor,
              const std::string& device_id,
              const media::AudioParameters& params,
              uint32_t shared_memory_count,
              bool enable_agc,
              StreamMonitorCoordinator* stream_monitor_coordinator,
              mojom::AudioProcessingConfigPtr processing_config);
  ~InputStream() override;

  const base::UnguessableToken& id() const { return id_; }
  void SetOutputDeviceForAec(const std::string& output_device_id);

  // media::mojom::AudioInputStream implementation.
  void Record() override;
#if defined(USE_NEVA_SUSPEND_MEDIA_CAPTURE)
  void Pause() override;
  void Resume() override;
#endif
  void SetVolume(double volume) override;

  // InputController::EventHandler implementation.
  void OnCreated(bool initially_muted) override;
  void OnError(InputController::ErrorCode error_code) override;
  void OnLog(base::StringPiece) override;
  void OnMuted(bool is_muted) override;

 private:
  void OnStreamError(bool signalPlatformError);
  void CallDeleter();

  const base::UnguessableToken id_;

  mojo::Binding<media::mojom::AudioInputStream> binding_;
  media::mojom::AudioInputStreamClientPtr client_;
  media::mojom::AudioInputStreamObserverPtr observer_;
  const scoped_refptr<media::mojom::ThreadSafeAudioLogPtr> log_;

  // Notify stream client on creation.
  CreatedCallback created_callback_;

  // Notify stream factory (audio service) on destruction.
  DeleteCallback delete_callback_;

  base::CancelableSyncSocket foreign_socket_;
  const std::unique_ptr<InputSyncWriter> writer_;
  std::unique_ptr<InputController> controller_;
  const std::unique_ptr<UserInputMonitor> user_input_monitor_;

  SEQUENCE_CHECKER(owning_sequence_);

  base::WeakPtrFactory<InputStream> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(InputStream);
};

}  // namespace audio

#endif  // SERVICES_AUDIO_INPUT_STREAM_H_
