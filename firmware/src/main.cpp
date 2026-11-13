// Mechanical rotary encoder + push button.
// Phase 1: reported over USB serial. Phase 2: also exposed over BLE (docs/ble-protocol.md).
// Samples at 1 kHz; debounce/decode logic lives in lib/EncoderDecoder (host-tested).
#include <Arduino.h>
#include "EncoderBle.h"
#include "EncoderDecoder.h"

// Same Arduino D-pin names on both boards. Avoids the default I2C (D4/D5), UART (D6/D7)
// and SPI (D8-D10) pins.
//   C6: D0=GPIO0 D1=GPIO1 D2=GPIO2     S3: D0=GPIO1 D1=GPIO2 D2=GPIO3
// Encoder common goes to GND and the module's "+" pin to 3V3 (its 10K resistors become
// pull-ups); the other side of the button goes to GND. Internal pull-ups are also enabled.
static const uint8_t PIN_A = D0;   // encoder A (CLK)
static const uint8_t PIN_B = D1;   // encoder B (DT)
static const uint8_t PIN_SW = D2;  // push button

static const uint32_t SAMPLE_US = 1000;  // 1 kHz

static enc::Config makeConfig() {
  enc::Config c;
  c.transitions_per_detent = 4;  // check the part: some encoders give 2 per detent
  c.rest_state = 3;              // both lines high at rest
  c.invert = false;              // set true if clockwise counts down
  c.button_debounce_ms = 10;
  return c;
}

static enc::Decoder decoder(makeConfig());
static enc::Button button(10);
static enc::ClampedCounter counter(-100, 100);
static EncoderBle ble;

static uint32_t nextSample = 0;

static void report() {
  Serial.printf("value=%d btn=%d ble=%s\n", counter.value(), button.pressed() ? 1 : 0,
                ble.connected() ? "connected" : "advertising");
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_A, INPUT_PULLUP);
  pinMode(PIN_B, INPUT_PULLUP);
  pinMode(PIN_SW, INPUT_PULLUP);
  delay(1500);  // give the USB serial monitor time to attach
  ble.begin();
  Serial.println("rotary encoder ready (send 'r' to reset the value to 0)");
  report();
  nextSample = micros();
}

void loop() {
  ble.loop(millis());  // notifications, rate limited

  const uint32_t now = micros();
  if ((int32_t)(now - nextSample) < 0) return;
  nextSample += SAMPLE_US;

  bool changed = false;

  const int step = decoder.update(digitalRead(PIN_A) == HIGH, digitalRead(PIN_B) == HIGH);
  if (step != 0 && counter.add(step)) changed = true;

  // Button is active low.
  if (button.update(digitalRead(PIN_SW) == LOW, millis())) changed = true;

  // Reset from the serial monitor ('r') or from a BLE central (Control = 0x01).
  const bool serialReset = Serial.available() && Serial.read() == 'r';
  if (serialReset || ble.takeResetRequest()) {
    counter.reset();
    changed = true;
  }

  if (changed) {
    ble.setValue((int16_t)counter.value());
    ble.setButton(button.pressed());
    report();
  }
}
