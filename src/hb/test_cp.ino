#include "test_cp.h"
#include "cp.h"
#include "log.h"
#include "sta.h"

#include <Arduino.h>

namespace hb {
namespace test_cp {
namespace {

constexpr uint32_t k_toggle_ms = 60000;
uint32_t next_ms = 0;

}  // namespace

void setup() {
  sta::setup();
  cp::setup();
  log::warn("Test 1: cp toggle 60s");
}

void loop() {
  cp::loop();
  sta::loop();
  if (millis() < next_ms) {
    return;
  }
  next_ms = millis() + k_toggle_ms;
  cp::toggle();
  log::warn(cp::enabled() ? "Portal on" : "Portal off");
}

}  // namespace test_cp
}  // namespace hb
