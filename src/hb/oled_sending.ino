#include "oled_sending.h"
#include "oled.h"

#include <Arduino.h>
#include <cstdint>

namespace hb {
namespace oled_sending {
namespace {

constexpr uint32_t k_frame_ms = 120;
constexpr uint8_t k_ring = 8;
constexpr int16_t k_cx = 64;
constexpr int16_t k_cy = 24;

// 8 points on a circle r≈16 (no floats).
constexpr int8_t k_dx[k_ring] = {16, 11, 0, -11, -16, -11, 0, 11};
constexpr int8_t k_dy[k_ring] = {0, -11, -16, -11, 0, 11, 16, 11};

uint8_t tick = 0;
uint32_t last_ms = 0;

void draw_heart(int16_t hx, int16_t hy) {
  oled::circle(static_cast<int16_t>(hx - 3), hy, 3, true);
  oled::circle(static_cast<int16_t>(hx + 3), hy, 3, true);
  for (int16_t i = 0; i <= 8; ++i) {
    const int16_t w = static_cast<int16_t>(6 - (i * 6) / 8);
    oled::line(static_cast<int16_t>(hx - w), static_cast<int16_t>(hy + 1 + i),
               static_cast<int16_t>(hx + w), static_cast<int16_t>(hy + 1 + i));
  }
}

}  // namespace

void setup() { tick = 0; }

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
  draw_heart(k_cx, k_cy);

  for (uint8_t i = 0; i < k_ring; ++i) {
    const int16_t x = static_cast<int16_t>(k_cx + k_dx[i]);
    const int16_t y = static_cast<int16_t>(k_cy + k_dy[i]);
    const bool head = i == tick;
    oled::circle(x, y, head ? 2 : 1, head);
  }

  oled::print_at(28, 52, 1, "sending");
  const uint8_t dots = static_cast<uint8_t>((tick % 4));
  if (dots >= 1) {
    oled::print_at(76, 52, 1, ".");
  }
  if (dots >= 2) {
    oled::print_at(82, 52, 1, ".");
  }
  if (dots >= 3) {
    oled::print_at(88, 52, 1, ".");
  }

  oled::flush();
  tick = static_cast<uint8_t>((tick + 1) % k_ring);
}

}  // namespace oled_sending
}  // namespace hb
