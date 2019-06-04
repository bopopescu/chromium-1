// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_UTILITY_UTILITY_SERVICE_FACTORY_H_
#define CONTENT_UTILITY_UTILITY_SERVICE_FACTORY_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/sequenced_task_runner.h"
#include "content/child/service_factory.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/mojom/service.mojom.h"

namespace content {

// Customization of ServiceFactory for the utility process. Exposed to the
// browser via the utility process's InterfaceRegistry.
class UtilityServiceFactory : public ServiceFactory {
 public:
  UtilityServiceFactory();
  ~UtilityServiceFactory() override;

  // ServiceFactory overrides:
  void CreateService(
      service_manager::mojom::ServiceRequest request,
      const std::string& name,
      service_manager::mojom::PIDReceiverPtr pid_receiver) override;
  void RegisterServices(ServiceMap* services) override;
  bool HandleServiceRequest(
      const std::string& name,
      service_manager::mojom::ServiceRequest request) override;
  void OnServiceQuit() override;

 private:
  void OnLoadFailed() override;

  void RunNetworkServiceOnIOThread(
      service_manager::mojom::ServiceRequest service_request,
      scoped_refptr<base::SequencedTaskRunner> main_thread_task_runner);
  std::unique_ptr<service_manager::Service> CreateAudioService(
      service_manager::mojom::ServiceRequest request);

  // Allows embedders to register their interface implementations before the
  // network or audio services are created. Used for testing.
  std::unique_ptr<service_manager::BinderRegistry> network_registry_;
  std::unique_ptr<service_manager::BinderRegistry> audio_registry_;

  std::unique_ptr<service_manager::Service> running_service_;

  DISALLOW_COPY_AND_ASSIGN(UtilityServiceFactory);
};

}  // namespace content

#endif  // CONTENT_UTILITY_UTILITY_SERVICE_FACTORY_H_
