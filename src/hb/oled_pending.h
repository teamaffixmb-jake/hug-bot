#ifndef HUG_BOT_OLED_PENDING_H
#define HUG_BOT_OLED_PENDING_H

#include <cstdint>

namespace hb {
namespace oled_pending {

// "N pending response" centered, slow slight pulse.
void setup();
void loop();
void set_count(uint32_t n);

}  // namespace oled_pending
}  // namespace hb

#endif  // HUG_BOT_OLED_PENDING_H
