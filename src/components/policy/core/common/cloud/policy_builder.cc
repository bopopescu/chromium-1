// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/cloud/policy_builder.h"

#include "base/macros.h"
#include "build/build_config.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"
#include "crypto/signature_creator.h"

namespace em = enterprise_management;

namespace policy {

namespace {

// Signing key test data in DER-encoded PKCS8 format.
const uint8_t kSigningKey[] = {
    0x30, 0x82, 0x01, 0x55, 0x02, 0x01, 0x00, 0x30, 0x0d, 0x06, 0x09, 0x2a,
    0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x04, 0x82,
    0x01, 0x3f, 0x30, 0x82, 0x01, 0x3b, 0x02, 0x01, 0x00, 0x02, 0x41, 0x00,
    0xd9, 0xcd, 0xca, 0xcd, 0xc3, 0xea, 0xbe, 0x72, 0x79, 0x1c, 0x29, 0x37,
    0x39, 0x99, 0x1f, 0xd4, 0xb3, 0x0e, 0xf0, 0x7b, 0x78, 0x77, 0x0e, 0x05,
    0x3b, 0x65, 0x34, 0x12, 0x62, 0xaf, 0xa6, 0x8d, 0x33, 0xce, 0x78, 0xf8,
    0x47, 0x05, 0x1d, 0x98, 0xaa, 0x1b, 0x1f, 0x50, 0x05, 0x5b, 0x3c, 0x19,
    0x3f, 0x80, 0x83, 0x63, 0x63, 0x3a, 0xec, 0xcb, 0x2e, 0x90, 0x4f, 0xf5,
    0x26, 0x76, 0xf1, 0xd5, 0x02, 0x03, 0x01, 0x00, 0x01, 0x02, 0x40, 0x64,
    0x29, 0xc2, 0xd9, 0x6b, 0xfe, 0xf9, 0x84, 0x75, 0x73, 0xe0, 0xf4, 0x77,
    0xb5, 0x96, 0xb0, 0xdf, 0x83, 0xc0, 0x4e, 0x57, 0xf1, 0x10, 0x6e, 0x91,
    0x89, 0x12, 0x30, 0x5e, 0x57, 0xff, 0x14, 0x59, 0x5f, 0x18, 0x86, 0x4e,
    0x4b, 0x17, 0x56, 0xfc, 0x8d, 0x40, 0xdd, 0x74, 0x65, 0xd3, 0xff, 0x67,
    0x64, 0xcb, 0x9c, 0xb4, 0x14, 0x8a, 0x06, 0xb7, 0x13, 0x45, 0x94, 0x16,
    0x7d, 0x3f, 0xe1, 0x02, 0x21, 0x00, 0xf6, 0x0f, 0x31, 0x6d, 0x06, 0xcc,
    0x3b, 0xa0, 0x44, 0x1f, 0xf5, 0xc2, 0x45, 0x2b, 0x10, 0x6c, 0xf9, 0x6f,
    0x8f, 0x87, 0x3d, 0xc0, 0x3b, 0x55, 0x13, 0x37, 0x80, 0xcd, 0x9f, 0xe1,
    0xb7, 0xd9, 0x02, 0x21, 0x00, 0xe2, 0x9a, 0x5f, 0xbf, 0x95, 0x74, 0xb5,
    0x7a, 0x6a, 0xa6, 0x97, 0xbd, 0x75, 0x8c, 0x97, 0x18, 0x24, 0xd6, 0x09,
    0xcd, 0xdc, 0xb5, 0x94, 0xbf, 0xe2, 0x78, 0xaa, 0x20, 0x47, 0x9f, 0x68,
    0x5d, 0x02, 0x21, 0x00, 0xaf, 0x8f, 0x97, 0x8c, 0x5a, 0xd5, 0x4d, 0x95,
    0xc4, 0x05, 0xa9, 0xab, 0xba, 0xfe, 0x46, 0xf1, 0xf9, 0xe7, 0x07, 0x59,
    0x4f, 0x4d, 0xe1, 0x07, 0x8a, 0x76, 0x87, 0x88, 0x2f, 0x13, 0x35, 0xc1,
    0x02, 0x20, 0x24, 0xc3, 0xd9, 0x2f, 0x13, 0x47, 0x99, 0x3e, 0x20, 0x59,
    0xa1, 0x1a, 0xeb, 0x1c, 0x81, 0x53, 0x38, 0x7e, 0xc5, 0x9e, 0x71, 0xe5,
    0xc0, 0x19, 0x95, 0xdb, 0xef, 0xf6, 0x46, 0xc8, 0x95, 0x3d, 0x02, 0x21,
    0x00, 0xaa, 0xb1, 0xff, 0x8a, 0xa2, 0xb2, 0x2b, 0xef, 0x9a, 0x83, 0x3f,
    0xc5, 0xbc, 0xd4, 0x6a, 0x07, 0xe8, 0xc7, 0x0b, 0x2e, 0xd4, 0x0f, 0xf8,
    0x98, 0x68, 0xe1, 0x04, 0xa8, 0x92, 0xd0, 0x10, 0xaa,
};

// SHA256 signature of kSigningKey for "example.com" domain.
const uint8_t kSigningKeySignature[] = {
    0x97, 0xEB, 0x13, 0xE6, 0x6C, 0xE2, 0x7A, 0x2F, 0xC6, 0x6E, 0x68, 0x8F,
    0xED, 0x5B, 0x51, 0x08, 0x27, 0xF0, 0xA5, 0x97, 0x20, 0xEE, 0xE2, 0x9B,
    0x5B, 0x63, 0xA5, 0x9C, 0xAE, 0x41, 0xFD, 0x34, 0xC4, 0x2E, 0xEB, 0x63,
    0x10, 0x80, 0x0C, 0x74, 0x77, 0x6E, 0x34, 0x1C, 0x1B, 0x3B, 0x8E, 0x2A,
    0x3A, 0x7F, 0xF9, 0x73, 0xB6, 0x2B, 0xB6, 0x45, 0xDB, 0x05, 0xE8, 0x5A,
    0x68, 0x36, 0x05, 0x3C, 0x62, 0x3A, 0x6C, 0x64, 0xDB, 0x0E, 0x61, 0xBD,
    0x29, 0x1C, 0x61, 0x4B, 0xE0, 0xDA, 0x07, 0xBA, 0x29, 0x81, 0xF0, 0x90,
    0x58, 0xB8, 0xBB, 0xF4, 0x69, 0xFF, 0x8F, 0x2B, 0x4A, 0x2D, 0x98, 0x51,
    0x37, 0xF5, 0x52, 0xCB, 0xE3, 0xC4, 0x6D, 0xEC, 0xEA, 0x32, 0x2D, 0xDD,
    0xD7, 0xFC, 0x43, 0xC6, 0x54, 0xE1, 0xC1, 0x66, 0x43, 0x37, 0x09, 0xE1,
    0xBF, 0xD1, 0x11, 0xFC, 0xDB, 0xBF, 0xDF, 0x66, 0x53, 0x8F, 0x38, 0x2D,
    0xAA, 0x89, 0xD2, 0x9F, 0x60, 0x90, 0xB7, 0x05, 0xC2, 0x20, 0x82, 0xE6,
    0xE0, 0x57, 0x55, 0xFF, 0x5F, 0xC1, 0x76, 0x66, 0x46, 0xF8, 0x67, 0xB8,
    0x8B, 0x81, 0x53, 0xA9, 0x8B, 0x48, 0x9E, 0x2A, 0xF9, 0x60, 0x57, 0xBA,
    0xD7, 0x52, 0x97, 0x53, 0xF0, 0x2F, 0x78, 0x68, 0x50, 0x18, 0x12, 0x00,
    0x5E, 0x8E, 0x2A, 0x62, 0x0D, 0x48, 0xA9, 0xB5, 0x6B, 0xBC, 0xA0, 0x52,
    0x53, 0xD7, 0x65, 0x23, 0xA4, 0xA5, 0xF5, 0x32, 0x49, 0x2D, 0xB2, 0x77,
    0x2C, 0x66, 0x97, 0xBA, 0x58, 0xE0, 0x16, 0x1C, 0x8C, 0x02, 0x5D, 0xE0,
    0x73, 0x2E, 0xDF, 0xB4, 0x2F, 0x4C, 0xA2, 0x11, 0x26, 0xC1, 0xAF, 0xAC,
    0x73, 0xBC, 0xB6, 0x98, 0xE0, 0x20, 0x61, 0x0E, 0x52, 0x4A, 0x6C, 0x80,
    0xB5, 0x0C, 0x10, 0x80, 0x09, 0x17, 0xF4, 0x9D, 0xFE, 0xB5, 0xFC, 0x63,
    0x9A, 0x80, 0x3F, 0x76,
};

// New signing key test data in DER-encoded PKCS8 format.
const uint8_t kNewSigningKey[] = {
    0x30, 0x82, 0x01, 0x54, 0x02, 0x01, 0x00, 0x30, 0x0d, 0x06, 0x09, 0x2a,
    0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x04, 0x82,
    0x01, 0x3e, 0x30, 0x82, 0x01, 0x3a, 0x02, 0x01, 0x00, 0x02, 0x41, 0x00,
    0x99, 0x98, 0x6b, 0x79, 0x5d, 0x38, 0x33, 0x79, 0x27, 0x0a, 0x2e, 0xb0,
    0x89, 0xba, 0xf8, 0xf6, 0x80, 0xde, 0xb0, 0x79, 0xf2, 0xd4, 0x6d, 0xf7,
    0x3c, 0xa3, 0x97, 0xf6, 0x4a, 0x3c, 0xa5, 0xcc, 0x40, 0x8a, 0xef, 0x59,
    0xaa, 0xc2, 0x82, 0x8f, 0xbc, 0x0d, 0x5b, 0x63, 0xc6, 0xaa, 0x72, 0xe2,
    0xf3, 0x57, 0xdd, 0x74, 0x00, 0xb0, 0x42, 0xd6, 0x27, 0xe7, 0x17, 0x61,
    0x0a, 0xdc, 0xc1, 0xf7, 0x02, 0x03, 0x01, 0x00, 0x01, 0x02, 0x40, 0x34,
    0xcf, 0xc9, 0xb4, 0x73, 0x2f, 0x0d, 0xd3, 0xcc, 0x6e, 0x9d, 0xdb, 0x29,
    0xa0, 0x56, 0x56, 0x3b, 0xbd, 0x56, 0x24, 0xb8, 0x2f, 0xfe, 0x97, 0x92,
    0x0c, 0x16, 0x06, 0x23, 0x44, 0x73, 0x25, 0x1d, 0x65, 0xf4, 0xda, 0x77,
    0xe7, 0x91, 0x2e, 0x91, 0x05, 0x10, 0xc1, 0x1b, 0x39, 0x5e, 0xb2, 0xf7,
    0xbd, 0x14, 0x19, 0xcb, 0x6b, 0xc3, 0xa9, 0xe8, 0x91, 0xf7, 0xa7, 0xa9,
    0x90, 0x08, 0x51, 0x02, 0x21, 0x00, 0xcc, 0x9e, 0x03, 0x54, 0x8f, 0x24,
    0xde, 0x90, 0x25, 0xec, 0x21, 0xaf, 0xe6, 0x27, 0x2a, 0x16, 0x42, 0x74,
    0xda, 0xf8, 0x84, 0xc4, 0x8c, 0x1e, 0x86, 0x12, 0x04, 0x5c, 0x17, 0x01,
    0xea, 0x9d, 0x02, 0x21, 0x00, 0xc0, 0x2a, 0x6c, 0xe9, 0xa1, 0x1a, 0x41,
    0x11, 0x94, 0x50, 0xf7, 0x1a, 0xd3, 0xbc, 0xf3, 0xa2, 0xf8, 0x46, 0xbc,
    0x26, 0x77, 0x78, 0xef, 0xc0, 0x54, 0xec, 0x22, 0x3f, 0x2c, 0x57, 0xe0,
    0xa3, 0x02, 0x20, 0x31, 0xf2, 0xc8, 0xa1, 0x55, 0xa8, 0x0c, 0x64, 0x67,
    0xbd, 0x72, 0xa3, 0xbb, 0xad, 0x07, 0xcb, 0x13, 0x41, 0xef, 0x4a, 0x07,
    0x2e, 0xeb, 0x7d, 0x70, 0x00, 0xe9, 0xeb, 0x88, 0xfa, 0x40, 0xc9, 0x02,
    0x20, 0x3a, 0xe0, 0xc4, 0xde, 0x10, 0x6e, 0x6a, 0xe1, 0x68, 0x00, 0x26,
    0xb6, 0x21, 0x8a, 0x13, 0x5c, 0x2b, 0x96, 0x00, 0xb0, 0x08, 0x8b, 0x15,
    0x6a, 0x68, 0x9a, 0xb1, 0x23, 0x8a, 0x02, 0xa2, 0xe1, 0x02, 0x21, 0x00,
    0xa3, 0xf2, 0x2d, 0x55, 0xc1, 0x6d, 0x40, 0xfa, 0x1d, 0xf7, 0xba, 0x86,
    0xef, 0x50, 0x98, 0xfc, 0xee, 0x09, 0xcc, 0xe7, 0x22, 0xb9, 0x4e, 0x80,
    0x32, 0x1a, 0x6b, 0xb3, 0x5f, 0x35, 0xbd, 0xf3,
};

// SHA256 signature of kNewSigningKey for "example.com" domain.
const uint8_t kNewSigningKeySignature[] = {
    0x70, 0xED, 0x27, 0x42, 0x34, 0x69, 0xB6, 0x47, 0x9E, 0x7C, 0xA0, 0xF0,
    0xE5, 0x0A, 0x49, 0x49, 0x00, 0xDA, 0xBC, 0x70, 0x01, 0xC5, 0x4B, 0xDB,
    0x47, 0xD5, 0xAF, 0xA1, 0xAD, 0xB7, 0xE4, 0xE1, 0xBD, 0x5A, 0x1C, 0x35,
    0x44, 0x5A, 0xAA, 0xDB, 0x27, 0xBA, 0xA4, 0xA9, 0xC8, 0xDD, 0xEC, 0xD6,
    0xEB, 0xFE, 0xDB, 0xE0, 0x03, 0x5C, 0xA6, 0x2E, 0x5A, 0xEC, 0x75, 0x79,
    0xB8, 0x5F, 0x0A, 0xEE, 0x05, 0xB2, 0x61, 0xDC, 0x58, 0xF0, 0xD1, 0xCB,
    0x7B, 0x2A, 0xDB, 0xC1, 0x7C, 0x60, 0xE6, 0x3E, 0x87, 0x02, 0x61, 0xE6,
    0x90, 0xFD, 0x54, 0x65, 0xC7, 0xFF, 0x74, 0x09, 0xD6, 0xAA, 0x8E, 0xDC,
    0x5B, 0xC8, 0x38, 0x0C, 0x84, 0x0E, 0x84, 0x2E, 0x37, 0x2A, 0x4B, 0xDE,
    0x31, 0x82, 0x76, 0x1E, 0x77, 0xA5, 0xC1, 0xD5, 0xED, 0xFF, 0xBC, 0xEA,
    0x91, 0xB7, 0xBC, 0xFF, 0x76, 0x23, 0xE2, 0x78, 0x63, 0x01, 0x47, 0x80,
    0x47, 0x1F, 0x3A, 0x49, 0xBF, 0x0D, 0xCF, 0x27, 0x70, 0x92, 0xBB, 0xEA,
    0xB3, 0x92, 0x70, 0xFF, 0x1E, 0x4B, 0x1B, 0xE0, 0x4E, 0x0C, 0x4C, 0x6B,
    0x5D, 0x77, 0x06, 0xBB, 0xFB, 0x9B, 0x0E, 0x55, 0xB8, 0x8A, 0xF2, 0x45,
    0xA9, 0xF3, 0x54, 0x3D, 0x0C, 0xAC, 0xA8, 0x15, 0xD2, 0x31, 0x8D, 0x97,
    0x08, 0x73, 0xC9, 0x0F, 0x1D, 0xDE, 0x10, 0x22, 0xC6, 0x55, 0x53, 0x7F,
    0x7C, 0x50, 0x16, 0x5A, 0x08, 0xCC, 0x1C, 0x53, 0x9B, 0x02, 0xB8, 0x80,
    0xB7, 0x46, 0xF5, 0xF1, 0xC7, 0x3D, 0x36, 0xBD, 0x26, 0x02, 0xDE, 0x10,
    0xAB, 0x5A, 0x03, 0xCD, 0x67, 0x00, 0x1C, 0x23, 0xC7, 0x13, 0xEE, 0x5D,
    0xAF, 0xC5, 0x1F, 0xE3, 0xA0, 0x54, 0xAC, 0xC2, 0xC9, 0x44, 0xD4, 0x4A,
    0x09, 0x8E, 0xEB, 0xAE, 0xCA, 0x08, 0x8A, 0x7F, 0x41, 0x7B, 0xD8, 0x2C,
    0xDD, 0x6F, 0x80, 0xC3,
};

const char user_affiliation_id1[] = "id1";
const char user_affiliation_id2[] = "id2";

std::vector<uint8_t> ExportPublicKey(const crypto::RSAPrivateKey& key) {
  std::vector<uint8_t> public_key;
  CHECK(key.ExportPublicKey(&public_key));
  return public_key;
}

std::string ConvertPublicKeyToString(const std::vector<uint8_t>& public_key) {
  return std::string(reinterpret_cast<const char*>(public_key.data()),
                     public_key.size());
}

// Produces |key|'s signature over |data| and stores it in |signature|.
void SignData(const std::string& data,
              crypto::RSAPrivateKey* const key,
              std::string* const signature) {
  std::unique_ptr<crypto::SignatureCreator> signature_creator(
      crypto::SignatureCreator::Create(key, crypto::SignatureCreator::SHA1));
  signature_creator->Update(reinterpret_cast<const uint8_t*>(data.c_str()),
                            data.size());
  std::vector<uint8_t> signature_bytes;
  CHECK(signature_creator->Final(&signature_bytes));
  signature->assign(reinterpret_cast<const char*>(signature_bytes.data()),
                    signature_bytes.size());
}

}  // namespace

// Constants used as dummy data for filling the PolicyData protobuf.
const char PolicyBuilder::kFakeDeviceId[] = "device-id";
const char PolicyBuilder::kFakeDomain[] = "example.com";
const char PolicyBuilder::kFakeGaiaId[] = "gaia-id";
const char PolicyBuilder::kFakeMachineName[] = "machine-name";
const char PolicyBuilder::kFakePolicyType[] = "policy type";
const int PolicyBuilder::kFakePublicKeyVersion = 17;
const int64_t PolicyBuilder::kFakeTimestamp = 365LL * 24 * 60 * 60 * 1000;
const char PolicyBuilder::kFakeToken[] = "token";
const char PolicyBuilder::kFakeUsername[] = "username@example.com";
const char PolicyBuilder::kFakeServiceAccountIdentity[] = "robot4test@g.com";

PolicyBuilder::PolicyBuilder() {
  SetDefaultSigningKey();
  CreatePolicyData();
  policy_data_->set_policy_type(kFakePolicyType);
  policy_data_->set_timestamp(kFakeTimestamp);
  policy_data_->set_gaia_id(kFakeGaiaId);
  policy_data_->set_request_token(kFakeToken);
  policy_data_->set_machine_name(kFakeMachineName);
  policy_data_->set_public_key_version(kFakePublicKeyVersion);
  policy_data_->set_username(kFakeUsername);
  policy_data_->set_device_id(kFakeDeviceId);
  policy_data_->set_state(em::PolicyData::ACTIVE);
  policy_data_->set_service_account_identity(kFakeServiceAccountIdentity);
  policy_data_->add_user_affiliation_ids(user_affiliation_id1);
  policy_data_->add_user_affiliation_ids(user_affiliation_id2);
}

PolicyBuilder::~PolicyBuilder() {}

std::unique_ptr<crypto::RSAPrivateKey> PolicyBuilder::GetSigningKey() const {
  if (raw_signing_key_.empty())
    return std::unique_ptr<crypto::RSAPrivateKey>();
  return crypto::RSAPrivateKey::CreateFromPrivateKeyInfo(raw_signing_key_);
}

void PolicyBuilder::SetSigningKey(const crypto::RSAPrivateKey& key) {
  key.ExportPrivateKey(&raw_signing_key_);
}

void PolicyBuilder::SetDefaultSigningKey() {
  raw_signing_key_.assign(kSigningKey, kSigningKey + arraysize(kSigningKey));
}

void PolicyBuilder::UnsetSigningKey() {
  raw_signing_key_.clear();
}

std::unique_ptr<crypto::RSAPrivateKey> PolicyBuilder::GetNewSigningKey() const {
  if (raw_new_signing_key_.empty())
    return std::unique_ptr<crypto::RSAPrivateKey>();
  return std::unique_ptr<crypto::RSAPrivateKey>(
      crypto::RSAPrivateKey::CreateFromPrivateKeyInfo(raw_new_signing_key_));
}

void PolicyBuilder::SetDefaultNewSigningKey() {
  raw_new_signing_key_.assign(kNewSigningKey,
                              kNewSigningKey + arraysize(kNewSigningKey));
  raw_new_signing_key_signature_ = GetTestOtherSigningKeySignature();
}

void PolicyBuilder::UnsetNewSigningKey() {
  raw_new_signing_key_.clear();
  raw_new_signing_key_signature_.clear();
}

void PolicyBuilder::SetDefaultInitialSigningKey() {
  raw_new_signing_key_.assign(kSigningKey,
                              kSigningKey + arraysize(kSigningKey));
  raw_new_signing_key_signature_ = GetTestSigningKeySignature();
  UnsetSigningKey();
}

void PolicyBuilder::Build() {
  // Generate signatures if applicable.
  std::unique_ptr<crypto::RSAPrivateKey> policy_signing_key =
      GetNewSigningKey();
  if (policy_signing_key) {
    // Add the new public key.
    policy_.set_new_public_key(
        ConvertPublicKeyToString(ExportPublicKey(*policy_signing_key)));
    policy_.set_new_public_key_verification_signature_deprecated(
        raw_new_signing_key_signature_);

    // The new public key must be signed by the old key.
    std::unique_ptr<crypto::RSAPrivateKey> old_signing_key = GetSigningKey();
    if (old_signing_key) {
      SignData(policy_.new_public_key(), old_signing_key.get(),
               policy_.mutable_new_public_key_signature());
    }
  } else {
    // No new signing key, so clear the old public key (this allows us to
    // reuse the same PolicyBuilder to build multiple policy blobs).
    policy_.clear_new_public_key();
    policy_.clear_new_public_key_verification_signature_deprecated();
    policy_.clear_new_public_key_signature();
    policy_signing_key = GetSigningKey();
  }

  if (policy_data_) {
    // Policy isn't signed, so there shouldn't be a public key version.
    if (!policy_signing_key)
      policy_data_->clear_public_key_version();

    // Serialize the policy data.
    CHECK(policy_data_->SerializeToString(policy_.mutable_policy_data()));

    // PolicyData signature.
    if (policy_signing_key) {
      SignData(policy_.policy_data(), policy_signing_key.get(),
               policy_.mutable_policy_data_signature());
    }
  } else {
    policy_.clear_policy_data();
    policy_.clear_policy_data_signature();
  }
}

std::string PolicyBuilder::GetBlob() const {
  return policy_.SerializeAsString();
}

std::unique_ptr<em::PolicyFetchResponse> PolicyBuilder::GetCopy() const {
  return std::make_unique<em::PolicyFetchResponse>(policy_);
}

// static
std::unique_ptr<crypto::RSAPrivateKey> PolicyBuilder::CreateTestSigningKey() {
  std::vector<uint8_t> raw_signing_key(kSigningKey,
                                       kSigningKey + arraysize(kSigningKey));
  return crypto::RSAPrivateKey::CreateFromPrivateKeyInfo(raw_signing_key);
}

// static
std::unique_ptr<crypto::RSAPrivateKey>
PolicyBuilder::CreateTestOtherSigningKey() {
  std::vector<uint8_t> raw_new_signing_key(
      kNewSigningKey, kNewSigningKey + arraysize(kNewSigningKey));
  return crypto::RSAPrivateKey::CreateFromPrivateKeyInfo(raw_new_signing_key);
}

// static
std::string PolicyBuilder::GetTestSigningKeySignature() {
  return std::string(reinterpret_cast<const char*>(kSigningKeySignature),
                     sizeof(kSigningKeySignature));
}

// static
std::string PolicyBuilder::GetTestOtherSigningKeySignature() {
  return std::string(reinterpret_cast<const char*>(kNewSigningKeySignature),
                     sizeof(kNewSigningKeySignature));
}

std::vector<uint8_t> PolicyBuilder::GetPublicSigningKey() const {
  std::unique_ptr<crypto::RSAPrivateKey> key = GetSigningKey();
  if (!key)
    return std::vector<uint8_t>();
  return ExportPublicKey(*key);
}

std::vector<uint8_t> PolicyBuilder::GetPublicNewSigningKey() const {
  std::unique_ptr<crypto::RSAPrivateKey> key = GetNewSigningKey();
  if (!key)
    return std::vector<uint8_t>();
  return ExportPublicKey(*key);
}

// static
std::vector<uint8_t> PolicyBuilder::GetPublicTestKey() {
  return ExportPublicKey(*CreateTestSigningKey());
}

// static
std::vector<uint8_t> PolicyBuilder::GetPublicTestOtherKey() {
  return ExportPublicKey(*CreateTestOtherSigningKey());
}

std::string PolicyBuilder::GetPublicSigningKeyAsString() const {
  return ConvertPublicKeyToString(GetPublicSigningKey());
}

std::string PolicyBuilder::GetPublicNewSigningKeyAsString() const {
  return ConvertPublicKeyToString(GetPublicNewSigningKey());
}

// static
std::string PolicyBuilder::GetPublicTestKeyAsString() {
  return ConvertPublicKeyToString(GetPublicTestKey());
}

// static
std::string PolicyBuilder::GetPublicTestOtherKeyAsString() {
  return ConvertPublicKeyToString(GetPublicTestOtherKey());
}

// static
std::vector<std::string> PolicyBuilder::GetUserAffiliationIds() {
  return {user_affiliation_id1, user_affiliation_id2};
}

// static
AccountId PolicyBuilder::GetFakeAccountIdForTesting() {
  return AccountId::FromUserEmailGaiaId(kFakeUsername, kFakeGaiaId);
}

template <>
TypedPolicyBuilder<em::CloudPolicySettings>::TypedPolicyBuilder()
    : payload_(new em::CloudPolicySettings()) {
  policy_data().set_policy_type(dm_protocol::kChromeUserPolicyType);
}

// Have the instantiation compiled into the module.
template class TypedPolicyBuilder<em::CloudPolicySettings>;

#if !defined(OS_ANDROID) && !defined(OS_IOS)
template <>
TypedPolicyBuilder<em::ExternalPolicyData>::TypedPolicyBuilder() {
  CreatePayload();
  policy_data().set_policy_type(dm_protocol::kChromeExtensionPolicyType);
}

template class TypedPolicyBuilder<em::ExternalPolicyData>;
#endif

#if defined(OS_CHROMEOS)
StringPolicyBuilder::StringPolicyBuilder() = default;

void StringPolicyBuilder::Build() {
  policy_data().set_policy_value(payload_);
  PolicyBuilder::Build();
}
#endif

}  // namespace policy
