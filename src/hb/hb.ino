// 0 = production (main_entry)
// 1 = cp toggle every 60s
// 2 = MQTT tx to hugbot/<id>/test-tx every 15s
// 3 = pb held: log every 100ms
// 4 = MQTT rx on hugbot/<id>/test-rx
// 5 = OLED I2C "Hello!"
// 6 = OLED stick-figure hug animation
// 7 = OLED "hug sent!" fly-by
// 8 = OLED hug-sending loader
// 9 = OLED "N missed hugs" pulse
#define RUN_MODE 9

#include "main.h"
#include "test.h"

#include <Arduino.h>

constexpr uint32_t k_serial_baud = 115200;

void setup() {
  Serial.begin(k_serial_baud);
  delay(2000);
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
