#include "cp.h"
#include "pref.h"

#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>

#include <cstring>

namespace hb {
namespace cp {
namespace {

constexpr char kApSsid[] = "Hug-Bot";
// Setup-AP PSK. Same role as the WPA2 password printed on a router sticker:
// the 4-way handshake encrypts the air before the portal POST runs.
constexpr char kApPsk[] = "hugbot-setup";
constexpr uint16_t kDnsPort = 53;
constexpr uint16_t kHttpPort = 80;
constexpr uint32_t kConnectTimeoutMs = 15000;
constexpr uint32_t kStaRetryMs = 15000;
constexpr size_t kMaxNetworks = 16;
constexpr size_t kSsidMaxLen = hb::wifi_t::kSsidMax;

constexpr const char *kProbePaths[] = {
    "/",
    "/generate_204",
    "/gen_204",
    "/hotspot-detect.html",
    "/connecttest.txt",
    "/ncsi.txt",
    "/fwlink",
    "/canonical.html",
    "/success.txt",
};

struct ScannedNetwork {
  char ssid[kSsidMaxLen + 1];
  int32_t rssi;
  bool open;
};

ScannedNetwork networks[kMaxNetworks];
size_t network_count = 0;
const char *page_status = nullptr;
bool routes_bound = false;
bool sta_wanted = false;
bool sta_connecting = false;
uint32_t sta_retry_at = 0;

DNSServer dns_server;
WebServer server(kHttpPort);

void AppendHtmlEscaped(String &out, const char *s) {
  for (; *s != '\0'; ++s) {
    switch (*s) {
      case '&':
        out += F("&amp;");
        break;
      case '<':
        out += F("&lt;");
        break;
      case '>':
        out += F("&gt;");
        break;
      case '"':
        out += F("&quot;");
        break;
      case '\'':
        out += F("&#39;");
        break;
      default:
        out += *s;
        break;
    }
  }
}

void SendNoCacheHeaders() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
}

void ScanNetworks() {
  network_count = 0;
  const int n = WiFi.scanNetworks();
  for (int i = 0; i < n && network_count < kMaxNetworks; ++i) {
    const String ssid = WiFi.SSID(i);
    if (ssid.isEmpty() || ssid.length() > kSsidMaxLen) {
      continue;
    }

    bool duplicate = false;
    for (size_t j = 0; j < network_count; ++j) {
      if (ssid == networks[j].ssid) {
        duplicate = true;
        break;
      }
    }
    if (duplicate) {
      continue;
    }

    ScannedNetwork &slot = networks[network_count];
    strncpy(slot.ssid, ssid.c_str(), kSsidMaxLen);
    slot.ssid[kSsidMaxLen] = '\0';
    slot.rssi = WiFi.RSSI(i);
    slot.open = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
    ++network_count;
  }
  WiFi.scanDelete();
}

void SendPortalPage() {
  String html;
  html.reserve(4096);
  html += F(
      "<!DOCTYPE html><html lang=\"en\"><head>"
      "<meta charset=\"UTF-8\">"
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  if (sta_connecting) {
    html += F(
        "<meta http-equiv=\"refresh\" content=\"1;url=/\">"
        "<script>setTimeout(function(){location.replace('/')},1000)</script>");
  }
  html += F(
      "<title>Hug-Bot</title>"
      "<style>"
      ":root { color-scheme: light; }"
      "body { margin: 0; min-height: 100vh; display: flex; align-items: center;"
      "justify-content: center; font-family: system-ui, sans-serif;"
      "background: #fff5f7; color: #3b1f2b; }"
      "main { width: min(22rem, calc(100% - 2rem)); padding: 1.5rem; }"
      "h1 { font-size: 1.75rem; margin: 0 0 0.25rem; text-align: center; }"
      "p, label { font-size: 0.95rem; }"
      ".lede { text-align: center; opacity: 0.8; margin: 0 0 1.25rem; }"
      ".status { background: #ffe3ea; border-radius: 0.5rem; padding: 0.6rem"
      " 0.75rem; margin: 0 0 1rem; }"
      ".ok { background: #e3f4ea; }"
      "form { display: grid; gap: 0.75rem; }"
      "label { display: grid; gap: 0.3rem; font-weight: 600; }"
      "select, input { font: inherit; padding: 0.55rem 0.6rem; border: 1px"
      " solid #e3b7c4; border-radius: 0.5rem; background: #fff; }"
      "button { font: inherit; font-weight: 650; padding: 0.7rem; border: 0;"
      " border-radius: 0.5rem; background: #3b1f2b; color: #fff; }"
      "button.secondary { background: #fff; color: #3b1f2b; border: 1px solid"
      " #3b1f2b; }"
      "a { color: inherit; }"
      ".refresh { text-align: center; margin: 1rem 0 0; }"
      "hr { border: 0; border-top: 1px solid #e3b7c4; margin: 1.25rem 0; }"
      "</style></head><body><main><h1>Hug-Bot</h1>");

  if (sta_connecting) {
    html += F("<p class=\"lede\">Connecting. This page will update when "
              "done.</p><p class=\"refresh\"><a href=\"/\">Continue</a></p>"
              "</main></body></html>");
    SendNoCacheHeaders();
    server.send(200, "text/html", html);
    return;
  }

  html += F(
      "<p class=\"lede\">Control Hug-Bot from this setup network.</p>");

  if (page_status != nullptr && page_status[0] != '\0') {
    html += F("<p class=\"status\">");
    AppendHtmlEscaped(html, page_status);
    html += F("</p>");
  }

  if (WiFi.status() == WL_CONNECTED) {
    html += F("<p class=\"status ok\">Connected to ");
    AppendHtmlEscaped(html, WiFi.SSID().c_str());
    html += F("<br>LAN IP: ");
    AppendHtmlEscaped(html, WiFi.localIP().toString().c_str());
    html += F("</p>"
              "<form method=\"POST\" action=\"/disconnect\">"
              "<button type=\"submit\" class=\"secondary\">Disconnect "
              "Wi-Fi</button></form><hr>");
  }

  html += F(
      "<form method=\"POST\" action=\"/connect\" autocomplete=\"off\">"
      "<label>Network<select name=\"ssid\">");

  if (network_count == 0) {
    html += F("<option value=\"\">No networks found</option>");
  }
  for (size_t i = 0; i < network_count; ++i) {
    html += F("<option value=\"");
    AppendHtmlEscaped(html, networks[i].ssid);
    html += F("\">");
    AppendHtmlEscaped(html, networks[i].ssid);
    html += networks[i].open ? F(" (open)") : F("");
    html += F("</option>");
  }

  html += F(
      "</select></label>"
      "<label>Hidden / other SSID<input type=\"text\" name=\"ssid_custom\""
      " maxlength=\"32\" spellcheck=\"false\"></label>"
      "<label>Password<input type=\"text\" name=\"psk\" maxlength=\"63\""
      " spellcheck=\"false\" autocomplete=\"off\"></label>"
      "<button type=\"submit\">Connect</button>"
      "</form>"
      "<p class=\"refresh\"><a href=\"/refresh\">Refresh network list</a></p>"
      "</main></body></html>");

  SendNoCacheHeaders();
  server.send(200, "text/html", html);
}

bool ConnectSta(const char *ssid, const char *psk) {
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, psk);

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    dns_server.processNextRequest();
    server.handleClient();
    if (!sta_wanted) {
      WiFi.disconnect(false);
      return false;
    }
    if (millis() - start >= kConnectTimeoutMs) {
      WiFi.disconnect(false);
      return false;
    }
    delay(10);
  }
  return true;
}

void BindRoutes();

void StartControlAp() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(kApSsid, kApPsk);

  const IPAddress ap_ip = WiFi.softAPIP();
  dns_server.start(kDnsPort, "*", ap_ip);
  BindRoutes();
  server.begin();

  Serial.print("Setup SSID: ");
  Serial.println(kApSsid);
  Serial.print("Setup password: ");
  Serial.println(kApPsk);
  Serial.print("Portal URL: http://");
  Serial.println(ap_ip);
}

void HandleConnect() {
  String ssid = server.arg("ssid_custom");
  ssid.trim();
  if (ssid.isEmpty()) {
    ssid = server.arg("ssid");
    ssid.trim();
  }
  const String psk = server.arg("psk");

  if (ssid.isEmpty() || ssid.length() > kSsidMaxLen) {
    page_status = "Select or enter a network name.";
    SendPortalPage();
    return;
  }
  if (psk.length() > hb::wifi_t::kPskMax ||
      (psk.length() > 0 && psk.length() < 8)) {
    page_status = "Password must be at least 8 characters, or blank if the "
                  "network is open.";
    SendPortalPage();
    return;
  }

  Serial.print("Connecting to ");
  Serial.println(ssid);

  sta_wanted = true;
  sta_connecting = true;
  SendPortalPage();

  if (!ConnectSta(ssid.c_str(), psk.c_str())) {
    sta_connecting = false;
    if (sta_wanted) {
      page_status = "Could not connect. Check the password and try again.";
    }
    return;
  }

  hb::wifi_t wifi;
  wifi.Assign(ssid.c_str(), psk.c_str());
  hb::pref::SetWifi(wifi);
  page_status = "Connected.";
  sta_connecting = false;
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
}

void HandleDisconnect() {
  sta_wanted = false;
  sta_connecting = false;
  hb::pref::ClearWifi();
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false);
  page_status = "Disconnected from Wi-Fi.";
  Serial.println("STA disconnected; Hug-Bot AP still up.");
  SendPortalPage();
}

void HandleRefresh() {
  ScanNetworks();
  page_status = nullptr;
  server.sendHeader("Location", "/", true);
  SendNoCacheHeaders();
  server.send(302, "text/plain", "");
}

void BindRoutes() {
  if (routes_bound) {
    return;
  }

  for (const char *path : kProbePaths) {
    server.on(path, SendPortalPage);
  }
  server.on("/connect", HTTP_POST, HandleConnect);
  server.on("/disconnect", HTTP_POST, HandleDisconnect);
  server.on("/refresh", HandleRefresh);
  server.onNotFound(SendPortalPage);
  routes_bound = true;
}

bool ConnectSaved() {
  hb::wifi_t wifi;
  if (!hb::pref::GetWifi(wifi)) {
    return false;
  }

  Serial.print("Connecting to saved network ");
  Serial.println(wifi.ssid);
  sta_wanted = true;
  if (!ConnectSta(wifi.ssid, wifi.psk)) {
    Serial.println("Saved network failed.");
    return false;
  }

  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

}  // namespace

void setup() {
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false, true);

  StartControlAp();
  ScanNetworks();
  ConnectSaved();
}

void loop() {
  dns_server.processNextRequest();
  server.handleClient();

  if (!sta_wanted || WiFi.status() == WL_CONNECTED ||
      millis() < sta_retry_at) {
    return;
  }
  sta_retry_at = millis() + kStaRetryMs;

  hb::wifi_t wifi;
  if (!hb::pref::GetWifi(wifi)) {
    sta_wanted = false;
    return;
  }

  Serial.print("Retrying ");
  Serial.println(wifi.ssid);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(wifi.ssid, wifi.psk);
}

}  // namespace cp
}  // namespace hb
