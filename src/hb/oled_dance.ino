#include "oled_dance.h"
#include "oled.h"

#include <Arduino.h>
#include <cstdint>

namespace hb {
namespace oled_dance {
namespace {

constexpr uint32_t k_frame_ms = 150;
constexpr uint8_t k_poses = 10;

struct pose_t {
  int16_t cx;
  int16_t lhx;
  int16_t lhy;
  int16_t rhx;
  int16_t rhy;
};

// Center rest → left wave → out → right wave → high → back.
constexpr pose_t k_pose[k_poses] = {
    {64, 56, 40, 72, 40},
    {56, 48, 12, 64, 38},
    {64, 48, 28, 80, 28},
    {72, 64, 38, 80, 12},
    {58, 50, 8, 66, 36},
    {70, 62, 36, 78, 8},
    {64, 52, 10, 76, 10},
    {52, 44, 16, 60, 40},
    {76, 68, 40, 84, 16},
    {64, 56, 40, 72, 40},
};

uint8_t frame = 0;
uint32_t last_ms = 0;
bool finished = false;

int16_t mix(int16_t a, int16_t b, uint8_t num, uint8_t den) {
  return static_cast<int16_t>(a + ((static_cast<int32_t>(b - a) * num) / den));
}

pose_t tween(const pose_t &a, const pose_t &b, uint8_t num, uint8_t den) {
  pose_t p{};
  p.cx = mix(a.cx, b.cx, num, den);
  p.lhx = mix(a.lhx, b.lhx, num, den);
  p.lhy = mix(a.lhy, b.lhy, num, den);
  p.rhx = mix(a.rhx, b.rhx, num, den);
  p.rhy = mix(a.rhy, b.rhy, num, den);
  return p;
}

void draw_figure(const pose_t &p) {
  constexpr int16_t k_head_y = 16;
  constexpr int16_t k_hip_y = 38;
  constexpr int16_t k_foot_y = 58;
  constexpr int16_t k_shoulder_y = 22;

  oled::line(p.cx, k_shoulder_y, p.cx, k_hip_y);
  oled::line(p.cx, k_hip_y, static_cast<int16_t>(p.cx - 7), k_foot_y);
  oled::line(p.cx, k_hip_y, static_cast<int16_t>(p.cx + 7), k_foot_y);
  oled::line(p.cx, k_shoulder_y, p.lhx, p.lhy);
  oled::line(p.cx, k_shoulder_y, p.rhx, p.rhy);
  oled::circle(p.cx, k_head_y, 6, false);
  oled::pixel(static_cast<int16_t>(p.cx - 2),
              static_cast<int16_t>(k_head_y - 1));
  oled::pixel(static_cast<int16_t>(p.cx + 2),
              static_cast<int16_t>(k_head_y - 1));
  oled::circle(p.lhx, p.lhy, 2, true);
  oled::circle(p.rhx, p.rhy, 2, true);
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

}  // namespace oled_dance
}  // namespace hb
