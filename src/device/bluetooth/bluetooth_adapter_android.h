// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUETOOTH_ADAPTER_ANDROID_H_
#define DEVICE_BLUETOOTH_BLUETOOTH_ADAPTER_ANDROID_H_

#include <memory>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "device/bluetooth/bluetooth_adapter.h"

using base::android::ScopedJavaLocalRef;

namespace device {

// BluetoothAdapterAndroid, along with the Java class
// org.chromium.device.bluetooth.BluetoothAdapter, implement BluetoothAdapter.
//
// The GATT Profile over Low Energy is supported, but not Classic Bluetooth at
// this time. LE GATT support has been initially built out to support Web
// Bluetooth, which does not need other Bluetooth features. There is no
// technical reason they can not be supported should a need arrise.
//
// BluetoothAdapterAndroid is reference counted, and owns the lifetime of the
// Java class BluetoothAdapter via j_adapter_. The adapter also owns a tree of
// additional C++ objects (Devices, Services, Characteristics, Descriptors),
// with each C++ object owning its associated Java class.
class DEVICE_BLUETOOTH_EXPORT BluetoothAdapterAndroid final
    : public BluetoothAdapter {
 public:
  // Create a BluetoothAdapterAndroid instance.
  //
  // |java_bluetooth_adapter_wrapper| is optional. If it is NULL the adapter
  // will return false for |IsPresent()| and not be functional.
  //
  // The BluetoothAdapterAndroid instance will indirectly hold a Java reference
  // to |bluetooth_adapter_wrapper|.
  static base::WeakPtr<BluetoothAdapterAndroid> Create(
      const base::android::JavaRef<jobject>&
          bluetooth_adapter_wrapper);  // Java Type: bluetoothAdapterWrapper

  // BluetoothAdapter:
  std::string GetAddress() const override;
  std::string GetName() const override;
  void SetName(const std::string& name,
               const base::Closure& callback,
               const ErrorCallback& error_callback) override;
  bool IsInitialized() const override;
  bool IsPresent() const override;
  bool IsPowered() const override;
  bool IsDiscoverable() const override;
  void SetDiscoverable(bool discoverable,
                       const base::Closure& callback,
                       const ErrorCallback& error_callback) override;
  bool IsDiscovering() const override;
  UUIDList GetUUIDs() const override;
  void CreateRfcommService(
      const BluetoothUUID& uuid,
      const ServiceOptions& options,
      const CreateServiceCallback& callback,
      const CreateServiceErrorCallback& error_callback) override;
  void CreateL2capService(
      const BluetoothUUID& uuid,
      const ServiceOptions& options,
      const CreateServiceCallback& callback,
      const CreateServiceErrorCallback& error_callback) override;
  void RegisterAdvertisement(
      std::unique_ptr<BluetoothAdvertisement::Data> advertisement_data,
      const CreateAdvertisementCallback& callback,
      const AdvertisementErrorCallback& error_callback) override;
  BluetoothLocalGattService* GetGattService(
      const std::string& identifier) const override;

  // Called when adapter state changes.
  void OnAdapterStateChanged(JNIEnv* env,
                             const base::android::JavaParamRef<jobject>& caller,
                             const bool powered);

  // Handles a scan error event by invalidating all discovery sessions.
  void OnScanFailed(JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& caller);

  // Creates or updates device with advertised UUID information when a device is
  // discovered during a scan.
  void CreateOrUpdateDeviceOnScan(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& caller,
      const base::android::JavaParamRef<jstring>& address,
      const base::android::JavaParamRef<jobject>&
          bluetooth_device_wrapper,  // Java Type: bluetoothDeviceWrapper
      int32_t rssi,
      const base::android::JavaParamRef<jobjectArray>&
          advertised_uuids,  // Java Type: String[]
      int32_t tx_power,
      const base::android::JavaParamRef<jobjectArray>&
          service_data_keys,  // Java Type: String[]
      const base::android::JavaParamRef<jobjectArray>&
          service_data_values,  // Java Type: byte[]
      const base::android::JavaParamRef<jintArray>&
          manufacturer_data_keys,  // Java Type: int[]
      const base::android::JavaParamRef<jobjectArray>&
          manufacturer_data_values  // Java Type: byte[]
      );

 protected:
  BluetoothAdapterAndroid();
  ~BluetoothAdapterAndroid() override;

  // BluetoothAdapter:
  bool SetPoweredImpl(bool powered) override;
  void AddDiscoverySession(
      BluetoothDiscoveryFilter* discovery_filter,
      const base::Closure& callback,
      DiscoverySessionErrorCallback error_callback) override;
  void RemoveDiscoverySession(
      BluetoothDiscoveryFilter* discovery_filter,
      const base::Closure& callback,
      DiscoverySessionErrorCallback error_callback) override;
  void SetDiscoveryFilter(
      std::unique_ptr<BluetoothDiscoveryFilter> discovery_filter,
      const base::Closure& callback,
      DiscoverySessionErrorCallback error_callback) override;
  void RemovePairingDelegateInternal(
      BluetoothDevice::PairingDelegate* pairing_delegate) override;

  void PurgeTimedOutDevices();

  // Java object org.chromium.device.bluetooth.ChromeBluetoothAdapter.
  base::android::ScopedJavaGlobalRef<jobject> j_adapter_;

 private:
  size_t num_discovery_sessions_ = 0;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<BluetoothAdapterAndroid> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothAdapterAndroid);
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_BLUETOOTH_ADAPTER_ANDROID_H_
