#ifndef HUG_BOT_CP_H
#define HUG_BOT_CP_H

namespace hb {
namespace cp {

void setup();
void loop();

// Hardware (button, etc.) is not chosen yet. Call these from that layer
// when it exists. enable() starts Hug-Bot AP + portal; disable() tears
// them down so the radio can idle or sleep. STA (LAN) is left as-is.
void enable();
void disable();
bool enabled();

}  // namespace cp
}  // namespace hb

#endif  // HUG_BOT_CP_H
