#include "oled_sent.h"
#include "oled.h"

#include <Arduino.h>
#include <cstdint>

namespace hb {
namespace oled_sent {
namespace {

constexpr int16_t k_text_w = 108;
constexpr int16_t k_start_x = 128;
constexpr int16_t k_end_x = -k_text_w - 16;
constexpr uint32_t k_frame_ms = 40;
constexpr int16_t k_y = 24;

int16_t x = k_start_x;
uint32_t last_ms = 0;
bool finished = false;

void draw_heart(int16_t hx, int16_t hy) {
  oled::circle(static_cast<int16_t>(hx - 4), hy, 4, true);
  oled::circle(static_cast<int16_t>(hx + 4), hy, 4, true);
  oled::line(static_cast<int16_t>(hx - 8), static_cast<int16_t>(hy + 1), hx,
             static_cast<int16_t>(hy + 12));
  oled::line(hx, static_cast<int16_t>(hy + 12), static_cast<int16_t>(hx + 8),
             static_cast<int16_t>(hy + 1));
  for (int16_t i = 0; i <= 10; ++i) {
    const int16_t w = static_cast<int16_t>(8 - (i * 8) / 10);
    oled::line(static_cast<int16_t>(hx - w), static_cast<int16_t>(hy + 2 + i),
               static_cast<int16_t>(hx + w), static_cast<int16_t>(hy + 2 + i));
  }
}

}  // namespace

void start() {
  x = k_start_x;
  last_ms = 0;
  finished = false;
}

void setup() { start(); }

bool done() { return finished; }

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
  draw_heart(static_cast<int16_t>(x + 118), static_cast<int16_t>(k_y + 4));
  oled::print_at(x, k_y, 2, "hug sent!");
  oled::flush();

  x = static_cast<int16_t>(x - 4);
  if (x < k_end_x) {
    x = k_start_x;
    finished = true;
  }
}

}  // namespace oled_sent
}  // namespace hb
