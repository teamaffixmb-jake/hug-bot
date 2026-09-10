#include "test.h"
#include "log.h"
#include "test_cp.h"
#include "test_mqtt.h"
#include "test_pb.h"

namespace hb {

void test_entry_setup() {
#if RUN_MODE == 1
  test_cp::setup();
#elif RUN_MODE == 2
  test_mqtt::setup();
#elif RUN_MODE == 3
  test_pb::setup();
#else
  log::error("Unknown RUN_MODE");
#endif
}

void test_entry_loop() {
#if RUN_MODE == 1
  test_cp::loop();
#elif RUN_MODE == 2
  test_mqtt::loop();
#elif RUN_MODE == 3
  test_pb::loop();
#endif
}

}  // namespace hb
