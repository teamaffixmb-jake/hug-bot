#include "pref.h"

#include <Arduino.h>
#include <Preferences.h>

namespace hb {
namespace pref {
namespace {

constexpr char kNamespace[] = "hb";
constexpr char kWifiSsidKey[] = "wifi_ssid";
constexpr char kWifiPskKey[] = "wifi_psk";
constexpr char kVolumeKey[] = "volume";
constexpr uint8_t kVolumeDefault = 80;

}  // namespace

bool GetWifi(wifi_t &out) {
  out.Assign(nullptr, nullptr);

  Preferences prefs;
  prefs.begin(kNamespace, true);
  const String ssid = prefs.getString(kWifiSsidKey, "");
  const String psk = prefs.getString(kWifiPskKey, "");
  prefs.end();

  if (ssid.isEmpty()) {
    return false;
  }
  out.Assign(ssid.c_str(), psk.c_str());
  return true;
}

void SetWifi(const wifi_t &wifi) {
  Preferences prefs;
  prefs.begin(kNamespace, false);
  prefs.putString(kWifiSsidKey, wifi.ssid);
  prefs.putString(kWifiPskKey, wifi.psk);
  prefs.end();
}

void ClearWifi() {
  Preferences prefs;
  prefs.begin(kNamespace, false);
  prefs.remove(kWifiSsidKey);
  prefs.remove(kWifiPskKey);
  prefs.end();
}

uint8_t GetVolume() {
  Preferences prefs;
  prefs.begin(kNamespace, true);
  const uint8_t volume = prefs.getUChar(kVolumeKey, kVolumeDefault);
  prefs.end();
  return volume > 100 ? 100 : volume;
}

void SetVolume(uint8_t volume) {
  if (volume > 100) {
    volume = 100;
  }
  Preferences prefs;
  prefs.begin(kNamespace, false);
  prefs.putUChar(kVolumeKey, volume);
  prefs.end();
}

}  // namespace pref
}  // namespace hb
