#include "cp.h"
#include "hug.h"
#include "input.h"
#include "sta.h"

constexpr uint32_t k_serial_baud = 115200;

void setup() {
  Serial.begin(k_serial_baud);
  hb::sta::setup();
  hb::cp::setup();
  hb::hug::setup();
  hb::input::setup();
  hb::sta::connect_saved();
}

void loop() {
  hb::input::loop();
  hb::cp::loop();
  hb::sta::loop();
  hb::hug::loop();
}
