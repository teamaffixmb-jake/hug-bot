#include "sta.h"
#include "log.h"
#include "pref.h"

#include <Arduino.h>
#include <WiFi.h>

#include <cstring>

namespace hb {
namespace sta {
namespace {

constexpr uint32_t kConnectTimeoutMs = 15000;
constexpr uint32_t kRetryMs = 15000;

PumpFn pump = nullptr;
bool sta_wanted = false;
uint32_t retry_at = 0;

void Pump() {
  if (pump != nullptr) {
    pump();
  }
}

bool ConnectBlocking(const char *ssid, const char *psk) {
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, psk);

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Pump();
    if (!sta_wanted) {
      WiFi.disconnect(false);
      return false;
    }
    if (millis() - start >= kConnectTimeoutMs) {
      WiFi.disconnect(false);
      log::Error("Wi-Fi join timed out");
      return false;
    }
    delay(10);
  }
  return true;
}

}  // namespace

void SetPump(PumpFn fn) { pump = fn; }

bool wanted() { return sta_wanted; }

void setup() {
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false, true);
}

bool Connect(const char *ssid, const char *psk) {
  sta_wanted = true;
  if (!ConnectBlocking(ssid, psk)) {
    return false;
  }
  wifi_t wifi;
  wifi.Assign(ssid, psk);
  pref::SetWifi(wifi);
  return true;
}

bool ConnectSaved() {
  wifi_t wifi;
  if (!pref::GetWifi(wifi)) {
    return false;
  }

  Serial.print("Connecting to saved network ");
  Serial.println(wifi.ssid);
  if (!Connect(wifi.ssid, wifi.psk)) {
    log::Error("Saved Wi-Fi join failed");
    Serial.println("Saved network failed.");
    return false;
  }

  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

void Disconnect() {
  sta_wanted = false;
  pref::ClearWifi();
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false);
  log::Warn("Disconnected from Wi-Fi");
}

void loop() {
  if (!sta_wanted || WiFi.status() == WL_CONNECTED || millis() < retry_at) {
    return;
  }
  retry_at = millis() + kRetryMs;

  wifi_t wifi;
  if (!pref::GetWifi(wifi)) {
    sta_wanted = false;
    return;
  }

  Serial.print("Retrying ");
  Serial.println(wifi.ssid);
  log::Warn("Retrying saved Wi-Fi");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(wifi.ssid, wifi.psk);
}

size_t Scan(ScanAp *out, size_t cap) {
  if (out == nullptr || cap == 0) {
    return 0;
  }

  size_t count = 0;
  const int n = WiFi.scanNetworks();
  for (int i = 0; i < n && count < cap; ++i) {
    const String ssid = WiFi.SSID(i);
    if (ssid.isEmpty() || ssid.length() > wifi_t::kSsidMax) {
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

    ScanAp &slot = out[count];
    strncpy(slot.ssid, ssid.c_str(), wifi_t::kSsidMax);
    slot.ssid[wifi_t::kSsidMax] = '\0';
    slot.rssi = WiFi.RSSI(i);
    slot.open = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
    ++count;
  }
  WiFi.scanDelete();
  return count;
}

}  // namespace sta
}  // namespace hb
