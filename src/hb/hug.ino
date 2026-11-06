#include "hug.h"
#include "api.h"
#include "log.h"
#include "oled.h"
#include "oled_ui.h"

#include <Arduino.h>
#include <cstdint>

// All hug state lives on AWS (infra/hug-bot.yaml); this module only turns
// what the API last said into animations. Nothing is stored on the board,
// so there is no second copy to drift.

namespace hb {
namespace hug {
namespace {

constexpr int32_t k_display_cap = 999;

bool showing = false;
uint32_t showing_through = 0;
// Received count already animated this boot. AWS's seen count lags until our
// mark_seen lands, so this stops the animation replaying meanwhile.
uint32_t shown_through = 0;
bool was_online = false;

void deliver_received(const api::view_t &v) {
  if (!v.valid || showing) {
    return;
  }
  const uint32_t done = v.seen > shown_through ? v.seen : shown_through;
  if (v.received <= done) {
    return;
  }
  if (!oled::ready()) {
    shown_through = v.received;
    api::mark_seen(v.received);
    return;
  }
  if (!oled_ui::on_receive()) {
    return;
  }
  showing = true;
  showing_through = v.received;
  log::warn("Hug received");
}

}  // namespace

void setup() {
  if (api::k_host[0] == '\0' || api::k_token[0] == '\0') {
    log::error("API secrets.h missing");
  }
  api::setup();
}

void loop() {
  api::start();
  const bool up = api::online();
  if (up != was_online) {
    was_online = up;
    log::warn(up ? "AWS online" : "AWS offline");
  }
  if (!up) {
    oled_ui::on_aws_connecting();
    return;
  }
  deliver_received(api::view());
}

void pause() {
  api::stop();
  // An interrupted animation was never marked seen, so it replays on resume.
  showing = false;
}

void send() {
  if (!api::send_hug()) {
    log::warn("Hug not sent (offline)");
    return;
  }
  log::warn("Hug sent");
  oled_ui::on_send();
}

bool send_settled() { return api::hugs_in_flight() == 0; }

void receive_shown() {
  if (!showing) {
    return;
  }
  showing = false;
  shown_through = showing_through;
  api::mark_seen(showing_through);
}

int32_t pending_for_display() {
  const api::view_t v = api::view();
  if (!v.valid) {
    return 0;
  }
  if (v.pending > k_display_cap) {
    return k_display_cap;
  }
  if (v.pending < -k_display_cap) {
    return -k_display_cap;
  }
  return v.pending;
}

}  // namespace hug
}  // namespace hb
