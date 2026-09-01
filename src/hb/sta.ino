#include "sta.h"
#include "log.h"
#include "pref.h"

#include <Arduino.h>
#include <WiFi.h>

#include <cstring>

namespace hb {
namespace sta {
namespace {

constexpr uint32_t k_connect_timeout_ms = 15000;
constexpr uint32_t k_retry_ms = 15000;

pump_fn_t pump = nullptr;
bool sta_wanted = false;
uint32_t retry_at = 0;

void run_pump() {
  if (pump != nullptr) {
    pump();
  }
}

bool connect_blocking(const char *ssid, const char *psk) {
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, psk);

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    run_pump();
    if (!sta_wanted) {
      WiFi.disconnect(false);
      return false;
    }
    if (millis() - start >= k_connect_timeout_ms) {
      WiFi.disconnect(false);
      log::error("Wi-Fi join timed out");
      return false;
    }
    delay(10);
  }
  return true;
}

}  // namespace

void set_pump(pump_fn_t fn) { pump = fn; }

bool wanted() { return sta_wanted; }

void setup() {
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false, true);
}

bool connect(const char *ssid, const char *psk) {
  sta_wanted = true;
  if (!connect_blocking(ssid, psk)) {
    return false;
  }
  wifi_t wifi;
  wifi.assign(ssid, psk);
  pref::set_wifi(wifi);
  return true;
}

bool connect_saved() {
  wifi_t wifi;
  if (!pref::get_wifi(wifi)) {
    return false;
  }

  Serial.print("Connecting to saved network ");
  Serial.println(wifi.ssid);
  if (!connect(wifi.ssid, wifi.psk)) {
    log::error("Saved Wi-Fi join failed");
    Serial.println("Saved network failed.");
    return false;
  }

  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

void disconnect() {
  sta_wanted = false;
  pref::clear_wifi();
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false);
  log::warn("Disconnected from Wi-Fi");
}

void loop() {
  if (!sta_wanted || WiFi.status() == WL_CONNECTED || millis() < retry_at) {
    return;
  }
  retry_at = millis() + k_retry_ms;

  wifi_t wifi;
  if (!pref::get_wifi(wifi)) {
    sta_wanted = false;
    return;
  }

  Serial.print("Retrying ");
  Serial.println(wifi.ssid);
  log::warn("Retrying saved Wi-Fi");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(wifi.ssid, wifi.psk);
}

size_t scan(scan_ap_t *out, size_t cap) {
  if (out == nullptr || cap == 0) {
    return 0;
  }

  size_t count = 0;
  const int n = WiFi.scanNetworks();
  for (int i = 0; i < n && count < cap; ++i) {
    const String ssid = WiFi.SSID(i);
    if (ssid.isEmpty() || ssid.length() > wifi_t::k_ssid_max) {
      continue;
    }

    bool duplicate = false;
    for (size_t j = 0; j < count; ++j) {
      if (ssid == out[j].ssid) {
        duplicate = true;
        break;
      }
    }
    if (duplicate) {
      continue;
    }

    scan_ap_t &slot = out[count];
    strncpy(slot.ssid, ssid.c_str(), wifi_t::k_ssid_max);
    slot.ssid[wifi_t::k_ssid_max] = '\0';
    slot.rssi = WiFi.RSSI(i);
    slot.open = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
    ++count;
  }
  WiFi.scanDelete();
  return count;
}

}  // namespace sta
}  // namespace hb
