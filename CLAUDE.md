# esp32RotaryDecoder

Prototype: mechanical rotary encoder (quadrature + push button) on Seeed XIAO ESP32-C6
and XIAO ESP32-S3. Phase 1 reports over USB serial; later phases expose it over BLE to a
SwiftUI demo app (iPhone, iPad, Mac) showing an integer in -100...100 and an "LED" that
lights while the button is pressed. See `PLAN.md` for phases and design decisions.

## Layout

- `firmware/`: PlatformIO project (Arduino framework). Open this folder in VSCode.
  - `lib/EncoderDecoder/`: pure C++ decode/debounce, no Arduino includes, so it runs in host tests.
  - `lib/EncoderBle/`: NimBLE-Arduino GATT server.
- `apple/EncoderDemo/`: Xcode multiplatform SwiftUI app.
- `apple/Packages/EncoderBLE/`: local Swift package with the CoreBluetooth client and a mock.
- `docs/ble-protocol.md`: the GATT contract. **Source of truth**; update it first, then firmware and app.

## Firmware commands (run from `firmware/`)

Use `~/.platformio/penv/bin/pio` (Python 3.11). The Homebrew `pio` runs on Python 3.15, which the
pioarduino platform rejects. Examples below say `pio` for brevity.

```bash
pio run -e xiao_esp32c6              # build (also: xiao_esp32s3)
pio run -e xiao_esp32c6 -t upload    # flash
pio device monitor                   # serial monitor, 115200
pio test -e native                   # host unit tests, no hardware needed
```

## Conventions

- Debounce is in software (state-table quadrature decoder plus integrator for the button), not
  GPIO interrupts. Keep hardware-specific code out of `EncoderDecoder`.
- Use Arduino `D`-pin names (`D1`, `D2`, `D3`), not raw GPIO numbers, so both boards share code.
  Avoid strapping pins. Pin choices go in `docs/wiring.md`.
- Firmware owns the value and clamps to [-100, 100]. The app is display-only.
- Any change to the BLE service, characteristics or formats must be made in `docs/ble-protocol.md`
  and mirrored in both `EncoderBle` and the Swift package.
- Prefer a unit test in the `native` env for any decode/debounce change before testing on hardware.
- `platformio.ini` uses the pioarduino platform fork because the C6 needs Arduino-ESP32 3.x.
- The iOS simulator has no Bluetooth: use the mock in `EncoderBLE` for previews and test on real devices.
- The Xcode project is created in the Xcode GUI (not hand-written pbxproj). Commit the `.xcodeproj`,
  not `xcuserdata/` or DerivedData.

## Status

Phase 1 code written: builds for C6 and S3, 18 host tests pass. Not yet flashed or tested with the
real encoder. Pins: A=D3, B=D4, SW=D5 (`docs/wiring.md`).
