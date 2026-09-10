#include "test_mqtt.h"
#include "log.h"
#include "mqtt.h"
#include "sta.h"

#include <Arduino.h>

namespace hb {
namespace test_mqtt {
namespace {

constexpr uint32_t k_retry_ms = 15000;
uint32_t next_pub_ms = 0;
bool sent = false;
bool passed = false;

}  // namespace

void setup() {
  sta::setup();
  mqtt::setup();
  sta::connect_saved();
  log::warn("Test 2: MQTT self inbox");
}

void loop() {
  sta::loop();
  mqtt::loop();
  if (passed || !mqtt::connected()) {
    return;
  }

  if (!sent) {
    (void)mqtt::take_received();
    if (!mqtt::publish_own_inbox()) {
      return;
    }
    sent = true;
    next_pub_ms = millis() + k_retry_ms;
    return;
  }

  if (mqtt::take_received()) {
    passed = true;
    log::warn("MQTT self-test pass");
    return;
  }

  if (millis() < next_pub_ms) {
    return;
  }
  next_pub_ms = millis() + k_retry_ms;
  sent = mqtt::publish_own_inbox();
}

}  // namespace test_mqtt
}  // namespace hb
