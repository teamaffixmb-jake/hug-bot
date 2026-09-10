#include "test_mqtt_tx.h"
#include "log.h"
#include "mqtt.h"
#include "sta.h"

#include <Arduino.h>
#include <cstdio>

namespace hb {
namespace test_mqtt_tx {
namespace {

constexpr uint32_t k_period_ms = 15000;
uint32_t next_pub_ms = 0;
uint8_t seq = 0;

}  // namespace

void setup() {
  sta::setup();
  mqtt::setup();
  mqtt::listen_none();
  sta::connect_saved();
  log::warn("Test 2: MQTT tx 0-255");
}

void loop() {
  sta::loop();
  mqtt::loop();
  if (!mqtt::connected()) {
    return;
  }
  if (millis() < next_pub_ms) {
    return;
  }
  next_pub_ms = millis() + k_period_ms;
  if (mqtt::publish_test_inbox(seq)) {
    char line[24];
    snprintf(line, sizeof(line), "MQTT sent %u", static_cast<unsigned>(seq));
    log::warn(line);
    seq = static_cast<uint8_t>(seq + 1);
  }
}

}  // namespace test_mqtt_tx
}  // namespace hb
