#include "cp.h"
#include "sta.h"

constexpr uint32_t kSerialBaud = 115200;

void setup() {
  Serial.begin(kSerialBaud);
  hb::sta::setup();
  hb::cp::setup();
  hb::sta::ConnectSaved();
}

void loop() {
  hb::cp::loop();
  hb::sta::loop();
}
