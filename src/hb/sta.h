#ifndef HUG_BOT_STA_H
#define HUG_BOT_STA_H

#include "wifi_vo.h"

#include <cstddef>
#include <cstdint>

namespace hb {
namespace sta {

using pump_fn_t = void (*)();

struct scan_ap_t {
  char ssid[wifi_t::k_ssid_max + 1];
  int32_t rssi;
  bool open;
};

void setup();
void loop();
void set_pump(pump_fn_t pump);

bool connect(const char *ssid, const char *psk);
bool connect_saved();
void disconnect();

bool wanted();

size_t scan(scan_ap_t *out, size_t cap);

}  // namespace sta
}  // namespace hb

#endif  // HUG_BOT_STA_H
