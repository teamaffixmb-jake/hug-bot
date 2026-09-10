#ifndef HUG_BOT_MODE_H
#define HUG_BOT_MODE_H

#include <cstdint>

namespace hb {
namespace mode {

void setup();
void loop();

// Slot 0 = hug ops, slot 1 = captive portal. No-op if already there
// (does not write NVS). Other values clamp to 0.
void set_slot(uint8_t slot);
void toggle();
void short_press();

}  // namespace mode
}  // namespace hb

#endif  // HUG_BOT_MODE_H
