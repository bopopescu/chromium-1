// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/assistant_service_metrics_provider.h"

#include "base/metrics/histogram_macros.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/arc/arc_prefs.h"
#include "components/prefs/pref_service.h"

AssistantServiceMetricsProvider::AssistantServiceMetricsProvider() = default;
AssistantServiceMetricsProvider::~AssistantServiceMetricsProvider() = default;

void AssistantServiceMetricsProvider::ProvideCurrentSessionData(
    metrics::ChromeUserMetricsExtension* uma_proto_unused) {
  if (arc::IsAssistantAllowedForProfile(
          ProfileManager::GetActiveUserProfile()) !=
      ash::mojom::AssistantAllowedState::ALLOWED) {
    return;
  }

  UMA_HISTOGRAM_BOOLEAN(
      "Assistant.ServiceEnabledUserCount",
      ProfileManager::GetActiveUserProfile()->GetPrefs()->GetBoolean(
          arc::prefs::kVoiceInteractionEnabled));
}
