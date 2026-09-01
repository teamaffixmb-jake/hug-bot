#include "input.h"
#include "cp.h"

namespace hb {
namespace input {

void setup() {}

void loop() {
  // Placeholder until a button (or other trigger) is wired.
  // Serial: 'e' enable portal, 'd' disable portal.
  if (!Serial.available()) {
    return;
  }
  const char cmd = static_cast<char>(Serial.read());
  if (cmd == 'e') {
    cp::enable();
  } else if (cmd == 'd') {
    cp::disable();
  }
}

}  // namespace input
}  // namespace hb
