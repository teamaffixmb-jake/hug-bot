#include "test_mqtt_rx.h"
#include "log.h"
#include "mqtt.h"
#include "sta.h"

#include <Arduino.h>
#include <cstdio>

namespace hb {
namespace test_mqtt_rx {

void setup() {
  sta::setup();
  mqtt::setup();
  mqtt::listen_test_inbox();
  sta::connect_saved();
  log::warn("Test 4: MQTT rx test inbox");
}

void loop() {
  sta::loop();
  mqtt::loop();
  uint8_t b = 0;
  if (!mqtt::take_received(b)) {
    return;
  }
  char line[24];
  snprintf(line, sizeof(line), "MQTT recv %u", static_cast<unsigned>(b));
  log::warn(line);
}

}  // namespace test_mqtt_rx
}  // namespace hb
