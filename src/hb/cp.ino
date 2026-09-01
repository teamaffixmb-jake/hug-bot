#include "cp.h"
#include "log.h"
#include "pref.h"
#include "sta.h"
#include "wifi_vo.h"

#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>

namespace hb {
namespace cp {
namespace {

constexpr char kApSsid[] = "Hug-Bot";
// Setup-AP PSK. Same role as the WPA2 password printed on a router sticker:
// the 4-way handshake encrypts the air before the portal POST runs.
constexpr char kApPsk[] = "hugbot-setup";
constexpr uint16_t kDnsPort = 53;
constexpr uint16_t kHttpPort = 80;
constexpr size_t kMaxNetworks = 16;

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

sta::ScanAp networks[kMaxNetworks];
size_t network_count = 0;
const char *page_status = nullptr;
bool routes_bound = false;
bool sta_connecting = false;

DNSServer dns_server;
WebServer server(kHttpPort);

void Pump() {
  dns_server.processNextRequest();
  server.handleClient();
}

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
  network_count = sta::Scan(networks, kMaxNetworks);
}

enum class Tab { Home, Wifi, Volume, Logs, About };

Tab ParseTab() {
  const String uri = server.uri();
  if (uri == "/connect" || uri == "/refresh") {
    return Tab::Wifi;
  }
  if (uri == "/volume") {
    return Tab::Volume;
  }

  if (!server.hasArg("tab")) {
    return Tab::Home;
  }
  const String tab = server.arg("tab");
  if (tab == "wifi") {
    return Tab::Wifi;
  }
  if (tab == "volume") {
    return Tab::Volume;
  }
  if (tab == "logs") {
    return Tab::Logs;
  }
  if (tab == "about") {
    return Tab::About;
  }
  return Tab::Home;
}

void AppendMenu(String &html) {
  html += F(
      "<nav class=\"menu\">"
      "<a href=\"/?tab=wifi\">Wi-Fi</a>"
      "<a href=\"/?tab=volume\">Volume</a>"
      "<a href=\"/?tab=logs\">Logs</a>"
      "<a href=\"/?tab=about\">About</a>"
      "</nav>");
}

void AppendBack(String &html) {
  html += F("<p class=\"back\"><a href=\"/\">Back</a></p>");
}

void AppendWifiPanel(String &html) {
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
              "Wi-Fi</button></form>");
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
      "<p class=\"refresh\"><a href=\"/refresh\">Refresh network list</a></p>");
}

void AppendVolumePanel(String &html) {
  const uint8_t volume = pref::GetVolume();
  char value[4];
  snprintf(value, sizeof(value), "%u", volume);

  html += F("<p class=\"lede\">Speaker level. Not wired to hardware yet.</p>"
            "<form method=\"POST\" action=\"/volume\">"
            "<label>Volume<input type=\"range\" name=\"volume\" min=\"0\""
            " max=\"100\" value=\"");
  html += value;
  html += F("\" oninput=\"document.getElementById('vp').textContent="
            "this.value+'%'\"></label>"
            "<p class=\"volpct\"><span id=\"vp\">");
  html += value;
  html += F("%</span></p>"
            "<button type=\"submit\">Save</button></form>");
}

void AppendLogsPanel(String &html) {
  if (log::Size() == 0) {
    html += F("<p class=\"lede\">No warnings or errors yet.</p>");
    return;
  }

  html += F("<pre class=\"log\">");
  char line[log::kLineMax];
  log::Level level;
  for (size_t i = 0; i < log::Size(); ++i) {
    if (!log::Get(i, level, line, sizeof(line))) {
      continue;
    }
    html += (level == log::Level::Error) ? F("E  ") : F("W  ");
    AppendHtmlEscaped(html, line);
    html += '\n';
  }
  html += F("</pre>");
}

void AppendAboutPanel(String &html) {
  html += F("<p class=\"lede dedication\">Made with love for Leon Masuda</p>");
}

void SendPortalPage() {
  String html;
  html.reserve(8192);
  html += F(
      "<!DOCTYPE html><html lang=\"en\"><head>"
      "<meta charset=\"UTF-8\">"
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  if (sta_connecting) {
    html += F(
        "<meta http-equiv=\"refresh\" content=\"1;url=/?tab=wifi\">"
        "<script>setTimeout(function(){location.replace('/?tab=wifi')},1000)"
        "</script>");
  } else if (ParseTab() == Tab::Logs) {
    html += F(
        "<meta http-equiv=\"refresh\" content=\"2;url=/?tab=logs\">");
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
      ".menu { display: grid; gap: 0.65rem; margin-top: 1.25rem; }"
      ".menu a { display: block; text-align: center; padding: 0.85rem;"
      " border-radius: 0.5rem; text-decoration: none; background: #3b1f2b;"
      " color: #fff; font-weight: 650; }"
      ".back { margin: 0 0 1rem; }"
      ".back a { font-weight: 650; text-decoration: none; }"
      ".volpct { text-align: center; font-size: 1.35rem; font-weight: 650;"
      " margin: 0; }"
      "pre.log { text-align: left; font: 0.8rem/1.4 ui-monospace, monospace;"
      " max-height: 16rem; overflow: auto; background: #fff; padding: 0.75rem;"
      " border-radius: 0.5rem; border: 1px solid #e3b7c4; white-space: pre-wrap;"
      " }"
      "input[type=range] { width: 100%; }"
      ".dedication { font-size: 1.15rem; margin: 2rem 0; }"
      "</style></head><body><main><h1>Hug-Bot</h1>");

  if (sta_connecting) {
    html += F("<p class=\"lede\">Connecting. This page will update when "
              "done.</p><p class=\"refresh\"><a href=\"/?tab=wifi\">Continue"
              "</a></p></main></body></html>");
    SendNoCacheHeaders();
    server.send(200, "text/html", html);
    return;
  }

  const Tab tab = ParseTab();
  if (tab == Tab::Home) {
    AppendMenu(html);
  } else {
    AppendBack(html);
    if (tab == Tab::Volume) {
      AppendVolumePanel(html);
    } else if (tab == Tab::Logs) {
      AppendLogsPanel(html);
    } else if (tab == Tab::About) {
      AppendAboutPanel(html);
    } else {
      AppendWifiPanel(html);
    }
  }

  html += F("</main></body></html>");
  SendNoCacheHeaders();
  server.send(200, "text/html", html);
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

  if (ssid.isEmpty() || ssid.length() > wifi_t::kSsidMax) {
    page_status = "Select or enter a network name.";
    SendPortalPage();
    return;
  }
  if (psk.length() > wifi_t::kPskMax ||
      (psk.length() > 0 && psk.length() < 8)) {
    page_status = "Password must be at least 8 characters, or blank if the "
                  "network is open.";
    SendPortalPage();
    return;
  }

  Serial.print("Connecting to ");
  Serial.println(ssid);

  sta_connecting = true;
  SendPortalPage();

  if (!sta::Connect(ssid.c_str(), psk.c_str())) {
    sta_connecting = false;
    if (sta::wanted()) {
      page_status = "Could not connect. Check the password and try again.";
      log::Error("Wi-Fi join failed");
    }
    return;
  }

  page_status = "Connected.";
  sta_connecting = false;
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
}

void HandleDisconnect() {
  sta_connecting = false;
  sta::Disconnect();
  page_status = "Disconnected from Wi-Fi.";
  Serial.println("STA disconnected; Hug-Bot AP still up.");
  server.sendHeader("Location", "/?tab=wifi", true);
  SendNoCacheHeaders();
  server.send(302, "text/plain", "");
}

void HandleRefresh() {
  ScanNetworks();
  page_status = nullptr;
  server.sendHeader("Location", "/?tab=wifi", true);
  SendNoCacheHeaders();
  server.send(302, "text/plain", "");
}

void HandleVolume() {
  int parsed = 80;
  if (server.hasArg("volume")) {
    parsed = server.arg("volume").toInt();
  }
  if (parsed < 0) {
    parsed = 0;
  }
  if (parsed > 100) {
    parsed = 100;
  }
  pref::SetVolume(static_cast<uint8_t>(parsed));
  server.sendHeader("Location", "/?tab=volume", true);
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
  server.on("/volume", HTTP_POST, HandleVolume);
  server.on("/refresh", HandleRefresh);
  server.onNotFound(SendPortalPage);
  routes_bound = true;
}

}  // namespace

void setup() {
  sta::SetPump(Pump);
  StartControlAp();
  ScanNetworks();
}

void loop() {
  Pump();
}

}  // namespace cp
}  // namespace hb
