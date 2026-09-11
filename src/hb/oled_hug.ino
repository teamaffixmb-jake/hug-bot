#include "oled_hug.h"
#include "oled.h"

#include <Arduino.h>
#include <cstdint>

namespace hb {
namespace oled_hug {
namespace {

constexpr uint8_t k_cx = 64;
constexpr uint32_t k_frame_ms = 160;
constexpr uint8_t k_poses = 7;

struct pose_t {
  int16_t head_y;
  int16_t head_r;
  int16_t hip_y;
  int16_t foot_y;
  int16_t stance;
  int16_t lhx;
  int16_t lhy;
  int16_t rhx;
  int16_t rhy;
  int16_t lex;
  int16_t ley;
  int16_t rex;
  int16_t rey;
};

// Far → closer → reach → grab sides → wrap the rim → squeeze.
constexpr pose_t k_pose[k_poses] = {
    {22, 5, 40, 58, 7, 56, 38, 72, 38, 58, 32, 70, 32},
    {18, 7, 38, 58, 8, 44, 30, 84, 30, 50, 28, 78, 28},
    {15, 8, 37, 58, 9, 10, 26, 117, 26, 28, 24, 99, 24},
    {14, 9, 36, 58, 10, 1, 22, 126, 22, 14, 28, 113, 28},
    {13, 10, 36, 58, 10, 1, 2, 126, 2, 6, 16, 121, 16},
    {12, 11, 35, 58, 11, 22, 1, 105, 1, 3, 8, 124, 8},
    {11, 12, 34, 58, 11, 34, 1, 93, 1, 2, 6, 125, 6},
};

uint8_t frame = 0;
uint32_t last_ms = 0;
bool finished = false;

int16_t mix(int16_t a, int16_t b, uint8_t num, uint8_t den) {
  return static_cast<int16_t>(a + ((static_cast<int32_t>(b - a) * num) / den));
}

pose_t tween(const pose_t &a, const pose_t &b, uint8_t num, uint8_t den) {
  pose_t p{};
  p.head_y = mix(a.head_y, b.head_y, num, den);
  p.head_r = mix(a.head_r, b.head_r, num, den);
  p.hip_y = mix(a.hip_y, b.hip_y, num, den);
  p.foot_y = mix(a.foot_y, b.foot_y, num, den);
  p.stance = mix(a.stance, b.stance, num, den);
  p.lhx = mix(a.lhx, b.lhx, num, den);
  p.lhy = mix(a.lhy, b.lhy, num, den);
  p.rhx = mix(a.rhx, b.rhx, num, den);
  p.rhy = mix(a.rhy, b.rhy, num, den);
  p.lex = mix(a.lex, b.lex, num, den);
  p.ley = mix(a.ley, b.ley, num, den);
  p.rex = mix(a.rex, b.rex, num, den);
  p.rey = mix(a.rey, b.rey, num, den);
  return p;
}

void draw_figure(const pose_t &p) {
  const int16_t neck_y =
      static_cast<int16_t>(p.head_y + p.head_r + 1);
  const int16_t shoulder_y = static_cast<int16_t>(neck_y + 2);

  oled::rect(0, 0, 128, 64, false);

  oled::line(k_cx, neck_y, k_cx, p.hip_y);
  oled::line(k_cx, p.hip_y, static_cast<int16_t>(k_cx - p.stance), p.foot_y);
  oled::line(k_cx, p.hip_y, static_cast<int16_t>(k_cx + p.stance), p.foot_y);

  oled::line(k_cx, shoulder_y, p.lex, p.ley);
  oled::line(p.lex, p.ley, p.lhx, p.lhy);
  oled::line(k_cx, shoulder_y, p.rex, p.rey);
  oled::line(p.rex, p.rey, p.rhx, p.rhy);

  oled::circle(k_cx, p.head_y, p.head_r, false);
  if (p.head_r >= 7) {
    oled::pixel(static_cast<int16_t>(k_cx - 3),
                static_cast<int16_t>(p.head_y - 1));
    oled::pixel(static_cast<int16_t>(k_cx + 3),
                static_cast<int16_t>(p.head_y - 1));
  }
  if (p.head_r >= 10) {
    oled::pixel(static_cast<int16_t>(k_cx - 2),
                static_cast<int16_t>(p.head_y + 3));
    oled::pixel(k_cx, static_cast<int16_t>(p.head_y + 4));
    oled::pixel(static_cast<int16_t>(k_cx + 2),
                static_cast<int16_t>(p.head_y + 3));
  }

  oled::circle(p.lhx, p.lhy, 3, true);
  oled::circle(p.rhx, p.rhy, 3, true);
}

}  // namespace

void start() {
  frame = 0;
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

  const uint8_t i = static_cast<uint8_t>(frame / 2);
  const uint8_t n = static_cast<uint8_t>((i + 1) % k_poses);
  const pose_t p = tween(k_pose[i], k_pose[n], frame & 1, 2);

  oled::clear();
  draw_figure(p);
  oled::flush();

  frame = static_cast<uint8_t>((frame + 1) % (k_poses * 2));
  if (frame == 0) {
    finished = true;
  }
}

}  // namespace oled_hug
}  // namespace hb
