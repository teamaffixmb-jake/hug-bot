#include "oled_ui.h"
#include "hug.h"
#include "mqtt.h"
#include "oled.h"
#include "oled_dance.h"
#include "oled_hug.h"
#include "oled_missed.h"
#include "oled_net.h"
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
  stopped,
  wifiWait,
  wifiOk,
  wifiFail,
  mqttWait,
  mqttOk,
  mqttFail,
};

constexpr uint32_t k_dance_wait_ms = 30000;
constexpr uint32_t k_send_loader_ms = 1500;
constexpr uint32_t k_missed_wait_ms = 60000;
constexpr uint32_t k_wifi_hold_ms = 2000;
constexpr int32_t k_missed_grace = -6;
constexpr uint8_t k_hug_loops = 5;

state_t state = state_t::stopped;
bool portal_shown = false;
bool boot_movie = true;
bool missed_armed = false;
uint32_t missed_start = 0;
uint32_t idle_entered_ms = 0;
uint32_t send_started = 0;
uint8_t hug_loops_left = 0;
bool hold_armed = false;
uint32_t hold_start = 0;

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

void go_rest() {
  disarm_hold();
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

void enter_mqtt_wait() {
  disarm_hold();
  oled_net::start(oled_net::kind_t::mqtt_wait);
  state = state_t::mqttWait;
  oled_net::loop();
}

void enter_mqtt_ok() {
  oled_net::start(oled_net::kind_t::mqtt_ok);
  state = state_t::mqttOk;
  oled_net::loop();
}

void enter_mqtt_fail() {
  oled_net::start(oled_net::kind_t::mqtt_fail);
  state = state_t::mqttFail;
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
  sync_miss_timer();
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
  if (mqtt::connected()) {
    go_rest();
    return;
  }
  if (sta_up()) {
    enter_mqtt_wait();
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
  if (!mqtt::connected()) {
    enter_mqtt_wait();
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
  sync_miss_timer();
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

void on_receive() {
  if (state == state_t::stopped) {
    return;
  }
  boot_movie = false;
  sync_miss_timer();
  if (!oled::ready()) {
    return;
  }
  disarm_hold();
  refresh_missed_count();
  oled_hug::start();
  hug_loops_left = k_hug_loops;
  state = state_t::received;
}

bool on_mqtt_connecting() {
  if (state == state_t::mqttWait) {
    return true;
  }
  if (state == state_t::stopped || hug_clip() || wifi_clip() ||
      state == state_t::mqttOk || state == state_t::mqttFail) {
    return false;
  }
  if (!sta_up() || mqtt::connected()) {
    return false;
  }
  enter_mqtt_wait();
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
  case state_t::idle:
    if (missed_eligible()) {
      enter_missed();
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
    if (!sta_up()) {
      enter_wifi_wait();
      break;
    }
    if (!mqtt::connected()) {
      enter_mqtt_wait();
      break;
    }
    if (!missed_eligible()) {
      go_rest();
      break;
    }
    oled_missed::loop();
    break;

  case state_t::wifiWait:
    poll_wifi_wait();
    break;

  case state_t::wifiOk:
    oled_net::loop();
    if (oled_net::done()) {
      if (!mqtt::connected()) {
        enter_mqtt_wait();
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

  case state_t::mqttWait:
    if (!sta_up()) {
      enter_wifi_wait();
      break;
    }
    oled_net::loop();
    if (mqtt::connected()) {
      enter_mqtt_ok();
      break;
    }
    if (mqtt::take_connect_attempt() == mqtt::connect_attempt_t::fail) {
      enter_mqtt_fail();
    }
    break;

  case state_t::mqttOk:
    if (!sta_up()) {
      enter_wifi_wait();
      break;
    }
    oled_net::loop();
    if (oled_net::done()) {
      go_rest();
    }
    break;

  case state_t::mqttFail:
    if (!sta_up()) {
      enter_wifi_wait();
      break;
    }
    oled_net::loop();
    if (oled_net::done()) {
      enter_mqtt_wait();
    }
    break;

  case state_t::stopped:
    break;
  }
}

}  // namespace oled_ui
}  // namespace hb
