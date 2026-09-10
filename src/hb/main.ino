#include "main.h"
#include "cp.h"
#include "hug.h"
#include "input.h"
#include "pb.h"
#include "sta.h"

namespace hb {

void main_entry_setup() {
  sta::setup();
  cp::setup();
  hug::setup();
  pb::setup();
  input::setup();
  sta::connect_saved();
}

void main_entry_loop() {
  input::loop();
  cp::loop();
  sta::loop();
  hug::loop();
}

}  // namespace hb
