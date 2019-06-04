// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_COMPONENT_UPDATER_TIMER_UPDATE_SCHEDULER_H_
#define COMPONENTS_COMPONENT_UPDATER_TIMER_UPDATE_SCHEDULER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "components/component_updater/timer.h"
#include "components/component_updater/update_scheduler.h"

namespace component_updater {

// Scheduler that uses base::Timer to schedule updates.
class TimerUpdateScheduler : public UpdateScheduler {
 public:
  TimerUpdateScheduler();
  ~TimerUpdateScheduler() override;

  // UpdateScheduler:
  void Schedule(const base::TimeDelta& initial_delay,
                const base::TimeDelta& delay,
                const UserTask& user_task,
                const OnStopTaskCallback& on_stop) override;
  void Stop() override;

 private:
  Timer timer_;
  base::RepeatingClosure user_task_;

  DISALLOW_COPY_AND_ASSIGN(TimerUpdateScheduler);
};

}  // namespace component_updater

#endif  // COMPONENTS_COMPONENT_UPDATER_TIMER_UPDATE_SCHEDULER_H_
