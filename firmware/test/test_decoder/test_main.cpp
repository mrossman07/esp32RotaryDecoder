#include <unity.h>
#include "EncoderDecoder.h"

using namespace enc;

void setUp() {}
void tearDown() {}

// Quadrature states as (A,B). Clockwise (A leads B) is 00 -> 10 -> 11 -> 01 -> 00,
// so one detent starting from rest 11 goes 01, 00, 10, 11. Counter-clockwise is the reverse.
struct S { bool a, b; };
static const S CW[4]  = {{0, 1}, {0, 0}, {1, 0}, {1, 1}};  // from rest 11
static const S CCW[4] = {{1, 0}, {0, 0}, {0, 1}, {1, 1}};  // from rest 11

// Feed a sequence, return total steps.
static int feed(Decoder& d, const S* seq, int n) {
  int total = 0;
  for (int i = 0; i < n; i++) total += d.update(seq[i].a, seq[i].b);
  return total;
}

static Decoder primed() {
  Decoder d;
  d.update(true, true);  // baseline at rest
  return d;
}

void test_one_detent_clockwise() {
  Decoder d = primed();
  TEST_ASSERT_EQUAL(1, feed(d, CW, 4));
}

void test_one_detent_counterclockwise() {
  Decoder d = primed();
  TEST_ASSERT_EQUAL(-1, feed(d, CCW, 4));
}

void test_step_emitted_on_final_transition_only() {
  Decoder d = primed();
  TEST_ASSERT_EQUAL(0, d.update(CW[0].a, CW[0].b));
  TEST_ASSERT_EQUAL(0, d.update(CW[1].a, CW[1].b));
  TEST_ASSERT_EQUAL(0, d.update(CW[2].a, CW[2].b));
  TEST_ASSERT_EQUAL(1, d.update(CW[3].a, CW[3].b));
}

void test_repeated_samples_are_ignored() {
  Decoder d = primed();
  int total = 0;
  for (int i = 0; i < 4; i++)
    for (int r = 0; r < 5; r++) total += d.update(CW[i].a, CW[i].b);
  TEST_ASSERT_EQUAL(1, total);
}

void test_ten_detents() {
  Decoder d = primed();
  int total = 0;
  for (int i = 0; i < 10; i++) total += feed(d, CW, 4);
  TEST_ASSERT_EQUAL(10, total);
  for (int i = 0; i < 10; i++) total += feed(d, CCW, 4);
  TEST_ASSERT_EQUAL(0, total);
}

void test_bounce_on_first_edge_gives_no_extra_count() {
  Decoder d = primed();
  int total = 0;
  // 11 -> 01 -> 11 -> 01 -> 11 -> 01 (bouncing on the first edge) then the rest of the CW detent.
  const S seq[] = {{0, 1}, {1, 1}, {0, 1}, {1, 1}, {0, 1}, {0, 0}, {1, 0}, {1, 1}};
  total = feed(d, seq, 8);
  TEST_ASSERT_EQUAL(1, total);
}

void test_bounce_mid_detent_does_not_reverse() {
  Decoder d = primed();
  // CW detent with the 00 <-> 10 contact chattering before settling.
  const S seq[] = {{0, 1}, {0, 0}, {1, 0}, {0, 0}, {1, 0}, {0, 0}, {1, 0}, {1, 1}};
  TEST_ASSERT_EQUAL(1, feed(d, seq, 8));
}

void test_partial_turn_and_return_gives_nothing() {
  Decoder d = primed();
  const S seq[] = {{0, 1}, {0, 0}, {0, 1}, {1, 1}};  // goes halfway, comes back
  TEST_ASSERT_EQUAL(0, feed(d, seq, 4));
  // And the next full detent is still exactly one count.
  TEST_ASSERT_EQUAL(1, feed(d, CW, 4));
}

void test_invalid_jump_is_ignored_and_resyncs_at_rest() {
  Decoder d = primed();
  // 11 -> 00 is a two-bit jump (missed edge): ignored.
  TEST_ASSERT_EQUAL(0, d.update(false, false));
  TEST_ASSERT_EQUAL(0, d.update(true, true));  // back at rest: accumulator cleared
  // Following detents are clean despite the glitch.
  TEST_ASSERT_EQUAL(1, feed(d, CW, 4));
  TEST_ASSERT_EQUAL(-1, feed(d, CCW, 4));
}

void test_missed_transition_resyncs_at_rest() {
  Decoder d = primed();
  // Skip one edge mid-detent (no 00), arrive at rest: no count, no drift.
  const S seq[] = {{0, 1}, {1, 0}, {1, 1}};
  TEST_ASSERT_EQUAL(0, feed(d, seq, 3));
  TEST_ASSERT_EQUAL(1, feed(d, CW, 4));
}

void test_invert() {
  Config c;
  c.invert = true;
  Decoder d(c);
  d.update(true, true);
  TEST_ASSERT_EQUAL(-1, feed(d, CW, 4));
}

void test_two_transitions_per_detent() {
  Config c;
  c.transitions_per_detent = 2;
  c.rest_state = -1;  // rests in two states, no resync
  Decoder d(c);
  d.update(true, true);
  int total = 0;
  for (int i = 0; i < 4; i++) total += d.update(CW[i].a, CW[i].b);
  TEST_ASSERT_EQUAL(2, total);
}

// ---- Button ----

void test_button_accepts_after_debounce() {
  Button b(10);
  TEST_ASSERT_FALSE(b.update(false, 0));
  TEST_ASSERT_FALSE(b.update(true, 100));
  TEST_ASSERT_FALSE(b.update(true, 109));
  TEST_ASSERT_FALSE(b.pressed());
  TEST_ASSERT_TRUE(b.update(true, 110));
  TEST_ASSERT_TRUE(b.pressed());
  TEST_ASSERT_FALSE(b.update(true, 111));  // no repeat event
}

void test_button_bounce_is_rejected() {
  Button b(10);
  b.update(false, 0);
  // Chatter faster than the debounce window: never accepted.
  for (uint32_t t = 100; t < 140; t += 3) b.update(((t / 3) & 1) != 0, t);
  TEST_ASSERT_FALSE(b.pressed());
  // Settles pressed.
  b.update(true, 140);
  TEST_ASSERT_TRUE(b.update(true, 150));
  TEST_ASSERT_TRUE(b.pressed());
}

void test_button_release() {
  Button b(10);
  b.update(false, 0);
  b.update(true, 100);
  b.update(true, 110);
  TEST_ASSERT_TRUE(b.pressed());
  b.update(false, 200);
  TEST_ASSERT_TRUE(b.update(false, 210));
  TEST_ASSERT_FALSE(b.pressed());
}

void test_button_press_short_glitch_ignored() {
  Button b(10);
  b.update(false, 0);
  b.update(true, 100);   // 5 ms glitch
  b.update(false, 105);
  TEST_ASSERT_FALSE(b.update(false, 120));
  TEST_ASSERT_FALSE(b.pressed());
}

void test_button_millis_wraparound() {
  Button b(10);
  b.update(false, 0xFFFFFFF0u);
  b.update(true, 0xFFFFFFFAu);
  TEST_ASSERT_TRUE(b.update(true, 0x00000008u));  // 14 ms later across the wrap
}

// ---- ClampedCounter ----

void test_counter_clamps() {
  ClampedCounter c(-100, 100);
  TEST_ASSERT_EQUAL(0, c.value());
  TEST_ASSERT_TRUE(c.add(1));
  TEST_ASSERT_EQUAL(1, c.value());
  for (int i = 0; i < 150; i++) c.add(1);
  TEST_ASSERT_EQUAL(100, c.value());
  TEST_ASSERT_FALSE(c.add(1));  // pinned, no change
  for (int i = 0; i < 300; i++) c.add(-1);
  TEST_ASSERT_EQUAL(-100, c.value());
  c.reset();
  TEST_ASSERT_EQUAL(0, c.value());
}

// ---- ButtonGestures ----

// Starts released at t=0 so the gesture detector is primed.
static ButtonGestures primedGestures(uint16_t longMs = 1000) {
  ButtonGestures g(longMs);
  g.update(false, 0);
  return g;
}

void test_gesture_short_tap_reported_on_release() {
  ButtonGestures g = primedGestures();
  TEST_ASSERT_EQUAL(Gesture::None, g.update(true, 100));
  TEST_ASSERT_EQUAL(Gesture::None, g.update(true, 150));
  TEST_ASSERT_EQUAL(Gesture::ShortTap, g.update(false, 180));
  TEST_ASSERT_EQUAL(Gesture::None, g.update(false, 181));
}

void test_gesture_just_under_threshold_is_tap() {
  ButtonGestures g = primedGestures();
  g.update(true, 100);
  TEST_ASSERT_EQUAL(Gesture::None, g.update(true, 1099));  // held 999 ms
  TEST_ASSERT_EQUAL(Gesture::ShortTap, g.update(false, 1100));
}

void test_gesture_long_press_fires_at_threshold_while_held() {
  ButtonGestures g = primedGestures();
  g.update(true, 100);
  TEST_ASSERT_EQUAL(Gesture::None, g.update(true, 1099));
  TEST_ASSERT_EQUAL(Gesture::LongPress, g.update(true, 1100));  // exactly 1000 ms
  TEST_ASSERT_EQUAL(Gesture::None, g.update(true, 1101));
}

void test_gesture_long_press_release_reports_nothing() {
  ButtonGestures g = primedGestures();
  g.update(true, 100);
  TEST_ASSERT_EQUAL(Gesture::LongPress, g.update(true, 1100));
  TEST_ASSERT_EQUAL(Gesture::None, g.update(false, 1300));
}

void test_gesture_long_hold_fires_only_once() {
  ButtonGestures g = primedGestures();
  g.update(true, 100);
  int longs = 0;
  for (uint32_t t = 101; t < 6000; t++)
    if (g.update(true, t) == Gesture::LongPress) longs++;
  TEST_ASSERT_EQUAL(1, longs);
  TEST_ASSERT_EQUAL(Gesture::None, g.update(false, 6001));
}

void test_gesture_sequence_tap_long_tap() {
  ButtonGestures g = primedGestures();
  g.update(true, 100);
  TEST_ASSERT_EQUAL(Gesture::ShortTap, g.update(false, 160));
  g.update(true, 300);
  TEST_ASSERT_EQUAL(Gesture::LongPress, g.update(true, 1300));
  TEST_ASSERT_EQUAL(Gesture::None, g.update(false, 1400));
  g.update(true, 1500);
  TEST_ASSERT_EQUAL(Gesture::ShortTap, g.update(false, 1560));
}

void test_gesture_millis_wraparound() {
  ButtonGestures g = primedGestures();
  const uint32_t t0 = 0xFFFFFF00u;  // press just before the 32-bit wrap
  g.update(true, t0);
  TEST_ASSERT_EQUAL(Gesture::None, g.update(true, t0 + 999));
  TEST_ASSERT_EQUAL(Gesture::LongPress, g.update(true, t0 + 1000));  // wrapped past zero
}

void test_gesture_held_at_boot_is_ignored_until_released() {
  ButtonGestures g;
  TEST_ASSERT_EQUAL(Gesture::None, g.update(true, 0));  // already held on the first sample
  TEST_ASSERT_EQUAL(Gesture::None, g.update(true, 2000));  // no long press for it
  TEST_ASSERT_EQUAL(Gesture::None, g.update(false, 2100));  // and no tap on release
  g.update(true, 3000);
  TEST_ASSERT_EQUAL(Gesture::ShortTap, g.update(false, 3050));  // normal again
}

void test_gesture_with_debounced_button_ignores_bounce() {
  // End to end: raw chatter -> Button debounce -> ButtonGestures gives exactly one tap.
  Button b(10);
  ButtonGestures g;
  int taps = 0, longs = 0;
  auto step = [&](bool raw, uint32_t t) {
    b.update(raw, t);
    Gesture r = g.update(b.pressed(), t);
    if (r == Gesture::ShortTap) taps++;
    if (r == Gesture::LongPress) longs++;
  };
  step(false, 0);
  for (uint32_t t = 100; t < 112; t += 2) step(((t / 2) & 1) != 0, t);  // chatter on press
  for (uint32_t t = 112; t < 200; t++) step(true, t);
  for (uint32_t t = 200; t < 212; t += 2) step(((t / 2) & 1) == 0, t);  // chatter on release
  for (uint32_t t = 212; t < 300; t++) step(false, t);
  TEST_ASSERT_EQUAL(1, taps);
  TEST_ASSERT_EQUAL(0, longs);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_gesture_short_tap_reported_on_release);
  RUN_TEST(test_gesture_just_under_threshold_is_tap);
  RUN_TEST(test_gesture_long_press_fires_at_threshold_while_held);
  RUN_TEST(test_gesture_long_press_release_reports_nothing);
  RUN_TEST(test_gesture_long_hold_fires_only_once);
  RUN_TEST(test_gesture_sequence_tap_long_tap);
  RUN_TEST(test_gesture_millis_wraparound);
  RUN_TEST(test_gesture_held_at_boot_is_ignored_until_released);
  RUN_TEST(test_gesture_with_debounced_button_ignores_bounce);
  RUN_TEST(test_one_detent_clockwise);
  RUN_TEST(test_one_detent_counterclockwise);
  RUN_TEST(test_step_emitted_on_final_transition_only);
  RUN_TEST(test_repeated_samples_are_ignored);
  RUN_TEST(test_ten_detents);
  RUN_TEST(test_bounce_on_first_edge_gives_no_extra_count);
  RUN_TEST(test_bounce_mid_detent_does_not_reverse);
  RUN_TEST(test_partial_turn_and_return_gives_nothing);
  RUN_TEST(test_invalid_jump_is_ignored_and_resyncs_at_rest);
  RUN_TEST(test_missed_transition_resyncs_at_rest);
  RUN_TEST(test_invert);
  RUN_TEST(test_two_transitions_per_detent);
  RUN_TEST(test_button_accepts_after_debounce);
  RUN_TEST(test_button_bounce_is_rejected);
  RUN_TEST(test_button_release);
  RUN_TEST(test_button_press_short_glitch_ignored);
  RUN_TEST(test_button_millis_wraparound);
  RUN_TEST(test_counter_clamps);
  return UNITY_END();
}
