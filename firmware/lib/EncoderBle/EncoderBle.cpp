#include "EncoderBle.h"

// Minimum spacing between notifications of the same characteristic.
static const uint32_t NOTIFY_MIN_INTERVAL_MS = 15;

class EncoderBle::ServerCb : public NimBLEServerCallbacks {
 public:
  explicit ServerCb(EncoderBle& o) : owner_(o) {}
  void onConnect(NimBLEServer*, NimBLEConnInfo&) override { owner_.connected_ = true; }
  void onDisconnect(NimBLEServer* server, NimBLEConnInfo&, int) override {
    owner_.connected_ = server->getConnectedCount() > 0;
    // Advertising restarts automatically (advertiseOnDisconnect is enabled in begin()).
  }

 private:
  EncoderBle& owner_;
};

class EncoderBle::ControlCb : public NimBLECharacteristicCallbacks {
 public:
  explicit ControlCb(EncoderBle& o) : owner_(o) {}
  void onWrite(NimBLECharacteristic* c, NimBLEConnInfo&) override {
    const NimBLEAttValue v = c->getValue();
    Serial.printf("BLE control write: len=%u first=0x%02X\n", (unsigned)v.size(),
                  v.size() ? v.data()[0] : 0);
    if (v.size() >= 1 && v.data()[0] == CMD_RESET) owner_.resetRequested_ = true;
  }

 private:
  EncoderBle& owner_;
};

void EncoderBle::begin() {
  NimBLEDevice::init(DEVICE_NAME);

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCb(*this));
  server->advertiseOnDisconnect(true);

  NimBLEService* svc = server->createService(SERVICE_UUID);

  value_ = svc->createCharacteristic(VALUE_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  value_->setValue<int16_t>(valueNow_);

  button_ = svc->createCharacteristic(BUTTON_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  button_->setValue<uint8_t>(buttonNow_);

  NimBLECharacteristic* control = svc->createCharacteristic(CONTROL_UUID, NIMBLE_PROPERTY::WRITE);
  control->setCallbacks(new ControlCb(*this));

  // Advertisement: flags + the 128-bit service UUID (so apps can scan by service).
  // The name goes in the scan response; both together would not fit in 31 bytes.
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  NimBLEAdvertisementData advData;
  advData.setFlags(0x06);  // general discoverable, BR/EDR not supported
  advData.addServiceUUID(SERVICE_UUID);
  adv->setAdvertisementData(advData);

  NimBLEAdvertisementData scanData;
  scanData.setName(DEVICE_NAME);
  adv->setScanResponseData(scanData);

  adv->start();
}

void EncoderBle::setValue(int16_t v) {
  if (v == valueNow_) return;
  valueNow_ = v;
  value_->setValue<int16_t>(v);  // keeps reads correct even when not connected
  valueDirty_ = true;
}

void EncoderBle::setButton(bool pressed) {
  const uint8_t b = pressed ? 1 : 0;
  if (b == buttonNow_) return;
  buttonNow_ = b;
  button_->setValue<uint8_t>(b);
  buttonDirty_ = true;
}

void EncoderBle::loop(uint32_t nowMs) {
  if (!connected_) {
    // Nobody to notify; the next connection reads the current values.
    valueDirty_ = buttonDirty_ = false;
    return;
  }
  // notify() fails if the central has not subscribed yet or the stack is out of buffers.
  // Keep the dirty flag in that case and retry at the rate limit, so the latest value
  // still goes out once it can.
  if (valueDirty_ && (uint32_t)(nowMs - lastValueMs_) >= NOTIFY_MIN_INTERVAL_MS) {
    if (value_->notify()) valueDirty_ = false;
    lastValueMs_ = nowMs;
  }
  if (buttonDirty_ && (uint32_t)(nowMs - lastButtonMs_) >= NOTIFY_MIN_INTERVAL_MS) {
    if (button_->notify()) buttonDirty_ = false;
    lastButtonMs_ = nowMs;
  }
}

bool EncoderBle::takeResetRequest() {
  if (!resetRequested_) return false;
  resetRequested_ = false;
  return true;
}
