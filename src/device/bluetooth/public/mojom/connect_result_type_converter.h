// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_CONNECT_RESULT_TYPE_CONVERTER_H_
#define DEVICE_BLUETOOTH_CONNECT_RESULT_TYPE_CONVERTER_H_

#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/public/mojom/adapter.mojom.h"
#include "mojo/public/cpp/bindings/type_converter.h"

namespace mojo {

// TypeConverter to translate from
// device::BluetoothDevice::ConnectErrorCode to bluetooth.mojom.ConnectResult.
// TODO(crbug.com/666561): Replace because TypeConverter is deprecated.
template <>
struct TypeConverter<bluetooth::mojom::ConnectResult,
                     device::BluetoothDevice::ConnectErrorCode> {
  static bluetooth::mojom::ConnectResult Convert(
      const device::BluetoothDevice::ConnectErrorCode& input) {
    switch (input) {
      case device::BluetoothDevice::ConnectErrorCode::ERROR_AUTH_CANCELED:
        return bluetooth::mojom::ConnectResult::AUTH_CANCELED;
      case device::BluetoothDevice::ConnectErrorCode::ERROR_AUTH_FAILED:
        return bluetooth::mojom::ConnectResult::AUTH_FAILED;
      case device::BluetoothDevice::ConnectErrorCode::ERROR_AUTH_REJECTED:
        return bluetooth::mojom::ConnectResult::AUTH_REJECTED;
      case device::BluetoothDevice::ConnectErrorCode::ERROR_AUTH_TIMEOUT:
        return bluetooth::mojom::ConnectResult::AUTH_TIMEOUT;
      case device::BluetoothDevice::ConnectErrorCode::ERROR_FAILED:
        return bluetooth::mojom::ConnectResult::FAILED;
      case device::BluetoothDevice::ConnectErrorCode::ERROR_INPROGRESS:
        return bluetooth::mojom::ConnectResult::INPROGRESS;
      case device::BluetoothDevice::ConnectErrorCode::ERROR_UNKNOWN:
        return bluetooth::mojom::ConnectResult::UNKNOWN;
      case device::BluetoothDevice::ConnectErrorCode::ERROR_UNSUPPORTED_DEVICE:
        return bluetooth::mojom::ConnectResult::UNSUPPORTED_DEVICE;
      case device::BluetoothDevice::ConnectErrorCode::NUM_CONNECT_ERROR_CODES:
        NOTREACHED();
        return bluetooth::mojom::ConnectResult::FAILED;
    }
    NOTREACHED();
    return bluetooth::mojom::ConnectResult::FAILED;
  }
};
}  // namespace mojo

#endif  // DEVICE_BLUETOOTH_CONNECT_RESULT_TYPE_CONVERTER_H_
