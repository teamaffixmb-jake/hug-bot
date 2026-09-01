#ifndef HUG_BOT_WIFI_VO_H
#define HUG_BOT_WIFI_VO_H

#include <cstddef>
#include <cstring>

namespace hb {

struct wifi_t {
  static constexpr size_t k_ssid_max = 32;
  static constexpr size_t k_psk_max = 63;

  char ssid[k_ssid_max + 1];
  char psk[k_psk_max + 1];

  void assign(const char *ssid_src, const char *psk_src) {
    ssid[0] = '\0';
    psk[0] = '\0';
    if (ssid_src != nullptr) {
      strncpy(ssid, ssid_src, k_ssid_max);
      ssid[k_ssid_max] = '\0';
    }
    if (psk_src != nullptr) {
      strncpy(psk, psk_src, k_psk_max);
      psk[k_psk_max] = '\0';
    }
  }
};

}  // namespace hb

#endif  // HUG_BOT_WIFI_VO_H
