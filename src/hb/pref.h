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

// Returns false if no station SSID is stored.
bool get_wifi(wifi_t &out);

void set_wifi(const wifi_t &wifi);
void clear_wifi();

uint8_t get_volume();
void set_volume(uint8_t volume);

}  // namespace pref
}  // namespace hb

#endif  // HUG_BOT_PREF_H
