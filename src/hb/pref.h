#ifndef HUG_BOT_PREF_H
#define HUG_BOT_PREF_H

#include "wifi_vo.h"

#include <cstdint>

namespace hb {
namespace pref {

// Long-term storage inventory (NVS namespace "hb"):
//   wifi_ssid  station SSID  (wifi_t::ssid)
//   wifi_psk   station PSK   (wifi_t::psk, empty = open network)
//   volume     0..100
//   mode       0 = hug ops, 1 = captive portal (other values clamp to 0)
// Hug counts live on AWS, not here. Boards flashed with older firmware may
// still hold an unused "pending" key; nothing reads it.

// Returns false if no station SSID is stored.
bool get_wifi(wifi_t &out);

void set_wifi(const wifi_t &wifi);
void clear_wifi();

uint8_t get_volume();
void set_volume(uint8_t volume);

uint8_t get_mode();
// Returns false if the NVS write failed. No-op if already stored.
bool set_mode(uint8_t mode);

}  // namespace pref
}  // namespace hb

#endif  // HUG_BOT_PREF_H
