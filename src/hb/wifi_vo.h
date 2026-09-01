#ifndef HUG_BOT_WIFI_VO_H
#define HUG_BOT_WIFI_VO_H

#include <cstddef>
#include <cstring>

namespace hb {

struct wifi_t {
  static constexpr size_t kSsidMax = 32;
  static constexpr size_t kPskMax = 63;

  char ssid[kSsidMax + 1];
  char psk[kPskMax + 1];

  void Assign(const char *ssid_src, const char *psk_src) {
    ssid[0] = '\0';
    psk[0] = '\0';
    if (ssid_src != nullptr) {
      strncpy(ssid, ssid_src, kSsidMax);
      ssid[kSsidMax] = '\0';
    }
    if (psk_src != nullptr) {
      strncpy(psk, psk_src, kPskMax);
      psk[kPskMax] = '\0';
    }
  }
};

}  // namespace hb

#endif  // HUG_BOT_WIFI_VO_H
