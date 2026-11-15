# BLE GATT protocol

Single source of truth shared by the firmware (`firmware/lib/EncoderBle`) and the
Apple client (`apple/Packages/EncoderBLE`). Change it here first, then both sides.

Advertised name: `RotaryEnc` (plus the service UUID in the advertisement).
All multi-byte values are little-endian.

| Item | UUID | Properties | Format |
|------|------|-----------|--------|
| Encoder service | `f0b85ab3-94cf-4d88-bf3b-ed017371edf9` | | |
| Value | `6bf5d408-e6a7-4eef-9ac7-9890e9cc56a6` | Read, Notify | `int16`, clamped to -100...100 |
| Button | `3b371367-7749-4319-a442-ae92f52a406d` | Read, Notify | `uint8`: 0 = released, 1 = pressed (raw, debounced) |
| Control | `cc30c315-5374-4956-b9a6-7826ab23bd93` | Write | `uint8` command: 0x01 = reset value to 0 |
| Gesture | `7670a629-c062-4b70-a4de-09594771dbcb` | Read, Notify | `uint8` event: 0x01 = short tap, 0x02 = long press (0x00 = none yet) |
| Latch | `b429c943-136e-45bb-849c-eeb677d6bf00` | Read, Notify | `uint8`: 0 or 1, flips on every long press |

## Button gestures

The firmware classifies the debounced push button:

- **Short tap**: pressed and released in under 1000 ms. Reported on release.
- **Long press**: held for 1000 ms or more. Reported at the 1000 ms mark while still held;
  the later release produces no event. One long press gives exactly one event.
- The 1000 ms is measured from the debounced press (about 10 ms after the physical contact).

Each gesture is sent as one Gesture notification. Events are queued so quick successive taps
are not merged. Gesture read returns the last event (informational; use notifications).

The firmware owns the Latch state and flips it on each long press. It starts at 0 on boot and is
not cleared by the Control reset command. The app reads Latch on connect, so a reconnect shows the
true state.

## Decisions

- The firmware owns the value and the latch. The app only displays them,
  so reconnecting always shows the true state (the app reads Value and Latch on connect).
- Gesture is an event (momentary, notify), Latch is state (read + notify).
- The raw Button characteristic is kept for debugging (nRF Connect) and older clients.
- A client must tolerate missing Gesture/Latch characteristics (older firmware).
- No pairing/bonding in v1 (open connection). Revisit if needed.
