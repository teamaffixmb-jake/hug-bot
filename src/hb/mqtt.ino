#include "mqtt.h"
#include "log.h"
#include "mqtt_ca.h"

#include <Arduino.h>
#include <ArduinoMqttClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <ctime>

namespace hb {
namespace mqtt {
namespace {

constexpr uint16_t k_port = 8883;
constexpr char k_topic_prefix[] = "hugbot";
constexpr char k_payload[] = "h";
constexpr uint32_t k_retry_ms = 15000;
constexpr unsigned long k_keepalive_ms = 60000;
constexpr unsigned long k_connect_timeout_ms = 8000;
constexpr time_t k_min_epoch = 1700000000;  // 2023-11-14; TLS needs real time
constexpr size_t k_topic_max = 96;

WiFiClientSecure tls;
MqttClient client(tls);
bool ntp_started = false;
bool rx_pending = false;
uint32_t retry_at = 0;
char inbox[k_topic_max];
char outbox[k_topic_max];
char testbox[k_topic_max];
bool listen_test = false;

bool secrets_present() {
  return k_host[0] != '\0' && k_device_id[0] != '\0' && k_peer_id[0] != '\0' &&
         k_token[0] != '\0';
}

void make_topic(char *out, size_t cap, const char *device_id) {
  snprintf(out, cap, "%s/%s/inbox", k_topic_prefix, device_id);
}

bool time_ready() {
  return time(nullptr) >= k_min_epoch;
}

void start_ntp() {
  if (ntp_started) {
    return;
  }
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  ntp_started = true;
}

void drain_message() {
  while (client.available() > 0) {
    (void)client.read();
  }
}

void stop_client() {
  if (client.connected()) {
    client.stop();
  }
}

bool try_connect() {
  if (!secrets_present()) {
    log::error("MQTT secrets.h missing");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  start_ntp();
  if (!time_ready()) {
    return false;
  }

  tls.setCACert(k_amazon_root_ca1);
  client.setId(k_device_id);
  client.setUsernamePassword(k_device_id, k_token);
  client.setCleanSession(false);
  client.setKeepAliveInterval(k_keepalive_ms);
  client.setConnectionTimeout(k_connect_timeout_ms);

  if (!client.connect(k_host, k_port)) {
    log::error("MQTT connect failed");
    return false;
  }
  const char *sub = listen_test ? testbox : inbox;
  if (!client.subscribe(sub, 1)) {
    log::error("MQTT subscribe failed");
    client.stop();
    return false;
  }
  log::warn("MQTT connected");
  return true;
}

bool ensure_connected() {
  if (client.connected()) {
    return true;
  }
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  start_ntp();
  if (!time_ready()) {
    return false;
  }
  if (millis() < retry_at) {
    return false;
  }
  retry_at = millis() + k_retry_ms;
  return try_connect();
}

}  // namespace

void setup() {
  make_topic(inbox, sizeof(inbox), k_device_id);
  make_topic(outbox, sizeof(outbox), k_peer_id);
  snprintf(testbox, sizeof(testbox), "%s/%s/test", k_topic_prefix, k_device_id);
}

void listen_test_inbox() { listen_test = true; }

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    ntp_started = false;
    stop_client();
    return;
  }
  if (!ensure_connected()) {
    return;
  }

  client.poll();
  if (client.parseMessage() != 0) {
    rx_pending = true;
    log::warn("Hug received");
    drain_message();
  }
}

bool connected() { return client.connected(); }

bool take_received() {
  const bool pending = rx_pending;
  rx_pending = false;
  return pending;
}

bool publish_topic(const char *topic) {
  if (!ensure_connected()) {
    log::error("Hug send failed (MQTT)");
    return false;
  }

  if (!client.beginMessage(topic, false, 1) || client.print(k_payload) != 1 ||
      !client.endMessage()) {
    log::error("Hug send failed (MQTT)");
    return false;
  }
  log::warn("Hug sent");
  return true;
}

bool publish_hug() { return publish_topic(outbox); }

bool publish_test_inbox() { return publish_topic(testbox); }

}  // namespace mqtt
}  // namespace hb
