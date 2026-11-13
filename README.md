# esp32RotaryDecoder
Development/prototype for using mechanical rotary encoder with ESP32

Seeed XIAO ESP32-C6 / ESP32-S3 firmware (PlatformIO, Arduino) that debounces a mechanical
quadrature encoder with push button, reports over USB serial, then over BLE to a SwiftUI demo
app on iPhone, iPad and Mac.

- `PLAN.md`: project plan and phases
- `CLAUDE.md`: working notes for Claude Code
- `firmware/`: ESP32 PlatformIO project
- `apple/`: Xcode app and Swift BLE package
- `docs/`: BLE protocol and wiring notes
