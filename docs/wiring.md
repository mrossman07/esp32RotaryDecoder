# Wiring

Pin numbers confirmed against the Arduino-ESP32 variant files `pins_arduino.h`
for `XIAO_ESP32C6` and `XIAO_ESP32S3`.

| Signal | Arduino pin | C6 GPIO | S3 GPIO | Encoder pin |
|--------|-------------|---------|---------|-------------|
| A (CLK) | D0 | 0 | 1 | A / CLK |
| B (DT)  | D1 | 1  | 2 | B / DT |
| Button  | D2 | 2  | 3 | SW |
| GND     | GND | | | C (common) and the other SW pin |

## Pins deliberately avoided

- D4/D5: default I2C `SDA`/`SCL` (C6 GPIO22/23, S3 GPIO5/6), kept free for sensors/displays.
- D6/D7: UART `TX`/`RX`. D8-D10: SPI `SCK`/`MISO`/`MOSI`.
- D3: unused and free (C6 GPIO21, S3 GPIO4).
- S3 D2 is GPIO3, a strapping pin (selects the JTAG source only; does not affect boot mode),
  so it is fine for the button.

## Notes

- No external resistors: the firmware enables the internal pull-ups (`INPUT_PULLUP`).
  All inputs read high at rest and low when closed; the button is active low.
- **Encoder module "+" pin must go to 3V3.** The module has 10K resistors from CLK and DT to
  "+". Left unconnected they form a 20K link between CLK and DT, which drags the idle line low
  when the other closes, so no valid quadrature states appear and the decoder counts nothing.
  Connecting "+" to 3V3 (never 5V) makes them proper pull-ups. Verified on the C6.
- If contact bounce proves worse than the software debounce can handle, add 10 kohm pull-up
  plus 10-100 nF from each line to GND (RC filter).

## Tuning notes (fill in after testing with the real encoder)

- Encoder part number: _unknown_
- Transitions per detent (`transitions_per_detent`): 4 (verified on C6: one detent = one count)
- Clockwise gives positive counts? (`invert`): _not yet checked_
- Button debounce time: _10 ms assumed_
