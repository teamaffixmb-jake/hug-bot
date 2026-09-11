#include "oled_ui.h"
#include "hug.h"
#include "oled.h"
#include "oled_dance.h"
#include "oled_hug.h"
#include "oled_missed.h"
#include "oled_sending.h"
#include "oled_sent.h"

#include <Arduino.h>
#include <cstdint>

namespace hb {
namespace oled_ui {
namespace {

enum class state_t : uint8_t {
  idle,
  dance,
  sending,
  sent,
  received,
  missed,
  stopped,
};

constexpr uint32_t k_dance_wait_ms = 30000;
constexpr uint32_t k_send_loader_ms = 1500;
constexpr uint32_t k_missed_wait_ms = 60000;
constexpr int32_t k_missed_grace = -6;
constexpr uint8_t k_hug_loops = 5;

state_t state = state_t::stopped;
bool portal_shown = false;
bool missed_armed = false;
uint32_t missed_start = 0;
uint32_t idle_entered_ms = 0;
uint32_t send_started = 0;
uint8_t hug_loops_left = 0;

void sync_miss_timer() {
  const int32_t p = hug::pending_for_display();
  if (p >= 0) {
    missed_armed = false;
  } else if (!missed_armed) {
    missed_armed = true;
    missed_start = millis();
  }
}

bool missed_eligible() {
  const int32_t p = hug::pending_for_display();
  if (p > k_missed_grace) {
    return false;
  }
  if (!missed_armed) {
    return false;
  }
  return (millis() - missed_start) >= k_missed_wait_ms;
}

void refresh_missed_count() {
  const int32_t p = hug::pending_for_display();
  if (p < 0) {
    oled_missed::set_count(static_cast<uint32_t>(-p));
  }
}

void paint_idle() {
  if (!oled::ready()) {
    return;
  }
  oled::clear();
  oled::flush();
}

void paint_portal() {
  if (!oled::ready()) {
    return;
  }
  oled::clear();
  oled::print_at(22, 20, 1, "captive portal");
  oled::print_at(46, 36, 1, "active");
  oled::flush();
}

void enter_missed() {
  refresh_missed_count();
  oled_missed::setup();
  state = state_t::missed;
}

void go_rest() {
  if (missed_eligible()) {
    enter_missed();
    return;
  }
  if (state != state_t::idle) {
    paint_idle();
    idle_entered_ms = millis();
    state = state_t::idle;
  }
}

}  // namespace

void setup() {
  state = state_t::stopped;
  portal_shown = false;
}

void resume() {
  portal_shown = false;
  state = state_t::stopped;
}

void stop() {
  state = state_t::stopped;
  if (portal_shown) {
    return;
  }
  portal_shown = true;
  paint_portal();
}

void on_send() {
  if (state == state_t::stopped) {
    return;
  }
  sync_miss_timer();
  if (!oled::ready()) {
    return;
  }
  if (state == state_t::sending || state == state_t::sent ||
      state == state_t::received) {
    return;
  }
  oled_sending::setup();
  send_started = millis();
  state = state_t::sending;
}

void on_receive() {
  if (state == state_t::stopped) {
    return;
  }
  sync_miss_timer();
  if (!oled::ready()) {
    return;
  }
  refresh_missed_count();
  oled_hug::start();
  hug_loops_left = k_hug_loops;
  state = state_t::received;
}

void loop() {
  if (state == state_t::stopped) {
    if (portal_shown) {
      return;
    }
    sync_miss_timer();
    go_rest();
  }

  const uint32_t now = millis();

  switch (state) {
  case state_t::idle:
    if (missed_eligible()) {
      enter_missed();
      break;
    }
    if ((now - idle_entered_ms) >= k_dance_wait_ms) {
      oled_dance::start();
      state = state_t::dance;
    }
    break;

  case state_t::dance:
    oled_dance::loop();
    if (oled_dance::done()) {
      go_rest();
    }
    break;

  case state_t::sending:
    oled_sending::loop();
    if ((now - send_started) >= k_send_loader_ms) {
      oled_sent::start();
      state = state_t::sent;
    }
    break;

  case state_t::sent:
    oled_sent::loop();
    if (oled_sent::done()) {
      go_rest();
    }
    break;

  case state_t::received:
    oled_hug::loop();
    if (oled_hug::done()) {
      hug_loops_left = static_cast<uint8_t>(hug_loops_left - 1);
      if (hug_loops_left > 0) {
        oled_hug::start();
      } else {
        go_rest();
      }
    }
    break;

  case state_t::missed:
    if (!missed_eligible()) {
      go_rest();
      break;
    }
    oled_missed::loop();
    break;

  case state_t::stopped:
    break;
  }
}

}  // namespace oled_ui
}  // namespace hb
