// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LOCAL_DISCOVERY_ENDPOINT_RESOLVER_H_
#define CHROME_BROWSER_LOCAL_DISCOVERY_ENDPOINT_RESOLVER_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/local_discovery/service_discovery_client.h"

namespace net {
class HostPortPair;
class IPAddress;
class IPEndPoint;
}

namespace local_discovery {

class ServiceDiscoverySharedClient;

class EndpointResolver {
 public:
  using ResultCallback =
      base::OnceCallback<void(const net::IPEndPoint& endpoint)>;

  EndpointResolver();
  ~EndpointResolver();

  void Start(const std::string& service_name, ResultCallback callback);

  void Start(const net::HostPortPair& address, ResultCallback callback);

 private:
  void ServiceResolveComplete(ResultCallback callback,
                              ServiceResolver::RequestStatus result,
                              const ServiceDescription& description);

  void DomainResolveComplete(uint16_t port,
                             ResultCallback callback,
                             bool success,
                             const net::IPAddress& address_ipv4,
                             const net::IPAddress& address_ipv6);

 private:
  scoped_refptr<ServiceDiscoverySharedClient> service_discovery_client_;
  std::unique_ptr<ServiceResolver> service_resolver_;
  std::unique_ptr<LocalDomainResolver> domain_resolver_;

  DISALLOW_COPY_AND_ASSIGN(EndpointResolver);
};

}  // namespace local_discovery

#endif  // CHROME_BROWSER_LOCAL_DISCOVERY_ENDPOINT_RESOLVER_H_
