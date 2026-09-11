#ifndef HUG_BOT_OLED_NET_H
#define HUG_BOT_OLED_NET_H

namespace hb {
namespace oled_net {

enum class kind_t {
  wifi_wait,
  wifi_ok,
  wifi_fail,
  mqtt_wait,
  mqtt_ok,
  mqtt_fail
};

void start(kind_t kind);
void loop();
bool done();

}  // namespace oled_net
}  // namespace hb

#endif  // HUG_BOT_OLED_NET_H
