#include "main.h"
#include "input.h"
#include "mode.h"
#include "pb.h"
#include "sta.h"

namespace hb {

void main_entry_setup() {
  sta::setup();
  cp::setup();
  hug::setup();
  pb::setup();
  input::setup();
  mode::setup();
  sta::connect_saved();
}

void main_entry_loop() {
  input::loop();
  sta::loop();
  mode::loop();
}

}  // namespace hb
