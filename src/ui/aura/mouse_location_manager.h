// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MOUSE_LOCATION_MANAGER_H_
#define UI_AURA_MOUSE_LOCATION_MANAGER_H_

#include "base/atomicops.h"
#include "base/macros.h"
#include "mojo/public/cpp/system/buffer.h"

namespace gfx {
class Point;
}

namespace aura {

// Manages a shared memory buffer that stores the mouse location. It is
// expected Env calls SetMouseLocation() any time the location of the mouse
// changes.
class MouseLocationManager {
 public:
  MouseLocationManager();
  ~MouseLocationManager();

  // Sets the current mouse location to |point|. Atomically writes the location
  // to shared memory. |point| should be in screen-coord and DIP.
  void SetMouseLocation(const gfx::Point& point_in_dip);

  // Returns a read-only handle to the shared memory which contains the global
  // mouse position. Each call returns a new handle.
  mojo::ScopedSharedBufferHandle GetMouseLocationMemory();

 private:
  base::subtle::Atomic32* mouse_location_memory() {
    return reinterpret_cast<base::subtle::Atomic32*>(
        mouse_location_mapping_.get());
  }

  // The current location of the mouse. This is always kept up to date so we
  // can atomically write this to |mouse_location_memory()| once it is created.
  base::subtle::Atomic32 current_mouse_location_ = 0;

  // A handle to a shared memory buffer that is one 32 bit integer long. We
  // share this with any client as the same user. This buffer is lazily
  // created on the first access.
  mojo::ScopedSharedBufferHandle mouse_location_handle_;

  // The one int32 in |mouse_location_handle_|. When we write to this
  // location, we must always write to it atomically. (On the other side of the
  // mojo connection, this data must be read atomically.)
  mojo::ScopedSharedBufferMapping mouse_location_mapping_;

  DISALLOW_COPY_AND_ASSIGN(MouseLocationManager);
};

}  // namespace aura

#endif  // UI_AURA_MOUSE_LOCATION_MANAGER_H_
