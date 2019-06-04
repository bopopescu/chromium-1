// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCESSIBILITY_ACCESSIBILITY_CURSOR_RING_LAYER_H_
#define ASH_ACCESSIBILITY_ACCESSIBILITY_CURSOR_RING_LAYER_H_

#include "ash/accessibility/accessibility_focus_ring.h"
#include "ash/accessibility/focus_ring_layer.h"
#include "ash/ash_export.h"

namespace ash {

// A subclass of FocusRingLayer that highlights the mouse cursor while it's
// moving, to make it easier to find visually.
class ASH_EXPORT AccessibilityCursorRingLayer : public FocusRingLayer {
 public:
  AccessibilityCursorRingLayer(AccessibilityLayerDelegate* delegate,
                               int red,
                               int green,
                               int blue);
  ~AccessibilityCursorRingLayer() override;

  // Create the layer and update its bounds and position in the hierarchy.
  void Set(const gfx::Point& location);

 private:
  // ui::LayerDelegate overrides:
  void OnPaintLayer(const ui::PaintContext& context) override;

  // The current location.
  gfx::Point location_;

  int red_;
  int green_;
  int blue_;

  DISALLOW_COPY_AND_ASSIGN(AccessibilityCursorRingLayer);
};

}  // namespace ash

#endif  // ASH_ACCESSIBILITY_ACCESSIBILITY_CURSOR_RING_LAYER_H_
