// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/accessibility/tray_accessibility.h"

#include <memory>
#include <utility>

#include "ash/accessibility/accessibility_controller.h"
#include "ash/accessibility/accessibility_delegate.h"
#include "ash/magnifier/docked_magnifier_controller.h"
#include "ash/public/cpp/ash_features.h"
#include "ash/public/cpp/ash_view_ids.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/model/system_tray_model.h"
#include "ash/system/tray/hover_highlight_view.h"
#include "ash/system/tray/tray_detailed_view.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ash/system/tray/tray_utils.h"
#include "ash/system/tray/tri_view.h"
#include "base/metrics/user_metrics.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/image/image.h"
#include "ui/views/controls/separator.h"

namespace ash {
namespace {

enum AccessibilityState {
  A11Y_NONE = 0,
  A11Y_SPOKEN_FEEDBACK = 1 << 0,
  A11Y_HIGH_CONTRAST = 1 << 1,
  A11Y_SCREEN_MAGNIFIER = 1 << 2,
  A11Y_LARGE_CURSOR = 1 << 3,
  A11Y_AUTOCLICK = 1 << 4,
  A11Y_VIRTUAL_KEYBOARD = 1 << 5,
  A11Y_MONO_AUDIO = 1 << 6,
  A11Y_CARET_HIGHLIGHT = 1 << 7,
  A11Y_HIGHLIGHT_MOUSE_CURSOR = 1 << 8,
  A11Y_HIGHLIGHT_KEYBOARD_FOCUS = 1 << 9,
  A11Y_STICKY_KEYS = 1 << 10,
  A11Y_SELECT_TO_SPEAK = 1 << 11,
  A11Y_DOCKED_MAGNIFIER = 1 << 12,
  A11Y_DICTATION = 1 << 13,
};

}  // namespace

namespace tray {

////////////////////////////////////////////////////////////////////////////////
// ash::tray::AccessibilityDetailedView

AccessibilityDetailedView::AccessibilityDetailedView(
    DetailedViewDelegate* delegate)
    : TrayDetailedView(delegate) {
  Reset();
  AppendAccessibilityList();
  CreateTitleRow(IDS_ASH_STATUS_TRAY_ACCESSIBILITY_TITLE);
  Layout();
}

void AccessibilityDetailedView::OnAccessibilityStatusChanged() {
  AccessibilityDelegate* delegate = Shell::Get()->accessibility_delegate();
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();

  spoken_feedback_enabled_ = controller->IsSpokenFeedbackEnabled();
  TrayPopupUtils::UpdateCheckMarkVisibility(spoken_feedback_view_,
                                            spoken_feedback_enabled_);

  select_to_speak_enabled_ = controller->IsSelectToSpeakEnabled();
  TrayPopupUtils::UpdateCheckMarkVisibility(select_to_speak_view_,
                                            select_to_speak_enabled_);

  if (dictation_view_) {
    dictation_enabled_ = controller->IsDictationEnabled();
    TrayPopupUtils::UpdateCheckMarkVisibility(dictation_view_,
                                              dictation_enabled_);
  }

  high_contrast_enabled_ = controller->IsHighContrastEnabled();
  TrayPopupUtils::UpdateCheckMarkVisibility(high_contrast_view_,
                                            high_contrast_enabled_);

  screen_magnifier_enabled_ = delegate->IsMagnifierEnabled();
  TrayPopupUtils::UpdateCheckMarkVisibility(screen_magnifier_view_,
                                            screen_magnifier_enabled_);

  if (features::IsDockedMagnifierEnabled()) {
    docked_magnifier_enabled_ =
        Shell::Get()->docked_magnifier_controller()->GetEnabled();
    TrayPopupUtils::UpdateCheckMarkVisibility(docked_magnifier_view_,
                                              docked_magnifier_enabled_);
  }

  autoclick_enabled_ = controller->IsAutoclickEnabled();
  TrayPopupUtils::UpdateCheckMarkVisibility(autoclick_view_,
                                            autoclick_enabled_);

  virtual_keyboard_enabled_ = controller->IsVirtualKeyboardEnabled();
  TrayPopupUtils::UpdateCheckMarkVisibility(virtual_keyboard_view_,
                                            virtual_keyboard_enabled_);

  large_cursor_enabled_ = controller->IsLargeCursorEnabled();
  TrayPopupUtils::UpdateCheckMarkVisibility(large_cursor_view_,
                                            large_cursor_enabled_);

  mono_audio_enabled_ = controller->IsMonoAudioEnabled();
  TrayPopupUtils::UpdateCheckMarkVisibility(mono_audio_view_,
                                            mono_audio_enabled_);

  caret_highlight_enabled_ = controller->IsCaretHighlightEnabled();
  TrayPopupUtils::UpdateCheckMarkVisibility(caret_highlight_view_,
                                            caret_highlight_enabled_);

  highlight_mouse_cursor_enabled_ = controller->IsCursorHighlightEnabled();
  TrayPopupUtils::UpdateCheckMarkVisibility(highlight_mouse_cursor_view_,
                                            highlight_mouse_cursor_enabled_);

  if (highlight_keyboard_focus_view_) {
    highlight_keyboard_focus_enabled_ = controller->IsFocusHighlightEnabled();
    TrayPopupUtils::UpdateCheckMarkVisibility(
        highlight_keyboard_focus_view_, highlight_keyboard_focus_enabled_);
  }

  sticky_keys_enabled_ = controller->IsStickyKeysEnabled();
  TrayPopupUtils::UpdateCheckMarkVisibility(sticky_keys_view_,
                                            sticky_keys_enabled_);
}

void AccessibilityDetailedView::AppendAccessibilityList() {
  CreateScrollableList();

  AccessibilityDelegate* delegate = Shell::Get()->accessibility_delegate();
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();

  spoken_feedback_enabled_ = controller->IsSpokenFeedbackEnabled();
  spoken_feedback_view_ = AddScrollListCheckableItem(
      kSystemMenuAccessibilityChromevoxIcon,
      l10n_util::GetStringUTF16(
          IDS_ASH_STATUS_TRAY_ACCESSIBILITY_SPOKEN_FEEDBACK),
      spoken_feedback_enabled_);

  select_to_speak_enabled_ = controller->IsSelectToSpeakEnabled();
  select_to_speak_view_ = AddScrollListCheckableItem(
      kSystemMenuAccessibilitySelectToSpeakIcon,
      l10n_util::GetStringUTF16(
          IDS_ASH_STATUS_TRAY_ACCESSIBILITY_SELECT_TO_SPEAK),
      select_to_speak_enabled_);

  dictation_enabled_ = controller->IsDictationEnabled();
  dictation_view_ = AddScrollListCheckableItem(
      kDictationMenuIcon,
      l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_ACCESSIBILITY_DICTATION),
      dictation_enabled_);

  high_contrast_enabled_ = controller->IsHighContrastEnabled();
  high_contrast_view_ = AddScrollListCheckableItem(
      kSystemMenuAccessibilityContrastIcon,
      l10n_util::GetStringUTF16(
          IDS_ASH_STATUS_TRAY_ACCESSIBILITY_HIGH_CONTRAST_MODE),
      high_contrast_enabled_);

  screen_magnifier_enabled_ = delegate->IsMagnifierEnabled();
  screen_magnifier_view_ = AddScrollListCheckableItem(
      kSystemMenuAccessibilityFullscreenMagnifierIcon,
      l10n_util::GetStringUTF16(
          IDS_ASH_STATUS_TRAY_ACCESSIBILITY_SCREEN_MAGNIFIER),
      screen_magnifier_enabled_);

  if (features::IsDockedMagnifierEnabled()) {
    docked_magnifier_enabled_ =
        Shell::Get()->docked_magnifier_controller()->GetEnabled();
    docked_magnifier_view_ = AddScrollListCheckableItem(
        kSystemMenuAccessibilityDockedMagnifierIcon,
        l10n_util::GetStringUTF16(
            IDS_ASH_STATUS_TRAY_ACCESSIBILITY_DOCKED_MAGNIFIER),
        docked_magnifier_enabled_);
  }

  autoclick_enabled_ = controller->IsAutoclickEnabled();
  autoclick_view_ = AddScrollListCheckableItem(
      kSystemMenuAccessibilityAutoClickIcon,
      l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_ACCESSIBILITY_AUTOCLICK),
      autoclick_enabled_);
  autoclick_view_->set_id(ash::VIEW_ID_ACCESSIBILITY_AUTOCLICK);
  autoclick_view_->right_view()->set_id(
      ash::VIEW_ID_ACCESSIBILITY_AUTOCLICK_ENABLED);

  virtual_keyboard_enabled_ = controller->IsVirtualKeyboardEnabled();
  virtual_keyboard_view_ = AddScrollListCheckableItem(
      kSystemMenuKeyboardIcon,
      l10n_util::GetStringUTF16(
          IDS_ASH_STATUS_TRAY_ACCESSIBILITY_VIRTUAL_KEYBOARD),
      virtual_keyboard_enabled_);

  scroll_content()->AddChildView(CreateListSubHeaderSeparator());

  AddScrollListSubHeader(IDS_ASH_STATUS_TRAY_ACCESSIBILITY_ADDITIONAL_SETTINGS);

  large_cursor_enabled_ = controller->IsLargeCursorEnabled();
  large_cursor_view_ = AddScrollListCheckableItem(
      l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_ACCESSIBILITY_LARGE_CURSOR),
      large_cursor_enabled_);

  mono_audio_enabled_ = controller->IsMonoAudioEnabled();
  mono_audio_view_ = AddScrollListCheckableItem(
      l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_ACCESSIBILITY_MONO_AUDIO),
      mono_audio_enabled_);

  caret_highlight_enabled_ = controller->IsCaretHighlightEnabled();
  caret_highlight_view_ = AddScrollListCheckableItem(
      l10n_util::GetStringUTF16(
          IDS_ASH_STATUS_TRAY_ACCESSIBILITY_CARET_HIGHLIGHT),
      caret_highlight_enabled_);

  highlight_mouse_cursor_enabled_ = controller->IsCursorHighlightEnabled();
  highlight_mouse_cursor_view_ = AddScrollListCheckableItem(
      l10n_util::GetStringUTF16(
          IDS_ASH_STATUS_TRAY_ACCESSIBILITY_HIGHLIGHT_MOUSE_CURSOR),
      highlight_mouse_cursor_enabled_);

  // Focus highlighting can't be on when spoken feedback is on because
  // ChromeVox does its own focus highlighting.
  if (!spoken_feedback_enabled_) {
    highlight_keyboard_focus_enabled_ = controller->IsFocusHighlightEnabled();
    highlight_keyboard_focus_view_ = AddScrollListCheckableItem(
        l10n_util::GetStringUTF16(
            IDS_ASH_STATUS_TRAY_ACCESSIBILITY_HIGHLIGHT_KEYBOARD_FOCUS),
        highlight_keyboard_focus_enabled_);
  }

  sticky_keys_enabled_ = controller->IsStickyKeysEnabled();
  sticky_keys_view_ = AddScrollListCheckableItem(
      l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_ACCESSIBILITY_STICKY_KEYS),
      sticky_keys_enabled_);
}

void AccessibilityDetailedView::HandleViewClicked(views::View* view) {
  AccessibilityDelegate* delegate = Shell::Get()->accessibility_delegate();
  AccessibilityController* controller =
      Shell::Get()->accessibility_controller();
  using base::RecordAction;
  using base::UserMetricsAction;
  if (view == spoken_feedback_view_) {
    bool new_state = !controller->IsSpokenFeedbackEnabled();
    RecordAction(new_state
                     ? UserMetricsAction("StatusArea_SpokenFeedbackEnabled")
                     : UserMetricsAction("StatusArea_SpokenFeedbackDisabled"));
    controller->SetSpokenFeedbackEnabled(new_state, A11Y_NOTIFICATION_NONE);
  } else if (view == select_to_speak_view_) {
    bool new_state = !controller->IsSelectToSpeakEnabled();
    RecordAction(new_state
                     ? UserMetricsAction("StatusArea_SelectToSpeakEnabled")
                     : UserMetricsAction("StatusArea_SelectToSpeakDisabled"));
    controller->SetSelectToSpeakEnabled(new_state);
  } else if (view == dictation_view_) {
    bool new_state = !controller->IsDictationEnabled();
    RecordAction(new_state ? UserMetricsAction("StatusArea_DictationEnabled")
                           : UserMetricsAction("StatusArea_DictationDisabled"));
    controller->SetDictationEnabled(new_state);
  } else if (view == high_contrast_view_) {
    bool new_state = !controller->IsHighContrastEnabled();
    RecordAction(new_state
                     ? UserMetricsAction("StatusArea_HighContrastEnabled")
                     : UserMetricsAction("StatusArea_HighContrastDisabled"));
    controller->SetHighContrastEnabled(new_state);
  } else if (view == screen_magnifier_view_) {
    RecordAction(delegate->IsMagnifierEnabled()
                     ? UserMetricsAction("StatusArea_MagnifierDisabled")
                     : UserMetricsAction("StatusArea_MagnifierEnabled"));
    delegate->SetMagnifierEnabled(!delegate->IsMagnifierEnabled());
  } else if (features::IsDockedMagnifierEnabled() &&
             view == docked_magnifier_view_) {
    auto* docked_magnifier_controller =
        Shell::Get()->docked_magnifier_controller();
    const bool new_state = !docked_magnifier_controller->GetEnabled();
    RecordAction(new_state
                     ? UserMetricsAction("StatusArea_DockedMagnifierEnabled")
                     : UserMetricsAction("StatusArea_DockedMagnifierDisabled"));
    docked_magnifier_controller->SetEnabled(new_state);
  } else if (large_cursor_view_ && view == large_cursor_view_) {
    bool new_state = !controller->IsLargeCursorEnabled();
    RecordAction(new_state
                     ? UserMetricsAction("StatusArea_LargeCursorEnabled")
                     : UserMetricsAction("StatusArea_LargeCursorDisabled"));
    controller->SetLargeCursorEnabled(new_state);
  } else if (autoclick_view_ && view == autoclick_view_) {
    bool new_state = !controller->IsAutoclickEnabled();
    RecordAction(new_state ? UserMetricsAction("StatusArea_AutoClickEnabled")
                           : UserMetricsAction("StatusArea_AutoClickDisabled"));
    controller->SetAutoclickEnabled(new_state);
  } else if (virtual_keyboard_view_ && view == virtual_keyboard_view_) {
    bool new_state = !controller->IsVirtualKeyboardEnabled();
    RecordAction(new_state
                     ? UserMetricsAction("StatusArea_VirtualKeyboardEnabled")
                     : UserMetricsAction("StatusArea_VirtualKeyboardDisabled"));
    controller->SetVirtualKeyboardEnabled(new_state);
  } else if (caret_highlight_view_ && view == caret_highlight_view_) {
    bool new_state = !controller->IsCaretHighlightEnabled();
    RecordAction(new_state
                     ? UserMetricsAction("StatusArea_CaretHighlightEnabled")
                     : UserMetricsAction("StatusArea_CaretHighlightDisabled"));
    controller->SetCaretHighlightEnabled(new_state);
  } else if (mono_audio_view_ && view == mono_audio_view_) {
    bool new_state = !controller->IsMonoAudioEnabled();
    RecordAction(new_state ? UserMetricsAction("StatusArea_MonoAudioEnabled")
                           : UserMetricsAction("StatusArea_MonoAudioDisabled"));
    controller->SetMonoAudioEnabled(new_state);
  } else if (highlight_mouse_cursor_view_ &&
             view == highlight_mouse_cursor_view_) {
    bool new_state = !controller->IsCursorHighlightEnabled();
    RecordAction(
        new_state
            ? UserMetricsAction("StatusArea_HighlightMouseCursorEnabled")
            : UserMetricsAction("StatusArea_HighlightMouseCursorDisabled"));
    controller->SetCursorHighlightEnabled(new_state);
  } else if (highlight_keyboard_focus_view_ &&
             view == highlight_keyboard_focus_view_) {
    bool new_state = !controller->IsFocusHighlightEnabled();
    RecordAction(
        new_state
            ? UserMetricsAction("StatusArea_HighlightKeyboardFocusEnabled")
            : UserMetricsAction("StatusArea_HighlightKeyboardFocusDisabled"));
    controller->SetFocusHighlightEnabled(new_state);
  } else if (sticky_keys_view_ && view == sticky_keys_view_) {
    bool new_state = !controller->IsStickyKeysEnabled();
    RecordAction(new_state
                     ? UserMetricsAction("StatusArea_StickyKeysEnabled")
                     : UserMetricsAction("StatusArea_StickyKeysDisabled"));
    controller->SetStickyKeysEnabled(new_state);
  }
}

void AccessibilityDetailedView::HandleButtonPressed(views::Button* sender,
                                                    const ui::Event& event) {
  if (sender == help_view_)
    ShowHelp();
  else if (sender == settings_view_)
    ShowSettings();
}

void AccessibilityDetailedView::CreateExtraTitleRowButtons() {
  DCHECK(!help_view_);
  DCHECK(!settings_view_);

  tri_view()->SetContainerVisible(TriView::Container::END, true);

  help_view_ = CreateHelpButton();
  settings_view_ =
      CreateSettingsButton(IDS_ASH_STATUS_TRAY_ACCESSIBILITY_SETTINGS);
  tri_view()->AddView(TriView::Container::END, help_view_);
  tri_view()->AddView(TriView::Container::END, settings_view_);
}

void AccessibilityDetailedView::ShowSettings() {
  if (TrayPopupUtils::CanOpenWebUISettings()) {
    Shell::Get()
        ->system_tray_model()
        ->client_ptr()
        ->ShowAccessibilitySettings();
    CloseBubble();
  }
}

void AccessibilityDetailedView::ShowHelp() {
  if (TrayPopupUtils::CanOpenWebUISettings()) {
    Shell::Get()->system_tray_model()->client_ptr()->ShowAccessibilityHelp();
    CloseBubble();
  }
}

}  // namespace tray
}  // namespace ash
