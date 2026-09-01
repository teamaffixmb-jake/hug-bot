#include "cp.h"

constexpr uint32_t kSerialBaud = 115200;

void setup() {
  Serial.begin(kSerialBaud);
  hb::cp::setup();
}

void loop() {
  hb::cp::loop();
}
