#include "test.h"
#include "log.h"
#include "test_cp.h"
#include "test_mqtt_rx.h"
#include "test_mqtt_tx.h"
#include "test_oled.h"
#include "test_pb.h"

namespace hb {

void test_entry_setup() {
#if RUN_MODE == 1
  test_cp::setup();
#elif RUN_MODE == 2
  test_mqtt_tx::setup();
#elif RUN_MODE == 3
  test_pb::setup();
#elif RUN_MODE == 4
  test_mqtt_rx::setup();
#elif RUN_MODE == 5
  test_oled::setup();
#else
  log::error("Unknown RUN_MODE");
#endif
}

void test_entry_loop() {
#if RUN_MODE == 1
  test_cp::loop();
#elif RUN_MODE == 2
  test_mqtt_tx::loop();
#elif RUN_MODE == 3
  test_pb::loop();
#elif RUN_MODE == 4
  test_mqtt_rx::loop();
#elif RUN_MODE == 5
  test_oled::loop();
#endif
}

}  // namespace hb
