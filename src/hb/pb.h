#ifndef HUG_BOT_PB_H
#define HUG_BOT_PB_H

namespace hb {
namespace pb {

// Push button on A3. Wired to GND; pin uses the internal pull-up.
// down() is true while the button is held.
void setup();
bool down();

}  // namespace pb
}  // namespace hb

#endif  // HUG_BOT_PB_H
