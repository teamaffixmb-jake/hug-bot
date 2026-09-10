#ifndef HUG_BOT_INPUT_H
#define HUG_BOT_INPUT_H

namespace hb {
namespace input {

// GPIO is not chosen yet. When it is, this layer should call hug::send()
// and cp::enable() / cp::disable() / cp::toggle(). setup/loop do not
// start the portal or send a hug on their own.
void setup();
void loop();

}  // namespace input
}  // namespace hb

#endif  // HUG_BOT_INPUT_H
