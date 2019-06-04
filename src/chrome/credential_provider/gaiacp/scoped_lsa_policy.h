// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_CREDENTIAL_PROVIDER_GAIACP_SCOPED_LSA_POLICY_H_
#define CHROME_CREDENTIAL_PROVIDER_GAIACP_SCOPED_LSA_POLICY_H_

#include "base/callback.h"
#include "base/win/windows_types.h"

struct _UNICODE_STRING;

namespace credential_provider {

class FakeScopedLsaPolicyFactory;

class ScopedLsaPolicy {
 public:
  static std::unique_ptr<ScopedLsaPolicy> Create(ACCESS_MASK mask);

  virtual ~ScopedLsaPolicy();

  // Methods to store, retrieve, and remove private keyed data.  This data
  // is stored in protected memory in the OS that required SYSTEM account
  // to decrypt.
  virtual HRESULT StorePrivateData(const wchar_t* key, const wchar_t* value);
  virtual HRESULT RemovePrivateData(const wchar_t* key);
  virtual HRESULT RetrievePrivateData(const wchar_t* key,
                                      wchar_t* value,
                                      size_t length);

  // Adds the given right to the given user.
  virtual HRESULT AddAccountRights(PSID sid, const wchar_t* right);

  // Removes the user account from the system.
  virtual HRESULT RemoveAccount(PSID sid);

  // Initializes an LSA_UNICODE_STRING string from the given wide string.
  // A copy of the wide string is not made, so |lsa_string| must outlive
  // |string|.
  static void InitLsaString(const wchar_t* string, _UNICODE_STRING* lsa_string);

  // Set the function used to create instances of this class in tests.  This
  // is used in the Create() static function.
  using CreatorFunc = decltype(Create);
  using CreatorCallback = base::RepeatingCallback<CreatorFunc>;
  static void SetCreatorForTesting(CreatorCallback creator);

 protected:
  explicit ScopedLsaPolicy(ACCESS_MASK mask);

  bool IsValid() const;

 private:
  friend class FakeScopedLsaPolicyFactory;

  LSA_HANDLE handle_;

  // Gets storage of the function pointer used to create instances of this
  // class for tests.
  static CreatorCallback* GetCreatorCallbackStorage();
};

}  // namespace credential_provider

#endif  // CHROME_CREDENTIAL_PROVIDER_GAIACP_SCOPED_LSA_POLICY_H_
