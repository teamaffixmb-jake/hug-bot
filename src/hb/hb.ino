// 0 = production (main_entry)
// 1 = cp toggle every 60s
// 2 = MQTT publish to own inbox and wait to receive it
#define RUN_MODE 1

#include "main.h"
#include "test.h"

#include <Arduino.h>

constexpr uint32_t k_serial_baud = 115200;

void setup() {
  Serial.begin(k_serial_baud);
#if RUN_MODE == 0
  hb::main_entry_setup();
#else
  hb::test_entry_setup();
#endif
}

void loop() {
#if RUN_MODE == 0
  hb::main_entry_loop();
#else
  hb::test_entry_loop();
#endif
}
