#ifndef HUG_BOT_INPUT_H
#define HUG_BOT_INPUT_H

namespace hb {
namespace input {

// Push-button (pb) + Serial. Gestures go through mode; this layer does
// not call cp or hug directly. Serial: 'e' slot 1, 'd' slot 0, 'h' short
// press. setup() is empty; pb::setup() owns the pin.
void setup();
void loop();

}  // namespace input
}  // namespace hb

#endif  // HUG_BOT_INPUT_H
