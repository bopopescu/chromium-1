// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ANDROID_CODEC_IMAGE_GROUP_H_
#define MEDIA_GPU_ANDROID_CODEC_IMAGE_GROUP_H_

#include <unordered_set>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "media/gpu/android/codec_image.h"
#include "media/gpu/android/promotion_hint_aggregator.h"
#include "media/gpu/media_gpu_export.h"

namespace base {
class SequencedTaskRunner;
}

namespace media {

class AndroidOverlay;
struct AVDASurfaceBundle;
class CodecImage;

// Object that lives on the GPU thread that knows about all CodecImages that
// share the same bundle.  We are responsible for keeping the surface bundle
// around while any image is using it.  If the overlay is destroyed, then we
// unback the images.
//
// We're held by the codec images that use us, so that we last at least as long
// as each of them.  We might also be held by the VideoFrameFactory, if it's
// going to add new images to the group.
//
// Note that this class must be constructed on the thread on which the surface
// bundle (and overlay) may be accessed.  All other methods will run on the
// provided task runner.
class MEDIA_GPU_EXPORT CodecImageGroup
    : public base::RefCountedThreadSafe<CodecImageGroup> {
 public:
  // NOTE: Construction happens on the correct thread to access |bundle| and
  // any overlay it contains.  All other access to this class will happen on
  // |task_runner|, including destruction.
  CodecImageGroup(scoped_refptr<base::SequencedTaskRunner> task_runner,
                  scoped_refptr<AVDASurfaceBundle> bundle);

  // Set the callback that we'll notify when any image is destroyed.
  void SetDestructionCb(CodecImage::DestructionCb destruction_cb);

  // Notify us that |image| uses |surface_bundle_|.
  void AddCodecImage(CodecImage* image);

 protected:
  virtual ~CodecImageGroup();
  friend class base::RefCountedThreadSafe<CodecImageGroup>;
  friend class base::DeleteHelper<CodecImageGroup>;

  // Notify us that |image| has been destroyed.
  void OnCodecImageDestroyed(CodecImage* image);

  // Notify us that our overlay surface has been destroyed.
  void OnSurfaceDestroyed(AndroidOverlay*);

 private:
  // Remember that this lives on some other thread.  Do not actually use it.
  scoped_refptr<AVDASurfaceBundle> surface_bundle_;

  // All the images that use |surface_bundle_|.
  std::unordered_set<CodecImage*> images_;

  // We'll forward CodecImage destructions to |destruction_cb_|.
  CodecImage::DestructionCb destruction_cb_;

  base::WeakPtrFactory<CodecImageGroup> weak_this_factory_;
};

}  // namespace media

#endif  // MEDIA_GPU_ANDROID_CODEC_IMAGE_H_
