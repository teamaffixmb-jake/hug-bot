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
bool scan_started = false;
uint32_t retry_at = 0;
uint32_t scan_retry_at = 0;

void run_pump() {
  if (pump != nullptr) {
    pump();
  }
}

void apply_sta_mode() {
  if (WiFi.getMode() & WIFI_AP) {
    WiFi.mode(WIFI_AP_STA);
  } else {
    WiFi.mode(WIFI_STA);
  }
}

void abort_scan() {
  WiFi.scanDelete();
  scan_started = false;
}

bool connect_blocking(const char *ssid, const char *psk) {
  abort_scan();
  WiFi.setAutoReconnect(false);
  apply_sta_mode();
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
  if (scan_running()) {
    return;
  }
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
  apply_sta_mode();
  WiFi.begin(wifi.ssid, wifi.psk);
}

void start_scan() {
  if (scan_running()) {
    return;
  }
  if (millis() < scan_retry_at) {
    return;
  }
  WiFi.scanDelete();
  // Do not call WiFi.mode() here. Switching AP→AP_STA and scanning
  // immediately makes esp_wifi_scan_start fail, which used to show up as
  // "No networks found".
  const int16_t rc = WiFi.scanNetworks(true);
  scan_started = (rc == WIFI_SCAN_RUNNING);
  if (!scan_started) {
    scan_retry_at = millis() + 250;
  }
}

bool scan_running() {
  if (!scan_started) {
    return false;
  }
  const int16_t n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) {
    return true;
  }
  if (n < 0) {
    scan_started = false;
  }
  return false;
}

bool scan_ready() {
  return scan_started && WiFi.scanComplete() >= 0;
}

size_t take_scan(scan_ap_t *out, size_t cap) {
  if (scan_running()) {
    return 0;
  }
  scan_started = false;
  if (out == nullptr || cap == 0) {
    WiFi.scanDelete();
    return 0;
  }

  const int n = WiFi.scanComplete();
  if (n < 0) {
    WiFi.scanDelete();
    return 0;
  }

  size_t count = 0;
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
