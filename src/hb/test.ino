#include "test.h"
#include "log.h"
#include "test_cp.h"
#include "test_api.h"
#include "test_oled.h"
#include "test_oled_hug.h"
#include "test_oled_missed.h"
#include "test_oled_sending.h"
#include "test_oled_sent.h"
#include "test_pb.h"

namespace hb {

void test_entry_setup() {
#if RUN_MODE == 1
  test_cp::setup();
#elif RUN_MODE == 2
  test_api::setup();
#elif RUN_MODE == 3
  test_pb::setup();
#elif RUN_MODE == 5
  test_oled::setup();
#elif RUN_MODE == 6
  test_oled_hug::setup();
#elif RUN_MODE == 7
  test_oled_sent::setup();
#elif RUN_MODE == 8
  test_oled_sending::setup();
#elif RUN_MODE == 9
  test_oled_missed::setup();
#else
  log::error("Unknown RUN_MODE");
#endif
}

void test_entry_loop() {
#if RUN_MODE == 1
  test_cp::loop();
#elif RUN_MODE == 2
  test_api::loop();
#elif RUN_MODE == 3
  test_pb::loop();
#elif RUN_MODE == 5
  test_oled::loop();
#elif RUN_MODE == 6
  test_oled_hug::loop();
#elif RUN_MODE == 7
  test_oled_sent::loop();
#elif RUN_MODE == 8
  test_oled_sending::loop();
#elif RUN_MODE == 9
  test_oled_missed::loop();
#endif
}

}  // namespace hb
