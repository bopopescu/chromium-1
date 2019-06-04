// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/mac/seatbelt_extension.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "sandbox/mac/seatbelt_extension_token.h"

// libsandbox private API.
extern "C" {
extern const char* APP_SANDBOX_READ;

int64_t sandbox_extension_consume(const char* token);
int sandbox_extension_release(int64_t handle);
char* sandbox_extension_issue_file(const char* type,
                                   const char* path,
                                   uint32_t flags);
}

namespace sandbox {

SeatbeltExtension::~SeatbeltExtension() {
  DCHECK(token_.empty() && handle_ == 0)
      << "A SeatbeltExtension must be consumed permanently or revoked.";
}

// static
std::unique_ptr<SeatbeltExtensionToken> SeatbeltExtension::Issue(
    SeatbeltExtension::Type type,
    const std::string& resource) {
  char* token = IssueToken(type, resource);
  if (!token)
    return nullptr;
  return base::WrapUnique(new SeatbeltExtensionToken(token));
}

// static
std::unique_ptr<SeatbeltExtension> SeatbeltExtension::FromToken(
    SeatbeltExtensionToken token) {
  if (token.token_.empty())
    return nullptr;
  return base::WrapUnique(new SeatbeltExtension(token.token_));
}

bool SeatbeltExtension::Consume() {
  DCHECK(!token_.empty());
  handle_ = sandbox_extension_consume(token_.c_str());
  return handle_ > 0;
}

bool SeatbeltExtension::ConsumePermanently() {
  bool rv = Consume();
  handle_ = 0;
  token_.clear();
  return rv;
}

bool SeatbeltExtension::Revoke() {
  int rv = sandbox_extension_release(handle_);
  handle_ = 0;
  token_.clear();
  return rv == 0;
}

SeatbeltExtension::SeatbeltExtension(const std::string& token)
    : token_(token), handle_(0) {}

// static
// The token returned by libsandbox is an opaque string generated by the kernel.
// The string contains all the information about the extension (class and
// resource), which is then SHA1 hashed with a salt only known to the kernel.
// In this way, the kernel does not track issued tokens, it merely validates
// them on consumption.
char* SeatbeltExtension::IssueToken(SeatbeltExtension::Type type,
                                    const std::string& resource) {
  switch (type) {
    case FILE_READ:
      return sandbox_extension_issue_file(APP_SANDBOX_READ, resource.c_str(),
                                          0);
    default:
      NOTREACHED();
      return nullptr;
  }
}

}  // namespace sandbox
