#include "test_oled_sent.h"
#include "oled.h"
#include "oled_sent.h"

#include <Arduino.h>

namespace hb {
namespace test_oled_sent {

void setup() {
  oled::setup();
  if (!oled::ready()) {
    Serial.println("OLED not found — check A4/A5 and Vin/GND");
    return;
  }
  oled_sent::setup();
  Serial.println("OLED hug-sent anim");
}

void loop() { oled_sent::loop(); }

}  // namespace test_oled_sent
}  // namespace hb
