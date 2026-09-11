#ifndef HUG_BOT_OLED_MISSED_H
#define HUG_BOT_OLED_MISSED_H

#include <cstdint>

namespace hb {
namespace oled_missed {

// "N missed hugs" centered, slow slight pulse.
void setup();
void loop();
void set_count(uint32_t n);

}  // namespace oled_missed
}  // namespace hb

#endif  // HUG_BOT_OLED_MISSED_H
