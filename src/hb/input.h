#ifndef HUG_BOT_INPUT_H
#define HUG_BOT_INPUT_H

namespace hb {
namespace input {

// Physical triggers that call cp::enable / cp::disable live here once
// hardware exists. setup/loop are the hooks; they do not start the portal.
void setup();
void loop();

}  // namespace input
}  // namespace hb

#endif  // HUG_BOT_INPUT_H
