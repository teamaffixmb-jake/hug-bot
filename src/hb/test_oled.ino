#include "test_oled.h"
#include "oled.h"

#include <Arduino.h>

namespace hb {
namespace test_oled {

void setup() {
  oled::setup();
  if (!oled::ready()) {
    Serial.println("OLED not found — check A4/A5 and Vin/GND");
    return;
  }

  oled::clear();
  oled::print_at(10, 25, 2, "Hello!");
  oled::flush();
  Serial.println("Display initialized OK");
}

void loop() {}

}  // namespace test_oled
}  // namespace hb
