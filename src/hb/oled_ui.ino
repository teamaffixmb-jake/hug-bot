#include "oled_ui.h"
#include "hug.h"
#include "api.h"
#include "oled.h"
#include "oled_dance.h"
#include "oled_hug.h"
#include "oled_missed.h"
#include "oled_net.h"
#include "oled_pending.h"
#include "oled_sending.h"
#include "oled_sent.h"
#include "sta.h"

#include <Arduino.h>
#include <WiFi.h>
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
  pending,
  stopped,
  wifiWait,
  wifiOk,
  wifiFail,
  awsWait,
  awsOk,
  awsFail,
};

constexpr uint32_t k_dance_wait_ms = 30000;
constexpr uint32_t k_send_loader_ms = 1500;
// "hug sent!" only plays once AWS has the hug. If it takes longer than this
// the loader gives up; the press keeps retrying in the background.
constexpr uint32_t k_send_give_up_ms = 15000;
constexpr uint32_t k_wifi_hold_ms = 2000;
constexpr uint8_t k_hug_loops = 5;

state_t state = state_t::stopped;
bool portal_shown = false;
bool boot_movie = true;
uint32_t idle_entered_ms = 0;
uint32_t send_started = 0;
uint8_t hug_loops_left = 0;
bool hold_armed = false;
uint32_t hold_start = 0;

void refresh_missed_count() {
  const int32_t p = hug::pending_for_display();
  if (p < 0) {
    oled_missed::set_count(static_cast<uint32_t>(-p));
  }
}

void refresh_pending_count() {
  const int32_t p = hug::pending_for_display();
  if (p > 0) {
    oled_pending::set_count(static_cast<uint32_t>(p));
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

bool sta_up() { return WiFi.status() == WL_CONNECTED; }

bool wifi_hard_fail() {
  const wl_status_t s = WiFi.status();
  return s == WL_CONNECT_FAILED || s == WL_NO_SSID_AVAIL ||
         s == WL_CONNECTION_LOST;
}

void disarm_hold() { hold_armed = false; }

void enter_missed() {
  refresh_missed_count();
  oled_missed::setup();
  state = state_t::missed;
}

void enter_pending() {
  refresh_pending_count();
  oled_pending::setup();
  state = state_t::pending;
}

void go_rest() {
  disarm_hold();
  const int32_t p = hug::pending_for_display();
  if (p < 0) {
    enter_missed();
    return;
  }
  if (p > 0) {
    enter_pending();
    return;
  }
  if (state != state_t::idle) {
    paint_idle();
    idle_entered_ms = millis();
    state = state_t::idle;
  }
}

void arm_hold_if_up() {
  if (sta_up()) {
    hold_armed = true;
    hold_start = millis();
  } else {
    hold_armed = false;
  }
}

void enter_wifi_wait() {
  disarm_hold();
  oled_net::start(oled_net::kind_t::wifi_wait);
  state = state_t::wifiWait;
  arm_hold_if_up();
  oled_net::loop();
}

void enter_wifi_ok() {
  disarm_hold();
  oled_net::start(oled_net::kind_t::wifi_ok);
  state = state_t::wifiOk;
  oled_net::loop();
}

void enter_wifi_fail() {
  disarm_hold();
  oled_net::start(oled_net::kind_t::wifi_fail);
  state = state_t::wifiFail;
  oled_net::loop();
}

void enter_aws_wait() {
  disarm_hold();
  oled_net::start(oled_net::kind_t::aws_wait);
  state = state_t::awsWait;
  oled_net::loop();
}

void enter_aws_ok() {
  oled_net::start(oled_net::kind_t::aws_ok);
  state = state_t::awsOk;
  oled_net::loop();
}

void enter_aws_fail() {
  oled_net::start(oled_net::kind_t::aws_fail);
  state = state_t::awsFail;
  oled_net::loop();
}

bool hug_clip() {
  return state == state_t::sending || state == state_t::sent ||
         state == state_t::received;
}

bool wifi_clip() {
  return state == state_t::wifiWait || state == state_t::wifiOk ||
         state == state_t::wifiFail;
}

void leave_stopped() {
  if (boot_movie) {
    if (!sta_up()) {
      boot_movie = false;
      enter_wifi_fail();
    } else {
      boot_movie = false;
      enter_wifi_wait();
    }
    return;
  }
  if (api::online()) {
    go_rest();
    return;
  }
  if (sta_up()) {
    enter_aws_wait();
    return;
  }
  enter_wifi_wait();
}

void poll_wifi_wait() {
  oled_net::loop();
  if (!sta_up()) {
    hold_armed = false;
    if (wifi_hard_fail()) {
      enter_wifi_fail();
    }
    return;
  }
  if (!hold_armed) {
    hold_armed = true;
    hold_start = millis();
  }
  if ((millis() - hold_start) >= k_wifi_hold_ms) {
    enter_wifi_ok();
  }
}

void poll_rest_radio() {
  if (!sta_up()) {
    enter_wifi_wait();
    return;
  }
  if (!api::online()) {
    enter_aws_wait();
  }
}

}  // namespace

void setup() {
  state = state_t::stopped;
  portal_shown = false;
  boot_movie = true;
}

void resume() {
  portal_shown = false;
  state = state_t::stopped;
}

void stop() {
  boot_movie = false;
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
  boot_movie = false;
  if (hug_clip()) {
    return;
  }
  if (!oled::ready()) {
    return;
  }
  disarm_hold();
  oled_sending::setup();
  send_started = millis();
  state = state_t::sending;
}

bool on_receive() {
  // Only from rest, so a catch-up hug never cuts off the "connected" clip.
  if (state != state_t::idle && state != state_t::dance &&
      state != state_t::missed && state != state_t::pending) {
    return false;
  }
  if (!oled::ready()) {
    return false;
  }
  boot_movie = false;
  disarm_hold();
  refresh_missed_count();
  oled_hug::start();
  hug_loops_left = k_hug_loops;
  state = state_t::received;
  return true;
}

bool on_aws_connecting() {
  if (state == state_t::awsWait) {
    return true;
  }
  if (state == state_t::stopped || hug_clip() || wifi_clip() ||
      state == state_t::awsOk || state == state_t::awsFail) {
    return false;
  }
  if (!sta_up() || api::online()) {
    return false;
  }
  enter_aws_wait();
  return true;
}

void loop() {
  if (state == state_t::stopped) {
    if (portal_shown) {
      return;
    }
    leave_stopped();
  }

  const uint32_t now = millis();

  switch (state) {
  case state_t::idle: {
    const int32_t p = hug::pending_for_display();
    if (p < 0) {
      enter_missed();
      break;
    }
    if (p > 0) {
      enter_pending();
      break;
    }
    poll_rest_radio();
    if (state != state_t::idle) {
      break;
    }
    if ((now - idle_entered_ms) >= k_dance_wait_ms) {
      oled_dance::start();
      state = state_t::dance;
    }
    break;
  }

  case state_t::dance:
    poll_rest_radio();
    if (state != state_t::dance) {
      break;
    }
    oled_dance::loop();
    if (oled_dance::done()) {
      go_rest();
    }
    break;

  case state_t::sending:
    oled_sending::loop();
    if ((now - send_started) < k_send_loader_ms) {
      break;
    }
    if (hug::send_settled()) {
      oled_sent::start();
      state = state_t::sent;
    } else if ((now - send_started) >= k_send_give_up_ms) {
      go_rest();
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
        hug::receive_shown();
        go_rest();
      }
    }
    break;

  case state_t::missed: {
    if (!sta_up()) {
      enter_wifi_wait();
      break;
    }
    if (!api::online()) {
      enter_aws_wait();
      break;
    }
    const int32_t p = hug::pending_for_display();
    if (p >= 0) {
      go_rest();
      break;
    }
    refresh_missed_count();
    oled_missed::loop();
    break;
  }

  case state_t::pending: {
    if (!sta_up()) {
      enter_wifi_wait();
      break;
    }
    if (!api::online()) {
      enter_aws_wait();
      break;
    }
    const int32_t p = hug::pending_for_display();
    if (p <= 0) {
      go_rest();
      break;
    }
    refresh_pending_count();
    oled_pending::loop();
    break;
  }

  case state_t::wifiWait:
    poll_wifi_wait();
    break;

  case state_t::wifiOk:
    oled_net::loop();
    if (oled_net::done()) {
      if (!api::online()) {
        enter_aws_wait();
      } else {
        go_rest();
      }
    }
    break;

  case state_t::wifiFail:
    oled_net::loop();
    if (oled_net::done()) {
      if (sta::wanted()) {
        enter_wifi_wait();
      } else {
        go_rest();
      }
    }
    break;

  case state_t::awsWait:
    if (!sta_up()) {
      enter_wifi_wait();
      break;
    }
    oled_net::loop();
    if (api::online()) {
      enter_aws_ok();
      break;
    }
    if (api::take_connect_attempt() == api::connect_attempt_t::fail) {
      enter_aws_fail();
    }
    break;

  case state_t::awsOk:
    if (!sta_up()) {
      enter_wifi_wait();
      break;
    }
    oled_net::loop();
    if (oled_net::done()) {
      go_rest();
    }
    break;

  case state_t::awsFail:
    if (!sta_up()) {
      enter_wifi_wait();
      break;
    }
    oled_net::loop();
    if (oled_net::done()) {
      enter_aws_wait();
    }
    break;

  case state_t::stopped:
    break;
  }
}

}  // namespace oled_ui
}  // namespace hb
