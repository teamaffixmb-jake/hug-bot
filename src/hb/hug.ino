#include "hug.h"
#include "log.h"
#include "mqtt.h"
#include "oled_ui.h"
#include "pref.h"

#include <Arduino.h>
#include <cstdint>

namespace hb {
namespace hug {
namespace {

constexpr uint32_t k_poll_ms = 3000;
constexpr int32_t k_pending_cap = 999;

uint32_t last_poll_ms = 0;
bool polled = false;
int32_t pending = 0;

int32_t clamp_pending(int32_t value) {
  if (value > k_pending_cap) {
    return k_pending_cap;
  }
  if (value < -k_pending_cap) {
    return -k_pending_cap;
  }
  return value;
}

void persist() {
  if (!pref::set_pending(pending)) {
    log::error("Pending persist failed");
  }
}

void apply_send() {
  if (pending < 0) {
    pending = 0;
  } else if (pending < k_pending_cap) {
    pending = pending + 1;
  }
}

void apply_receive() {
  if (pending > 0) {
    pending = 0;
  } else if (pending > -k_pending_cap) {
    pending = pending - 1;
  }
}

}  // namespace

void setup() {
  mqtt::setup();
  const int32_t stored = pref::get_pending();
  pending = clamp_pending(stored);
  if (pending != stored) {
    persist();
  }
}

void loop() {
  const uint32_t now = millis();
  if (polled && (now - last_poll_ms) < k_poll_ms) {
    return;
  }
  polled = true;
  last_poll_ms = now;

  mqtt::loop();
  uint8_t b = 0;
  if (mqtt::take_received(b)) {
    const int32_t before = pending;
    apply_receive();
    if (pending != before) {
      persist();
    }
    oled_ui::on_receive();
    log::warn("Hug received");
  }
}

void pause() { mqtt::stop(); }

void send() {
  if (!mqtt::publish_hug()) {
    return;
  }
  const int32_t before = pending;
  apply_send();
  if (pending != before) {
    persist();
  }
  oled_ui::on_send();
}

int32_t pending_for_display() { return pending; }

}  // namespace hug
}  // namespace hb
