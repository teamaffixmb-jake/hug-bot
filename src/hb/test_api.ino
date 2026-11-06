#include "test_api.h"
#include "api.h"
#include "log.h"
#include "sta.h"

#include <Arduino.h>
#include <cstdio>

namespace hb {
namespace test_api {
namespace {

constexpr uint32_t k_period_ms = 5000;
uint32_t next_log_ms = 0;

}  // namespace

void setup() {
  sta::setup();
  api::setup();
  api::start();
  sta::connect_saved();
  log::warn("Test 2: API state every 5s (read only)");
}

void loop() {
  sta::loop();
  if (millis() < next_log_ms) {
    return;
  }
  next_log_ms = millis() + k_period_ms;
  const api::view_t v = api::view();
  char line[64];
  snprintf(line, sizeof(line), "API %s pending %ld recv %lu seen %lu",
           api::online() ? "up" : "down", static_cast<long>(v.pending),
           static_cast<unsigned long>(v.received),
           static_cast<unsigned long>(v.seen));
  log::warn(line);
}

}  // namespace test_api
}  // namespace hb
