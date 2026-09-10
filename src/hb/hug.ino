#include "hug.h"
#include "log.h"
#include "mqtt.h"

#include <Arduino.h>
#include <cstdint>

namespace hb {
namespace hug {
namespace {

constexpr uint32_t k_poll_ms = 3000;
uint32_t last_poll_ms = 0;
bool polled = false;

}  // namespace

void setup() { mqtt::setup(); }

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
    log::warn("Hug received");
  }
}

void pause() { mqtt::stop(); }

void send() { mqtt::publish_hug(); }

}  // namespace hug
}  // namespace hb
