// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/chrome_views_delegate.h"

#include "base/environment.h"
#include "base/nix/xdg_util.h"
#include "chrome/browser/ui/views/native_widget_factory.h"
#include "chrome/common/channel_info.h"
#include "chrome/grit/chrome_unscaled_resources.h"
#include "components/version_info/channel.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/views/linux_ui/linux_ui.h"

#if defined(OS_WEBOS)
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#endif

namespace {

bool IsDesktopEnvironmentUnity() {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  base::nix::DesktopEnvironment desktop_env =
      base::nix::GetDesktopEnvironment(env.get());
  return desktop_env == base::nix::DESKTOP_ENVIRONMENT_UNITY;
}

int GetWindowIconResourceId() {
#if defined(GOOGLE_CHROME_BUILD)
  switch (chrome::GetChannel()) {
    case version_info::Channel::DEV:
      return IDR_PRODUCT_LOGO_128_DEV;
    case version_info::Channel::BETA:
      return IDR_PRODUCT_LOGO_128_BETA;
    default:
      break;
  }
#endif
  return IDR_PRODUCT_LOGO_128;
}

}  // namespace

views::NativeWidget* ChromeViewsDelegate::CreateNativeWidget(
    views::Widget::InitParams* params,
    views::internal::NativeWidgetDelegate* delegate) {
  NativeWidgetType native_widget_type =
      (params->parent && params->type != views::Widget::InitParams::TYPE_MENU &&
       params->type != views::Widget::InitParams::TYPE_TOOLTIP)
          ? NativeWidgetType::NATIVE_WIDGET_AURA
          : NativeWidgetType::DESKTOP_NATIVE_WIDGET_AURA;
#if defined(OS_WEBOS)
  // On WebOS we should use aura native window for all widgets.
  // LSM supports only fullscreen native window and all popups and menu
  // must created as aura native widget. ozone-wayland crashes if desktop native
  // popup window will not have parent.
  if (!params->parent && !params->context)
    params->parent = aura::Env::GetRootWindow();
  native_widget_type = NativeWidgetType::NATIVE_WIDGET_AURA;
#endif
  return ::CreateNativeWidget(native_widget_type, params, delegate);
}

gfx::ImageSkia* ChromeViewsDelegate::GetDefaultWindowIcon() const {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  return rb.GetImageSkiaNamed(GetWindowIconResourceId());
}

bool ChromeViewsDelegate::WindowManagerProvidesTitleBar(bool maximized) {
  // On Ubuntu Unity, the system always provides a title bar for
  // maximized windows.
  //
  // TODO(thomasanderson,crbug.com/784010): Consider using the
  // _UNITY_SHELL wm hint when support for Ubuntu Trusty is dropped.
  if (!maximized)
    return false;
  static bool is_desktop_environment_unity = IsDesktopEnvironmentUnity();
  return is_desktop_environment_unity;
}
