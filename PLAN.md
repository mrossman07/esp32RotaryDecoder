# Plan: mechanical rotary encoder on XIAO ESP32-C6 / -S3

Goal: read a mechanical quadrature encoder with push button on a Seeed XIAO ESP32-C6
or ESP32-S3, first over USB serial, then over BLE to a SwiftUI demo app on iPhone,
iPad and Mac. The demo shows an integer (-100...100, starts at 0, clockwise = up,
counter-clockwise = down) and an "LED" that lights while the encoder button is pressed.

## Repository layout

```
PLAN.md  CLAUDE.md  README.md
docs/
  ble-protocol.md        GATT contract shared by firmware and app (source of truth)
  wiring.md              pins, wiring, debounce circuit (written in Phase 0/1)
firmware/                PlatformIO project (open this folder in VSCode)
  platformio.ini         envs: xiao_esp32c6, xiao_esp32s3, native (unit tests)
  src/main.cpp           wiring of the pieces below
  lib/EncoderDecoder/    pure C++ quadrature decode + debounce, no Arduino deps
  lib/EncoderBle/        NimBLE GATT server (Phase 2)
  test/                  Unity tests, run on the host with `pio test -e native`
apple/
  EncoderDemo/           Xcode project: one multiplatform SwiftUI app (iOS, iPadOS, macOS)
  Packages/EncoderBLE/   local Swift package: CoreBluetooth client + mock, unit-testable
```

## Key design decisions

1. **Debounce in software, with a state-table decoder.** Sample both channels and
   the button from a periodic timer/task at about 1 kHz. Quadrature uses a
   Gray-code state-transition table: invalid (bounce) transitions are discarded and
   a step is only emitted on a complete, valid sequence, so contact bounce cannot
   produce false counts or direction flips. The button uses a counter/integrator
   debounce (stable for ~5-10 ms). An optional hardware RC (10 kohm + 10-100 nF per
   line) is a documented fallback if the encoder is very noisy; start without it.
2. **Polling over GPIO interrupts.** Simpler, naturally debounced, and 1 kHz is far
   faster than a hand can turn a knob. Revisit only if power use matters.
3. **Decoder is hardware-free.** `EncoderDecoder` takes raw pin samples plus a
   timestamp and returns step deltas and button events, so it is unit-tested on the
   Mac with no board attached. Configurable `transitions_per_detent` (2 or 4; check
   the actual part's datasheet).
4. **Wiring:** A, B, switch to GPIO with `INPUT_PULLUP`, common to GND. Use Arduino
   `D`-pin names so the same code runs on both boards. Pins: A=D3, B=D4,
   SW=D5 (verified non-strapping on both boards; see `docs/wiring.md`).
5. **BLE stack: NimBLE-Arduino** (supports both C6 and S3, small footprint). Custom
   GATT service, defined in `docs/ble-protocol.md`: Value (int16, notify), Button
   (uint8, notify), Control (write, reset to 0).
6. **Firmware owns the value** and clamps to [-100, 100]; the app only displays it
   and reads the current value on connect.
7. **Apple side:** one SwiftUI multiplatform target using CoreBluetooth. All BLE logic
   lives in the `EncoderBLE` package behind a protocol, with a mock implementation
   for SwiftUI previews and for the iOS simulator (which has no Bluetooth).
   Requires `NSBluetoothAlwaysUsageDescription` and, on macOS, the App Sandbox
   Bluetooth entitlement.
8. **PlatformIO toolchain:** the C6 needs Arduino-ESP32 core 3.x, so `platformio.ini`
   uses the pioarduino platform fork instead of stock espressif32. Verify in Phase 0.

## Phases

### Phase 0 - Toolchain and bring-up
- [ ] Install the PlatformIO extension in VSCode. Note: the Homebrew `pio` runs on Python 3.15, which the
      platform rejects; use `~/.platformio/penv/bin/pio` (Python 3.11), which the extension also uses.
- [x] `platformio.ini` builds for both boards (C6 and S3 build verified).
- [ ] Confirm upload and the USB serial monitor work on both boards
      (`ARDUINO_USB_CDC_ON_BOOT=1`; may need the boot button for the first flash).
- [x] Pins chosen and recorded in `docs/wiring.md`; [ ] encoder wired.
- [x] Add `firmware/.vscode/extensions.json`.

### Phase 1 - Encoder over USB serial
- [x] `EncoderDecoder` + host unit tests (`native` env): 18 tests pass (rotation both
      directions, bounce/glitch sequences, missed-edge recovery, button debounce, clamping).
- [x] `main.cpp` (builds for both boards; not yet run on hardware): 1 kHz sampling, clamped counter, print `value=<n> btn=<0|1>` on change.
- [ ] Verify on both boards with the real encoder: one detent = exactly one count,
      no phantom counts or reversals when turning fast or slowly, clean button press.
- [ ] Tune `transitions_per_detent` and debounce times; note results in `docs/wiring.md`.

Done when: turning the knob prints correct, stable counts on both boards.

### Phase 2 - BLE peripheral (firmware)
- [ ] Add NimBLE-Arduino to `lib_deps`; implement `EncoderBle` per `docs/ble-protocol.md`.
- [ ] Notify on value change and on button press/release; support read on connect
      and the reset control write; restart advertising on disconnect.
- [ ] Test with nRF Connect (iOS/Mac) before any app exists.
- [ ] Keep serial output as a debug log.

Done when: nRF Connect shows correct notifications on both boards.

### Phase 3 - Apple demo app
- [ ] In Xcode create a multiplatform App (SwiftUI) in `apple/EncoderDemo/`; add
      Bluetooth usage string and macOS sandbox Bluetooth entitlement.
- [ ] Create the `EncoderBLE` Swift package: CoreBluetooth central (scan by service UUID,
      connect, discover, subscribe, auto-reconnect), a protocol plus mock, tests.
- [ ] UI: large integer (-100...100), "LED" indicator that lights while the button is
      pressed, connection status, a reset button.
- [ ] Run on a real iPhone, iPad and Mac (Bluetooth does not work in the iOS simulator).

Done when: all three devices show the live value and LED from either board.

### Phase 4 - Polish (optional)
- Reconnect robustness, multiple-peripheral picker, bonding, battery/power, enclosure.

## Open questions
- Exact encoder part number: detent/transition count, pull-up needs, button polarity.
- Whether the first app target should be iPhone-only to start, or all three at once
  (the plan assumes one multiplatform target).
- Minimum OS versions for the app.

## Verification notes
Built OK for C6 and S3; 18 host tests pass. Not yet flashed or tested on real hardware. The GATT UUIDs are drafts.
