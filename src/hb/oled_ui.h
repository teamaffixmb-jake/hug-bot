#ifndef HUG_BOT_OLED_UI_H
#define HUG_BOT_OLED_UI_H

namespace hb {
namespace oled_ui {

void setup();
void loop();
void on_send();
// Starts the received-hug animation if the UI is at rest; false if busy
// (hug retries). hug::receive_shown() is called when it ends.
bool on_receive();
void stop();
void resume();

// True iff state is awsWait after the call (including already there).
bool on_aws_connecting();

}  // namespace oled_ui
}  // namespace hb

#endif  // HUG_BOT_OLED_UI_H
