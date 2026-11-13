// BLE GATT server for the rotary encoder. Implements docs/ble-protocol.md
// (keep the UUIDs and formats in sync with that file and the Swift package).
#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>

class EncoderBle {
 public:
  static constexpr const char* DEVICE_NAME = "RotaryEnc";
  static constexpr const char* SERVICE_UUID = "f0b85ab3-94cf-4d88-bf3b-ed017371edf9";
  static constexpr const char* VALUE_UUID = "6bf5d408-e6a7-4eef-9ac7-9890e9cc56a6";
  static constexpr const char* BUTTON_UUID = "3b371367-7749-4319-a442-ae92f52a406d";
  static constexpr const char* CONTROL_UUID = "cc30c315-5374-4956-b9a6-7826ab23bd93";
  static constexpr uint8_t CMD_RESET = 0x01;

  // Start the BLE stack and begin advertising.
  void begin();

  // Update the values. Cheap to call every loop; notifications are sent from loop().
  void setValue(int16_t v);
  void setButton(bool pressed);

  // Call every loop iteration. Sends pending notifications, rate limited so a fast
  // knob spin cannot overrun the BLE stack's buffers (the latest value always wins).
  void loop(uint32_t nowMs);

  // True once after a central wrote the reset command; the caller applies it.
  bool takeResetRequest();

  bool connected() const { return connected_; }

 private:
  class ServerCb;
  class ControlCb;

  NimBLECharacteristic* value_ = nullptr;
  NimBLECharacteristic* button_ = nullptr;

  int16_t valueNow_ = 0;
  uint8_t buttonNow_ = 0;
  bool valueDirty_ = false;
  bool buttonDirty_ = false;
  uint32_t lastValueMs_ = 0;
  uint32_t lastButtonMs_ = 0;

  // Written from the BLE host task, read from loop().
  volatile bool connected_ = false;
  volatile bool resetRequested_ = false;

  friend class ServerCb;
  friend class ControlCb;
};
