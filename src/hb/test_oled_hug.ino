#include "test_oled_hug.h"
#include "oled.h"
#include "oled_hug.h"

#include <Arduino.h>

namespace hb {
namespace test_oled_hug {

void setup() {
  oled::setup();
  if (!oled::ready()) {
    Serial.println("OLED not found — check A4/A5 and Vin/GND");
    return;
  }
  oled_hug::setup();
  Serial.println("OLED hug anim");
}

void loop() { oled_hug::loop(); }

}  // namespace test_oled_hug
}  // namespace hb
