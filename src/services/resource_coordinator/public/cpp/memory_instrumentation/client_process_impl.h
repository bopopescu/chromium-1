// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_MEMORY_INSTRUMENTATION_CLIENT_PROCESS_IMPL_H_
#define SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_MEMORY_INSTRUMENTATION_CLIENT_PROCESS_IMPL_H_

#include "base/compiler_specific.h"
#include "base/component_export.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/memory_dump_request_args.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/coordinator.h"
#include "services/resource_coordinator/public/mojom/memory_instrumentation/memory_instrumentation.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace memory_instrumentation {

class TracingObserver;

// This is the bridge between MemoryDumpManager and the Coordinator service.
// This indirection is needed to avoid a dependency from //base, where
// MemoryDumpManager lives, to //services, where the Coordinator service lives.
//
// This cannot just be implemented by the Coordinator service, because there is
// no Coordinator service in child processes. So, in a child process, the
// local dump manager remotely connects to the Coordinator service. In the
// browser process, it locally connects to the Coordinator service.
class COMPONENT_EXPORT(RESOURCE_COORDINATOR_PUBLIC_MEMORY_INSTRUMENTATION)
    ClientProcessImpl : public mojom::ClientProcess {
 public:
  struct COMPONENT_EXPORT(
      RESOURCE_COORDINATOR_PUBLIC_MEMORY_INSTRUMENTATION) Config {
   public:
    Config(service_manager::Connector* connector,
           const std::string& service_name,
           mojom::ProcessType process_type);
    ~Config();

    service_manager::Connector* const connector;
    Coordinator* coordinator_for_testing;
    const std::string service_name;
    const mojom::ProcessType process_type;
  };

  static void CreateInstance(const Config& config);

 private:
  friend std::default_delete<ClientProcessImpl>;  // For testing
  friend class MemoryTracingIntegrationTest;

  ClientProcessImpl(const Config& config);
  ~ClientProcessImpl() override;

  // Implements base::trace_event::MemoryDumpManager::RequestGlobalDumpCallback.
  // This function will be called by the MemoryDumpScheduler::OnTick.
  void RequestGlobalMemoryDump_NoCallback(
      base::trace_event::MemoryDumpType type,
      base::trace_event::MemoryDumpLevelOfDetail level_of_detail);

  // mojom::ClientProcess implementation. The Coordinator calls this.
  void RequestChromeMemoryDump(
      const base::trace_event::MemoryDumpRequestArgs& args,
      RequestChromeMemoryDumpCallback callback) override;

  // Callback passed to base::MemoryDumpManager::CreateProcessDump().
  void OnChromeMemoryDumpDone(
      bool success,
      uint64_t dump_guid,
      std::unique_ptr<base::trace_event::ProcessMemoryDump>);

  // mojom::ClientProcess implementation. The Coordinator calls this.
  void RequestOSMemoryDump(mojom::MemoryMapOption mmap_option,
                           const std::vector<base::ProcessId>& ids,
                           RequestOSMemoryDumpCallback callback) override;

  struct OSMemoryDumpArgs {
    OSMemoryDumpArgs();
    OSMemoryDumpArgs(OSMemoryDumpArgs&&);
    ~OSMemoryDumpArgs();
    mojom::MemoryMapOption mmap_option;
    std::vector<base::ProcessId> pids;
    RequestOSMemoryDumpCallback callback;
  };
  void PerformOSMemoryDump(OSMemoryDumpArgs args);

  // Map containing pending chrome memory callbacks indexed by dump guid.
  // This must be destroyed after |binding_|.
  std::map<uint64_t, RequestChromeMemoryDumpCallback> pending_chrome_callbacks_;

  // On macOS, we must wait for the most recent RequestChromeMemoryDumpCallback
  // to complete before running the OS calculations. The key to this map is the
  // dump_guid of that RequestChromeMemoryDumpCallback, the value a vector of
  // callbacks to calculate and run. For more details, see
  // https://bugs.chromium.org/p/chromium/issues/detail?id=812346#c16.
  std::map<uint64_t, std::vector<OSMemoryDumpArgs>>
      delayed_os_memory_dump_callbacks_;
  base::Optional<uint64_t> most_recent_chrome_memory_dump_guid_;

  mojom::CoordinatorPtr coordinator_;
  mojo::Binding<mojom::ClientProcess> binding_;
  const mojom::ProcessType process_type_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // TODO(ssid): This should be moved to coordinator instead of clients once we
  // have the whole chrome dumps sent via mojo, crbug.com/728199.
  std::unique_ptr<TracingObserver> tracing_observer_;

  DISALLOW_COPY_AND_ASSIGN(ClientProcessImpl);
};

}  // namespace memory_instrumentation

#endif  // SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_MEMORY_INSTRUMENTATION_CLIENT_PROCESS_IMPL_H_
