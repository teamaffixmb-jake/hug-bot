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

constexpr char k_ap_ssid[] = "Hug-Bot";
// Setup-AP PSK. Same role as the WPA2 password printed on a router sticker:
// the 4-way handshake encrypts the air before the portal POST runs.
constexpr char k_ap_psk[] = "hugbot-setup";
constexpr uint16_t k_dns_port = 53;
constexpr uint16_t k_http_port = 80;
constexpr size_t k_max_networks = 16;

constexpr const char *k_probe_paths[] = {
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

sta::scan_ap_t networks[k_max_networks];
size_t network_count = 0;
const char *page_status = nullptr;
bool routes_bound = false;
bool sta_connecting = false;
bool hosting = false;

DNSServer dns_server;
WebServer http_server(k_http_port);

void pump() {
  dns_server.processNextRequest();
  http_server.handleClient();
}

void append_html_escaped(String &out, const char *s) {
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

void send_no_cache_headers() {
  http_server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  http_server.sendHeader("Pragma", "no-cache");
  http_server.sendHeader("Expires", "-1");
}

// 302/meta-refresh leave extra history entries, so Back looks broken.
void send_location_replace(const String &url) {
  String html;
  html.reserve(192);
  html += F("<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
            "<script>location.replace(\"");
  html += url;
  html += F("\");</script></head><body></body></html>");
  send_no_cache_headers();
  http_server.send(200, "text/html", html);
}

void handle_captive_probe() {
  send_location_replace(String("http://") + WiFi.softAPIP().toString() + "/");
}

void scan_networks() {
  network_count = sta::scan(networks, k_max_networks);
}

enum class tab_t { home, wifi, volume, logs, about };

tab_t parse_tab() {
  const String uri = http_server.uri();
  if (uri == "/connect" || uri == "/refresh") {
    return tab_t::wifi;
  }
  if (uri == "/volume") {
    return tab_t::volume;
  }

  if (!http_server.hasArg("tab")) {
    return tab_t::home;
  }
  const String tab = http_server.arg("tab");
  if (tab == "wifi") {
    return tab_t::wifi;
  }
  if (tab == "volume") {
    return tab_t::volume;
  }
  if (tab == "logs") {
    return tab_t::logs;
  }
  if (tab == "about") {
    return tab_t::about;
  }
  return tab_t::home;
}

void append_menu(String &html) {
  html += F(
      "<nav class=\"menu\">"
      "<a href=\"/?tab=wifi\">Wi-Fi</a>"
      "<a href=\"/?tab=volume\">Volume</a>"
      "<a href=\"/?tab=logs\">Logs</a>"
      "<a href=\"/?tab=about\">About</a>"
      "</nav>");
}

void append_back(String &html) {
  html += F("<p class=\"back\"><a href=\"/\""
            " onclick=\"location.replace('/');return false\">Back</a></p>");
}

void append_wifi_panel(String &html) {
  if (page_status != nullptr && page_status[0] != '\0') {
    html += F("<p class=\"status\">");
    append_html_escaped(html, page_status);
    html += F("</p>");
  }

  if (WiFi.status() == WL_CONNECTED) {
    html += F("<p class=\"status ok\">Connected to ");
    append_html_escaped(html, WiFi.SSID().c_str());
    html += F("<br>LAN IP: ");
    append_html_escaped(html, WiFi.localIP().toString().c_str());
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
    append_html_escaped(html, networks[i].ssid);
    html += F("\">");
    append_html_escaped(html, networks[i].ssid);
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

void append_volume_panel(String &html) {
  const uint8_t volume = pref::get_volume();
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

void append_logs_panel(String &html) {
  if (log::size() == 0) {
    html += F("<p class=\"lede\">No warnings or errors yet.</p>");
    return;
  }

  html += F("<pre class=\"log\">");
  char line[log::k_line_max];
  log::level_t level;
  for (size_t i = 0; i < log::size(); ++i) {
    if (!log::get(i, level, line, sizeof(line))) {
      continue;
    }
    html += (level == log::level_t::error) ? F("E  ") : F("W  ");
    append_html_escaped(html, line);
    html += '\n';
  }
  html += F("</pre>");
}

void append_about_panel(String &html) {
  html += F("<p class=\"lede dedication\">Made with love for Leon Masuda</p>");
}

void send_portal_page() {
  String html;
  html.reserve(8192);
  html += F(
      "<!DOCTYPE html><html lang=\"en\"><head>"
      "<meta charset=\"UTF-8\">"
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
      "<script>(function(){var h=\"");
  html += WiFi.softAPIP().toString();
  html += F("\";if(location.host!==h)location.replace(\"http://\"+h"
            "+location.pathname+location.search);})()</script>");
  if (sta_connecting) {
    html += F(
        "<script>setTimeout(function(){location.replace('/?tab=wifi')},1000)"
        "</script>");
  } else if (parse_tab() == tab_t::logs) {
    html += F(
        "<script>setTimeout(function(){location.replace('/?tab=logs')},2000)"
        "</script>");
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
      ".heart { color: #e03131; }"
      "</style></head><body><main><h1>Hug-Bot <span class=\"heart\">♥</span></h1>");

  if (http_server.uri() == "/refresh") {
    html += F("<p class=\"lede\">Scanning for networks…</p>"
              "<script>location.replace('/scan')</script>"
              "</main></body></html>");
    send_no_cache_headers();
    http_server.send(200, "text/html", html);
    return;
  }

  if (sta_connecting) {
    html += F("<p class=\"lede\">Connecting. This page will update when "
              "done.</p><p class=\"refresh\"><a href=\"/?tab=wifi\">Continue"
              "</a></p></main></body></html>");
    send_no_cache_headers();
    http_server.send(200, "text/html", html);
    return;
  }

  const tab_t tab = parse_tab();
  if (tab == tab_t::home) {
    append_menu(html);
  } else {
    append_back(html);
    if (tab == tab_t::volume) {
      append_volume_panel(html);
    } else if (tab == tab_t::logs) {
      append_logs_panel(html);
    } else if (tab == tab_t::about) {
      append_about_panel(html);
    } else {
      append_wifi_panel(html);
    }
  }

  html += F("</main></body></html>");
  send_no_cache_headers();
  http_server.send(200, "text/html", html);
}

void bind_routes();

void start_control_ap() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.mode(WIFI_AP_STA);
  } else {
    WiFi.mode(WIFI_AP);
  }
  WiFi.softAP(k_ap_ssid, k_ap_psk);

  const IPAddress ap_ip = WiFi.softAPIP();
  dns_server.start(k_dns_port, "*", ap_ip);
  bind_routes();
  http_server.begin();
  hosting = true;

  Serial.print("Setup SSID: ");
  Serial.println(k_ap_ssid);
  Serial.print("Setup password: ");
  Serial.println(k_ap_psk);
  Serial.print("Portal URL: http://");
  Serial.println(ap_ip);
  log::warn("Captive portal enabled");
}

void stop_control_ap() {
  dns_server.stop();
  http_server.stop();
  WiFi.softAPdisconnect(true);
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
  } else {
    WiFi.mode(WIFI_OFF);
  }
  hosting = false;
  sta_connecting = false;
  log::warn("Captive portal disabled");
}

void handle_connect() {
  String ssid = http_server.arg("ssid_custom");
  ssid.trim();
  if (ssid.isEmpty()) {
    ssid = http_server.arg("ssid");
    ssid.trim();
  }
  const String psk = http_server.arg("psk");

  if (ssid.isEmpty() || ssid.length() > wifi_t::k_ssid_max) {
    page_status = "Select or enter a network name.";
    send_location_replace("/?tab=wifi");
    return;
  }
  if (psk.length() > wifi_t::k_psk_max ||
      (psk.length() > 0 && psk.length() < 8)) {
    page_status = "Password must be at least 8 characters, or blank if the "
                  "network is open.";
    send_location_replace("/?tab=wifi");
    return;
  }

  Serial.print("Connecting to ");
  Serial.println(ssid);

  sta_connecting = true;
  send_portal_page();

  if (!sta::connect(ssid.c_str(), psk.c_str())) {
    sta_connecting = false;
    if (sta::wanted()) {
      page_status = "Could not connect. Check the password and try again.";
      log::error("Wi-Fi join failed");
    }
    return;
  }

  page_status = "Connected.";
  sta_connecting = false;
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
}

void handle_disconnect() {
  sta_connecting = false;
  sta::disconnect();
  page_status = "Disconnected from Wi-Fi.";
  Serial.println("STA disconnected; Hug-Bot AP still up.");
  send_location_replace("/?tab=wifi");
}

void handle_refresh() {
  send_portal_page();
}

void handle_scan() {
  scan_networks();
  page_status = nullptr;
  send_location_replace("/?tab=wifi");
}

void handle_volume() {
  int parsed = 80;
  if (http_server.hasArg("volume")) {
    parsed = http_server.arg("volume").toInt();
  }
  if (parsed < 0) {
    parsed = 0;
  }
  if (parsed > 100) {
    parsed = 100;
  }
  pref::set_volume(static_cast<uint8_t>(parsed));
  send_location_replace("/?tab=volume");
}

void bind_routes() {
  if (routes_bound) {
    return;
  }

  http_server.on("/", send_portal_page);
  for (const char *path : k_probe_paths) {
    if (path[0] == '/' && path[1] == '\0') {
      continue;
    }
    http_server.on(path, handle_captive_probe);
  }
  http_server.on("/connect", HTTP_POST, handle_connect);
  http_server.on("/disconnect", HTTP_POST, handle_disconnect);
  http_server.on("/volume", HTTP_POST, handle_volume);
  http_server.on("/refresh", handle_refresh);
  http_server.on("/scan", handle_scan);
  http_server.onNotFound(handle_captive_probe);
  routes_bound = true;
}

}  // namespace

void setup() {
  sta::set_pump(pump);
}

void loop() {
  if (!hosting) {
    return;
  }
  pump();
}

void enable() {
  if (hosting) {
    return;
  }
  start_control_ap();
  scan_networks();
}

void disable() {
  if (!hosting) {
    return;
  }
  stop_control_ap();
}

void toggle() {
  if (hosting) {
    disable();
  } else {
    enable();
  }
}

bool enabled() { return hosting; }

}  // namespace cp
}  // namespace hb
