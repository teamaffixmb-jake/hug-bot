#include "test_pb.h"
#include "log.h"
#include "pb.h"

#include <Arduino.h>

namespace hb {
namespace test_pb {
namespace {

constexpr uint32_t k_log_ms = 100;
uint32_t next_ms = 0;

}  // namespace

void setup() {
  pb::setup();
  log::warn("Test 3: hold pb, log every 100ms");
}

void loop() {
  if (!pb::down()) {
    next_ms = 0;
    return;
  }
  if (next_ms != 0 && millis() < next_ms) {
    return;
  }
  next_ms = millis() + k_log_ms;
  log::warn("pb down");
}

}  // namespace test_pb
}  // namespace hb
