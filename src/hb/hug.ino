#include "hug.h"
#include "log.h"
#include "mqtt.h"

namespace hb {
namespace hug {

void setup() { mqtt::setup(); }

void loop() {
  mqtt::loop();
  uint8_t b = 0;
  if (mqtt::take_received(b)) {
    log::warn("Hug received");
  }
}

void send() { mqtt::publish_hug(); }

}  // namespace hug
}  // namespace hb
