// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/provision_fetcher_impl.h"

#include "content/public/browser/provision_fetcher_factory.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace content {

// static
void ProvisionFetcherImpl::Create(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    media::mojom::ProvisionFetcherRequest request) {
  DCHECK(url_loader_factory);
  mojo::MakeStrongBinding(
      std::make_unique<ProvisionFetcherImpl>(
          CreateProvisionFetcher(std::move(url_loader_factory))),
      std::move(request));
}

ProvisionFetcherImpl::ProvisionFetcherImpl(
    std::unique_ptr<media::ProvisionFetcher> provision_fetcher)
    : provision_fetcher_(std::move(provision_fetcher)), weak_factory_(this) {
  DVLOG(1) << __FUNCTION__;
}

ProvisionFetcherImpl::~ProvisionFetcherImpl() {}

void ProvisionFetcherImpl::Retrieve(const std::string& default_url,
                                    const std::string& request_data,
                                    RetrieveCallback callback) {
  DVLOG(1) << __FUNCTION__ << ": " << default_url;
  provision_fetcher_->Retrieve(
      default_url, request_data,
      base::Bind(&ProvisionFetcherImpl::OnResponse, weak_factory_.GetWeakPtr(),
                 base::Passed(&callback)));
}

void ProvisionFetcherImpl::OnResponse(RetrieveCallback callback,
                                      bool success,
                                      const std::string& response) {
  DVLOG(1) << __FUNCTION__ << ": " << success;
  std::move(callback).Run(success, response);
}

}  // namespace content
