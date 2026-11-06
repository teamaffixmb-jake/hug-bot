#include "pref.h"

#include <Arduino.h>
#include <Preferences.h>

namespace hb {
namespace pref {
namespace {

constexpr char k_namespace[] = "hb";
constexpr char k_wifi_ssid_key[] = "wifi_ssid";
constexpr char k_wifi_psk_key[] = "wifi_psk";
constexpr char k_volume_key[] = "volume";
constexpr char k_mode_key[] = "mode";
constexpr uint8_t k_volume_default = 80;
constexpr uint8_t k_mode_default = 0;
constexpr uint8_t k_mode_max = 1;

uint8_t clamp_mode(uint8_t mode) {
  return mode > k_mode_max ? k_mode_default : mode;
}

}  // namespace

bool get_wifi(wifi_t &out) {
  out.assign(nullptr, nullptr);

  Preferences prefs;
  prefs.begin(k_namespace, true);
  const String ssid = prefs.getString(k_wifi_ssid_key, "");
  const String psk = prefs.getString(k_wifi_psk_key, "");
  prefs.end();

  if (ssid.isEmpty()) {
    return false;
  }
  out.assign(ssid.c_str(), psk.c_str());
  return true;
}

void set_wifi(const wifi_t &wifi) {
  Preferences prefs;
  prefs.begin(k_namespace, false);
  prefs.putString(k_wifi_ssid_key, wifi.ssid);
  prefs.putString(k_wifi_psk_key, wifi.psk);
  prefs.end();
}

void clear_wifi() {
  Preferences prefs;
  prefs.begin(k_namespace, false);
  prefs.remove(k_wifi_ssid_key);
  prefs.remove(k_wifi_psk_key);
  prefs.end();
}

uint8_t get_volume() {
  Preferences prefs;
  prefs.begin(k_namespace, true);
  const uint8_t volume = prefs.getUChar(k_volume_key, k_volume_default);
  prefs.end();
  return volume > 100 ? 100 : volume;
}

void set_volume(uint8_t volume) {
  if (volume > 100) {
    volume = 100;
  }
  Preferences prefs;
  prefs.begin(k_namespace, false);
  prefs.putUChar(k_volume_key, volume);
  prefs.end();
}

uint8_t get_mode() {
  Preferences prefs;
  prefs.begin(k_namespace, true);
  const uint8_t mode = prefs.getUChar(k_mode_key, k_mode_default);
  prefs.end();
  return clamp_mode(mode);
}

bool set_mode(uint8_t mode) {
  mode = clamp_mode(mode);
  if (get_mode() == mode) {
    return true;
  }
  Preferences prefs;
  prefs.begin(k_namespace, false);
  const size_t written = prefs.putUChar(k_mode_key, mode);
  prefs.end();
  return written > 0;
}

}  // namespace pref
}  // namespace hb
