// Hardware-free quadrature decoder, button debouncer and clamped counter.
// No Arduino includes, so it builds and is unit-tested on the host.
//
// Usage: call update() at a fixed rate (about 1 kHz) with the raw pin levels.
#pragma once
#include <stdint.h>

namespace enc {

struct Config {
  // Quadrature transitions per mechanical detent: 4 for most encoders, 2 for some.
  uint8_t transitions_per_detent = 4;
  // Detent (rest) state as (A<<1)|B, used to resync after missed transitions.
  // 3 = both lines high (typical with pull-ups). -1 disables resync (use for
  // encoders that rest in two different states).
  int8_t rest_state = 3;
  // Swap if clockwise comes out negative (depends on wiring / part).
  bool invert = false;
  // The button must be stable this long to be accepted.
  uint16_t button_debounce_ms = 10;
};

// Counts valid Gray-code transitions. A bounce that goes forward then back nets
// zero, and an invalid two-bit jump is ignored, so contact bounce cannot produce
// a count. Reports one step per detent.
class Decoder {
 public:
  explicit Decoder(Config c = Config()) : cfg_(c) {}

  // Feed one sample of the raw channel levels (true = high).
  // Returns detent steps since the previous call: +1 clockwise, -1 counter-clockwise, else 0.
  int update(bool a, bool b) {
    const uint8_t cur = (uint8_t)((a ? 2 : 0) | (b ? 1 : 0));
    if (!primed_) {  // first sample just sets the baseline
      prev_ = cur;
      primed_ = true;
      return 0;
    }
    int step = 0;
    if (cur != prev_) {
      // Clockwise sequence (A leads B): 00 -> 10 -> 11 -> 01 -> 00
      static const uint8_t next_cw[4] = {2, 0, 3, 1};
      if (next_cw[prev_] == cur) acc_++;
      else if (next_cw[cur] == prev_) acc_--;
      // else: two-bit jump, invalid, ignore
      prev_ = cur;

      const int n = cfg_.transitions_per_detent;
      if (acc_ >= n) { step = 1; acc_ -= n; }
      else if (acc_ <= -n) { step = -1; acc_ += n; }
    }
    if (cfg_.rest_state >= 0 && cur == (uint8_t)cfg_.rest_state) acc_ = 0;
    return cfg_.invert ? -step : step;
  }

 private:
  Config cfg_;
  uint8_t prev_ = 0;
  int acc_ = 0;
  bool primed_ = false;
};

// Debounces an active-high "pressed" signal. Accepts a new level once it has been
// stable for button_debounce_ms.
class Button {
 public:
  explicit Button(uint16_t debounce_ms = 10) : debounce_ms_(debounce_ms) {}

  // Returns true when the debounced state changed on this call.
  bool update(bool pressed, uint32_t now_ms) {
    if (!primed_) {
      raw_ = state_ = pressed;
      since_ = now_ms;
      primed_ = true;
      return false;
    }
    if (pressed != raw_) {
      raw_ = pressed;
      since_ = now_ms;
    }
    if (raw_ != state_ && (uint32_t)(now_ms - since_) >= debounce_ms_) {
      state_ = raw_;
      return true;
    }
    return false;
  }

  bool pressed() const { return state_; }

 private:
  uint16_t debounce_ms_;
  bool raw_ = false, state_ = false, primed_ = false;
  uint32_t since_ = 0;
};

// Integer that saturates at [lo, hi].
class ClampedCounter {
 public:
  ClampedCounter(int lo = -100, int hi = 100) : lo_(lo), hi_(hi) {}

  // Returns true if the value changed.
  bool add(int delta) {
    int v = value_ + delta;
    if (v < lo_) v = lo_;
    if (v > hi_) v = hi_;
    const bool changed = v != value_;
    value_ = v;
    return changed;
  }
  void reset() { value_ = 0; }
  int value() const { return value_; }

 private:
  int lo_, hi_, value_ = 0;
};

}  // namespace enc
