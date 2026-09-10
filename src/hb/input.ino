#include "input.h"
#include "mode.h"
#include "pb.h"

#include <Arduino.h>
#include <cstdint>

namespace hb {
namespace input {
namespace {

constexpr uint32_t k_debounce_ms = 50;
constexpr uint32_t k_hold_ms = 5000;
constexpr uint32_t k_stall_ms = 200;

bool sampled = false;
uint32_t last_sample_ms = 0;

bool debounce_armed = false;
bool debounce_raw = false;
uint32_t debounce_at = 0;

bool have_stable = false;
bool stable_down = false;
bool armed = false;
bool latched = false;
uint32_t down_at = 0;

void cancel_press() {
  armed = false;
  latched = false;
  have_stable = false;
  debounce_armed = false;
}

void poll_button() {
  const uint32_t now = millis();
  if (sampled && (now - last_sample_ms) >= k_stall_ms) {
    cancel_press();
  }
  last_sample_ms = now;
  sampled = true;

  const bool raw = pb::down();
  if (!debounce_armed || raw != debounce_raw) {
    debounce_raw = raw;
    debounce_at = now;
    debounce_armed = true;
    return;
  }
  if ((now - debounce_at) < k_debounce_ms) {
    return;
  }

  const bool down = debounce_raw;
  if (!have_stable) {
    have_stable = true;
    stable_down = down;
    return;
  }

  if (down && !stable_down) {
    down_at = now;
    armed = true;
    latched = false;
  } else if (!down && stable_down) {
    if (armed && !latched) {
      const uint32_t dt = now - down_at;
      if (dt >= k_debounce_ms && dt < k_hold_ms) {
        mode::short_press();
      }
    }
    armed = false;
    latched = false;
  } else if (down && armed && !latched) {
    if ((now - down_at) >= k_hold_ms) {
      mode::toggle();
      latched = true;
    }
  }
  stable_down = down;
}

void poll_serial() {
  if (!Serial.available()) {
    return;
  }
  const char cmd = static_cast<char>(Serial.read());
  if (cmd == 'e') {
    mode::set_slot(1);
  } else if (cmd == 'd') {
    mode::set_slot(0);
  } else if (cmd == 'h') {
    mode::short_press();
  }
}

}  // namespace

void setup() {}

void loop() {
  poll_button();
  poll_serial();
}

}  // namespace input
}  // namespace hb
