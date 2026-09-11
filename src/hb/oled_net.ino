#include "oled_net.h"
#include "oled.h"

#include <Arduino.h>
#include <cstdint>

namespace hb {
namespace oled_net {
namespace {

constexpr uint32_t k_frame_ms = 140;
constexpr uint8_t k_wait_frames = 8;
constexpr uint8_t k_shot_frames = 14;
constexpr int16_t k_cx = 36;
constexpr int16_t k_ix = 96;

kind_t kind = kind_t::wifi_wait;
uint8_t frame = 0;
uint32_t last_ms = 0;
bool finished = false;
bool oneshot = false;

bool is_oneshot(kind_t k) {
  return k == kind_t::wifi_ok || k == kind_t::wifi_fail ||
         k == kind_t::mqtt_ok || k == kind_t::mqtt_fail;
}

const char *label() {
  switch (kind) {
  case kind_t::wifi_wait:
    return "wifi...";
  case kind_t::wifi_ok:
    return "wifi ok";
  case kind_t::wifi_fail:
    return "wifi failed";
  case kind_t::mqtt_wait:
    return "mqtt...";
  case kind_t::mqtt_ok:
    return "mqtt ok";
  case kind_t::mqtt_fail:
    return "mqtt failed";
  }
  return "";
}

int16_t label_x() {
  const char *s = label();
  uint8_t n = 0;
  while (s[n] != '\0') {
    ++n;
  }
  return static_cast<int16_t>((128 - static_cast<int16_t>(n * 6)) / 2);
}

void draw_figure() {
  constexpr int16_t k_head_y = 14;
  constexpr int16_t k_hip_y = 36;
  constexpr int16_t k_foot_y = 50;
  constexpr int16_t k_shoulder_y = 20;
  oled::line(k_cx, k_shoulder_y, k_cx, k_hip_y);
  oled::line(k_cx, k_hip_y, static_cast<int16_t>(k_cx - 7), k_foot_y);
  oled::line(k_cx, k_hip_y, static_cast<int16_t>(k_cx + 7), k_foot_y);
  oled::line(k_cx, k_shoulder_y, static_cast<int16_t>(k_cx - 12), 32);
  oled::line(k_cx, k_shoulder_y, static_cast<int16_t>(k_cx + 12), 32);
  oled::circle(k_cx, k_head_y, 6, false);
}

void draw_wifi(uint8_t lit) {
  const int16_t x = k_ix;
  const int16_t y = 28;
  oled::pixel(x, static_cast<int16_t>(y + 12));
  if (lit >= 1) {
    oled::circle(x, y, 4, false);
  }
  if (lit >= 2) {
    oled::circle(x, y, 8, false);
  }
  if (lit >= 3) {
    oled::circle(x, y, 12, false);
  }
}

void draw_lock(bool closed) {
  const int16_t x = k_ix;
  const int16_t y = 22;
  oled::circle(x, y, 6, false);
  if (closed) {
    oled::line(static_cast<int16_t>(x - 6), y, static_cast<int16_t>(x + 6), y);
  }
  oled::rect(static_cast<int16_t>(x - 8), static_cast<int16_t>(y + 4), 16, 14,
             false);
  oled::pixel(x, static_cast<int16_t>(y + 10));
}

void draw_x() {
  const int16_t x = k_ix;
  const int16_t y = 28;
  oled::line(static_cast<int16_t>(x - 10), static_cast<int16_t>(y - 8),
             static_cast<int16_t>(x + 10), static_cast<int16_t>(y + 12));
  oled::line(static_cast<int16_t>(x + 10), static_cast<int16_t>(y - 8),
             static_cast<int16_t>(x - 10), static_cast<int16_t>(y + 12));
}

void draw() {
  oled::clear();
  draw_figure();
  switch (kind) {
  case kind_t::wifi_wait:
    draw_wifi(static_cast<uint8_t>((frame % 4) + 1));
    break;
  case kind_t::wifi_ok:
    draw_wifi(3);
    break;
  case kind_t::wifi_fail:
    draw_wifi(1);
    draw_x();
    break;
  case kind_t::mqtt_wait:
    draw_lock((frame & 1) == 0);
    break;
  case kind_t::mqtt_ok:
    draw_lock(true);
    break;
  case kind_t::mqtt_fail:
    draw_lock(false);
    draw_x();
    break;
  }
  oled::print_at(label_x(), 54, 1, label());
  oled::flush();
}

}  // namespace

void start(kind_t k) {
  kind = k;
  frame = 0;
  last_ms = 0;
  finished = false;
  oneshot = is_oneshot(k);
}

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

  draw();

  if (oneshot) {
    frame = static_cast<uint8_t>(frame + 1);
    if (frame >= k_shot_frames) {
      frame = 0;
      finished = true;
    }
  } else {
    frame = static_cast<uint8_t>((frame + 1) % k_wait_frames);
  }
}

}  // namespace oled_net
}  // namespace hb
