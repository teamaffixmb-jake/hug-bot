#include "api.h"
#include "api_ca.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <cstdio>
#include <cstring>
#include <ctime>

namespace hb {
namespace api {
namespace {

constexpr uint32_t k_poll_ms = 5000;
constexpr uint32_t k_retry_ms = 3000;
constexpr uint16_t k_timeout_ms = 8000;
// One dropped request is common on Wi-Fi; only call it offline after two.
constexpr uint8_t k_offline_after = 2;
constexpr uint8_t k_queue_max = 20;
constexpr time_t k_min_epoch = 1700000000;  // 2023-11-14; TLS needs real time
constexpr uint32_t k_task_stack = 12288;
constexpr uint32_t k_idle_ms = 50;
constexpr size_t k_url_max = 128;

// Shared with the task; guarded by mux.
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
bool wanted = false;
bool is_online = false;
bool went_offline = false;
uint8_t queued = 0;
uint32_t seen_to_send = 0;  // 0 = nothing to report
view_t last_view = {};

// Task only.
WiFiClientSecure tls;
HTTPClient http;
char auth[96];
uint32_t boot_id = 0;
uint32_t seq = 1;
uint8_t failures = 0;
uint32_t retry_at = 0;
uint32_t poll_at = 0;
bool ntp_started = false;
bool session_open = false;

void close_session() {
  if (session_open) {
    http.end();
    tls.stop();
    session_open = false;
  }
}

void go_idle() {
  close_session();
  ntp_started = false;
  failures = 0;
  poll_at = 0;
  portENTER_CRITICAL(&mux);
  is_online = false;
  portEXIT_CRITICAL(&mux);
}

// One POST; on 200 fills out from "ok <pending> <received> <seen>".
bool request(const char *path, const char *body, view_t &out) {
  char url[k_url_max];
  snprintf(url, sizeof(url), "https://%s%s", k_host, path);
  if (!http.begin(tls, url)) {
    return false;
  }
  session_open = true;
  http.setReuse(true);
  http.setTimeout(k_timeout_ms);
  http.setConnectTimeout(k_timeout_ms);
  http.addHeader("Authorization", auth);
  http.addHeader("Content-Type", "text/plain");
  const int code = http.POST(
      reinterpret_cast<uint8_t *>(const_cast<char *>(body)), strlen(body));
  bool ok = false;
  if (code == 200) {
    const String text = http.getString();
    long pending = 0;
    unsigned long received = 0;
    unsigned long seen = 0;
    if (sscanf(text.c_str(), "ok %ld %lu %lu", &pending, &received, &seen) == 3) {
      out.valid = true;
      out.pending = static_cast<int32_t>(pending);
      out.received = static_cast<uint32_t>(received);
      out.seen = static_cast<uint32_t>(seen);
      ok = true;
    }
  }
  // Keeps the TLS connection open for the next call when the server allows.
  http.end();
  return ok;
}

void step() {
  bool want = false;
  uint8_t presses = 0;
  uint32_t seen = 0;
  portENTER_CRITICAL(&mux);
  want = wanted;
  presses = queued;
  seen = seen_to_send;
  portEXIT_CRITICAL(&mux);

  if (!want || WiFi.status() != WL_CONNECTED) {
    go_idle();
    vTaskDelay(pdMS_TO_TICKS(k_idle_ms));
    return;
  }
  if (!ntp_started) {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    ntp_started = true;
  }
  const uint32_t now = millis();
  if (time(nullptr) < k_min_epoch ||
      (failures > 0 && static_cast<int32_t>(now - retry_at) < 0)) {
    vTaskDelay(pdMS_TO_TICKS(k_idle_ms));
    return;
  }

  // The id stays the same until AWS confirms the press, so a retry after a
  // lost reply is not counted twice.
  char body[32] = "";
  const char *path = nullptr;
  if (presses > 0) {
    path = "/hug";
    snprintf(body, sizeof(body), "%08lx-%lu", static_cast<unsigned long>(boot_id),
             static_cast<unsigned long>(seq));
  } else if (seen > 0) {
    path = "/seen";
    snprintf(body, sizeof(body), "%lu", static_cast<unsigned long>(seen));
  } else if (static_cast<int32_t>(now - poll_at) >= 0) {
    path = "/state";
  } else {
    vTaskDelay(pdMS_TO_TICKS(k_idle_ms));
    return;
  }

  view_t got = {};
  const bool ok = request(path, body, got);
  if (ok) {
    failures = 0;
    poll_at = millis() + k_poll_ms;
    if (presses > 0) {
      seq = seq + 1;
    }
  } else {
    close_session();
    failures = failures < 255 ? failures + 1 : failures;
    retry_at = millis() + k_retry_ms;
  }

  portENTER_CRITICAL(&mux);
  if (ok) {
    last_view = got;
    is_online = true;
    if (presses > 0 && queued > 0) {
      queued = queued - 1;
    }
    if (presses == 0 && seen > 0 && seen_to_send == seen) {
      seen_to_send = 0;
    }
  } else if (failures >= k_offline_after && is_online) {
    is_online = false;
    went_offline = true;
  } else if (failures >= k_offline_after && failures % k_offline_after == 0) {
    went_offline = true;  // still failing; lets the UI replay its fail clip
  }
  portEXIT_CRITICAL(&mux);
}

void task(void *) {
  tls.setCACert(k_amazon_root_ca1);
  tls.setHandshakeTimeout(k_timeout_ms / 1000);
  for (;;) {
    step();
  }
}

}  // namespace

void setup() {
  snprintf(auth, sizeof(auth), "Bearer %s", k_token);
  boot_id = esp_random();
  if (k_host[0] == '\0' || k_token[0] == '\0') {
    return;  // hug logs this; never start talking to an empty host
  }
  // Core 0 runs Wi-Fi; Arduino's loop() is on core 1 and keeps animating.
  xTaskCreatePinnedToCore(task, "api", k_task_stack, nullptr, 1, nullptr, 0);
}

void start() {
  portENTER_CRITICAL(&mux);
  wanted = true;
  portEXIT_CRITICAL(&mux);
}

void stop() {
  portENTER_CRITICAL(&mux);
  wanted = false;
  is_online = false;
  portEXIT_CRITICAL(&mux);
}

bool online() {
  portENTER_CRITICAL(&mux);
  const bool up = is_online;
  portEXIT_CRITICAL(&mux);
  return up;
}

connect_attempt_t take_connect_attempt() {
  portENTER_CRITICAL(&mux);
  const bool failed = went_offline;
  went_offline = false;
  portEXIT_CRITICAL(&mux);
  return failed ? connect_attempt_t::fail : connect_attempt_t::none;
}

view_t view() {
  portENTER_CRITICAL(&mux);
  const view_t v = last_view;
  portEXIT_CRITICAL(&mux);
  return v;
}

bool send_hug() {
  bool ok = false;
  portENTER_CRITICAL(&mux);
  if (is_online && queued < k_queue_max) {
    queued = queued + 1;
    ok = true;
  }
  portEXIT_CRITICAL(&mux);
  return ok;
}

uint8_t hugs_in_flight() {
  portENTER_CRITICAL(&mux);
  const uint8_t n = queued;
  portEXIT_CRITICAL(&mux);
  return n;
}

void mark_seen(uint32_t through) {
  portENTER_CRITICAL(&mux);
  if (through > seen_to_send) {
    seen_to_send = through;
  }
  portEXIT_CRITICAL(&mux);
}

}  // namespace api
}  // namespace hb
