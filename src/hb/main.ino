#include "main.h"
#include "cp.h"
#include "hug.h"
#include "input.h"
#include "log.h"
#include "mode.h"
#include "oled.h"
#include "oled_ui.h"
#include "pb.h"
#include "sta.h"

namespace hb {

void main_entry_setup() {
  sta::setup();
  cp::setup();
  hug::setup();
  pb::setup();
  input::setup();
  oled::setup();
  oled_ui::setup();
  mode::setup();
  sta::connect_saved();
  log::warn(oled::ready() ? "OLED ready" : "OLED not ready");
}

void main_entry_loop() {
  input::loop();
  sta::loop();
  mode::loop();
  oled_ui::loop();
}

}  // namespace hb
