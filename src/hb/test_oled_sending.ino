#include "test_oled_sending.h"
#include "oled.h"
#include "oled_sending.h"

#include <Arduino.h>

namespace hb {
namespace test_oled_sending {

void setup() {
  oled::setup();
  if (!oled::ready()) {
    Serial.println("OLED not found — check A4/A5 and Vin/GND");
    return;
  }
  oled_sending::setup();
  Serial.println("OLED sending loader");
}

void loop() { oled_sending::loop(); }

}  // namespace test_oled_sending
}  // namespace hb
