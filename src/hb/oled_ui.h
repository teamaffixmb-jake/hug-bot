#ifndef HUG_BOT_OLED_UI_H
#define HUG_BOT_OLED_UI_H

namespace hb {
namespace oled_ui {

void setup();
void loop();
void on_send();
void on_receive();
void stop();
void resume();

// True iff state is mqttWait after the call (including already there).
bool on_mqtt_connecting();

}  // namespace oled_ui
}  // namespace hb

#endif  // HUG_BOT_OLED_UI_H
