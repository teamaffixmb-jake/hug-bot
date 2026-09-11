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

}  // namespace oled_ui
}  // namespace hb

#endif  // HUG_BOT_OLED_UI_H
