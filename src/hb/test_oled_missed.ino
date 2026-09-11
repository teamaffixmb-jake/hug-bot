#include "test_oled_missed.h"
#include "oled.h"
#include "oled_missed.h"

#include <Arduino.h>

namespace hb {
namespace test_oled_missed {

void setup() {
  oled::setup();
  if (!oled::ready()) {
    Serial.println("OLED not found — check A4/A5 and Vin/GND");
    return;
  }
  oled_missed::set_count(3);
  oled_missed::setup();
  Serial.println("OLED missed hugs");
}

void loop() { oled_missed::loop(); }

}  // namespace test_oled_missed
}  // namespace hb
