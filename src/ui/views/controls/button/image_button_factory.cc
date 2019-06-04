// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "ui/views/controls/button/image_button_factory.h"

#include "ui/gfx/color_utils.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/painter.h"

namespace views {

namespace {

void ConfigureVectorImageButton(ImageButton* button) {
  button->SetInkDropMode(Button::InkDropMode::ON);
  button->set_has_ink_drop_action_on_click(true);
  button->SetImageAlignment(ImageButton::ALIGN_CENTER,
                            ImageButton::ALIGN_MIDDLE);
  button->SetFocusPainter(nullptr);
  button->SetBorder(CreateEmptyBorder(
      LayoutProvider::Get()->GetInsetsMetric(INSETS_VECTOR_IMAGE_BUTTON)));
}

}  // namespace

ImageButton* CreateVectorImageButton(ButtonListener* listener) {
  ImageButton* button = new ImageButton(listener);
  ConfigureVectorImageButton(button);
  return button;
}

ToggleImageButton* CreateVectorToggleImageButton(ButtonListener* listener) {
  ToggleImageButton* button = new ToggleImageButton(listener);
  ConfigureVectorImageButton(button);
  return button;
}

void SetImageFromVectorIcon(ImageButton* button,
                            const gfx::VectorIcon& icon,
                            SkColor related_text_color) {
  SetImageFromVectorIcon(button, icon, GetDefaultSizeOfVectorIcon(icon),
                         related_text_color);
}

void SetImageFromVectorIcon(ImageButton* button,
                            const gfx::VectorIcon& icon,
                            int dip_size,
                            SkColor related_text_color) {
  const SkColor icon_color =
      color_utils::DeriveDefaultIconColor(related_text_color);
  const SkColor disabled_color =
      SkColorSetA(icon_color, gfx::kDisabledControlAlpha);
  const gfx::ImageSkia& normal_image =
      gfx::CreateVectorIcon(icon, dip_size, icon_color);
  const gfx::ImageSkia& disabled_image =
      gfx::CreateVectorIcon(icon, dip_size, disabled_color);

  button->SetImage(Button::STATE_NORMAL, normal_image);
  button->SetImage(Button::STATE_DISABLED, disabled_image);
  button->set_ink_drop_base_color(icon_color);
}

void SetToggledImageFromVectorIcon(ToggleImageButton* button,
                                   const gfx::VectorIcon& icon,
                                   int dip_size,
                                   SkColor related_text_color) {
  const SkColor icon_color =
      color_utils::DeriveDefaultIconColor(related_text_color);
  const SkColor disabled_color =
      SkColorSetA(icon_color, gfx::kDisabledControlAlpha);
  const gfx::ImageSkia normal_image =
      gfx::CreateVectorIcon(icon, dip_size, icon_color);
  const gfx::ImageSkia disabled_image =
      gfx::CreateVectorIcon(icon, dip_size, disabled_color);

  button->SetToggledImage(Button::STATE_NORMAL, &normal_image);
  button->SetToggledImage(Button::STATE_DISABLED, &disabled_image);
}

}  // views
