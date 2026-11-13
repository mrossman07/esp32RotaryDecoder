# BLE GATT protocol (draft)

Single source of truth shared by the firmware (`firmware/lib/EncoderBle`) and the
Apple client (`apple/Packages/EncoderBLE`). Change it here first, then both sides.

Advertised name: `RotaryEnc` (plus the service UUID in the advertisement).
All multi-byte values are little-endian.

| Item | UUID | Properties | Format |
|------|------|-----------|--------|
| Encoder service | `f0b85ab3-94cf-4d88-bf3b-ed017371edf9` | | |
| Value | `6bf5d408-e6a7-4eef-9ac7-9890e9cc56a6` | Read, Notify | `int16`, clamped to -100...100 |
| Button | `3b371367-7749-4319-a442-ae92f52a406d` | Read, Notify | `uint8`: 0 = released, 1 = pressed |
| Control | `cc30c315-5374-4956-b9a6-7826ab23bd93` | Write | `uint8` command: 0x01 = reset value to 0 |

## Decisions

- The firmware owns the value and clamps it to [-100, 100]. The app only displays it,
  so reconnecting always shows the true value (the app reads Value on connect).
- Button notifies on both press and release so the app's "LED" tracks the physical button.
- No pairing/bonding in v1 (open connection). Revisit if needed.
