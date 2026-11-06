#include "mode.h"
#include "cp.h"
#include "hug.h"
#include "log.h"
#include "oled_ui.h"
#include "pref.h"
#include "wifi_vo.h"

#include <cstdint>

namespace hb {
namespace mode {
namespace {

enum class slot_t : uint8_t { hug = 0, portal = 1 };

slot_t current = slot_t::hug;

slot_t clamp_slot(uint8_t slot) {
  return slot == static_cast<uint8_t>(slot_t::portal) ? slot_t::portal
                                                      : slot_t::hug;
}

void enter(slot_t slot) {
  if (slot == slot_t::portal) {
    // Peer hugs meanwhile wait on AWS and show on resume. Do not leave
    // API requests running under the AP.
    hug::pause();
    oled_ui::stop();
    cp::enable();
    return;
  }
  cp::disable();
  oled_ui::resume();
}

}  // namespace

void setup() {
  current = clamp_slot(pref::get_mode());

  wifi_t wifi;
  if (current == slot_t::hug && !pref::get_wifi(wifi)) {
    set_slot(static_cast<uint8_t>(slot_t::portal));
    return;
  }

  enter(current);
}

void loop() {
  if (current == slot_t::hug) {
    hug::loop();
  } else {
    cp::loop();
  }
}

void set_slot(uint8_t slot) {
  const slot_t next = clamp_slot(slot);
  if (next == current) {
    return;
  }

  current = next;
  enter(current);

  if (!pref::set_mode(static_cast<uint8_t>(current))) {
    log::error("Mode persist failed");
  }
}

void toggle() {
  set_slot(static_cast<uint8_t>(
      current == slot_t::hug ? slot_t::portal : slot_t::hug));
}

void short_press() {
  if (current == slot_t::hug) {
    hug::send();
  }
}

}  // namespace mode
}  // namespace hb
