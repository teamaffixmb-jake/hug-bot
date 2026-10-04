#include "oled_pending.h"
#include "oled.h"

#include <Arduino.h>
#include <cstdio>
#include <cstring>

namespace hb {
namespace oled_pending {
namespace {

constexpr uint32_t k_frame_ms = 80;
constexpr uint8_t k_pulse_steps = 24;
constexpr int16_t k_label_x = 16;
constexpr int16_t k_label_y = 36;

uint32_t count = 0;
char num[12];
uint8_t pulse = 0;
int8_t pulse_dir = 1;
uint32_t last_ms = 0;

void format_num() {
  snprintf(num, sizeof(num), "%lu", static_cast<unsigned long>(count));
}

int16_t num_x() {
  const int16_t w =
      static_cast<int16_t>(static_cast<int16_t>(strlen(num)) * 12);
  return static_cast<int16_t>((128 - w) / 2);
}

void draw(bool bigger) {
  const int16_t x = num_x();
  const int16_t y = 12;
  oled::print_at(x, y, 2, num);
  if (bigger) {
    oled::print_at(static_cast<int16_t>(x - 1), static_cast<int16_t>(y - 1),
                   2, num);
  }
  oled::print_at(k_label_x, k_label_y, 1, "pending response");
}

}  // namespace

void setup() {
  pulse = k_pulse_steps / 2;
  pulse_dir = 1;
  last_ms = 0;
  format_num();
}

void set_count(uint32_t n) {
  count = n;
  format_num();
}

void loop() {
  if (!oled::ready()) {
    return;
  }

  const uint32_t now = millis();
  if (last_ms != 0 && (now - last_ms) < k_frame_ms) {
    return;
  }
  last_ms = now;

  oled::clear();
  draw(pulse >= (k_pulse_steps / 2));
  oled::flush();

  pulse = static_cast<uint8_t>(pulse + pulse_dir);
  if (pulse == 0 || pulse == k_pulse_steps - 1) {
    pulse_dir = static_cast<int8_t>(-pulse_dir);
  }
}

}  // namespace oled_pending
}  // namespace hb
