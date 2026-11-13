# Wiring

Pin numbers confirmed against the Arduino-ESP32 variant files `pins_arduino.h`
for `XIAO_ESP32C6` and `XIAO_ESP32S3`. None are strapping or USB pins on either board.

| Signal | Arduino pin | C6 GPIO | S3 GPIO | Encoder pin |
|--------|-------------|---------|---------|-------------|
| A (CLK) | D3 | 21 | 4 | A / CLK |
| B (DT)  | D4 | 22 | 5 | B / DT |
| Button  | D5 | 23 | 6 | SW |
| GND     | GND | | | C (common) and the other SW pin |

- No external resistors: the firmware enables the internal pull-ups (`INPUT_PULLUP`).
  All inputs read high at rest and low when closed; the button is active low.
- If contact bounce proves worse than the software debounce can handle, add 10 kohm pull-up
  plus 10-100 nF from each line to GND (RC filter).
- The pin choice was moved off D2 (S3 GPIO3 is a strapping pin).

## Tuning notes (fill in after testing with the real encoder)

- Encoder part number: _unknown_
- Transitions per detent (`transitions_per_detent`): _4 assumed_
- Clockwise gives positive counts? (`invert`): _not yet checked_
- Button debounce time: _10 ms assumed_
