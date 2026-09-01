#ifndef HUG_BOT_PREF_H
#define HUG_BOT_PREF_H

#include "wifi_vo.h"

namespace hb {
namespace pref {

// Long-term storage inventory (NVS namespace "hb"):
//   wifi_ssid  station SSID  (wifi_t::ssid)
//   wifi_psk   station PSK   (wifi_t::psk, empty = open network)

// Returns false if no station SSID is stored.
bool GetWifi(wifi_t &out);

void SetWifi(const wifi_t &wifi);
void ClearWifi();

}  // namespace pref
}  // namespace hb

#endif  // HUG_BOT_PREF_H
