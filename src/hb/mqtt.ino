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
constexpr uint32_t k_retry_ms = 15000;
constexpr unsigned long k_keepalive_ms = 60000;
constexpr uint32_t k_connect_timeout_s = 8;
constexpr unsigned long k_connect_timeout_ms = k_connect_timeout_s * 1000;
constexpr time_t k_min_epoch = 1700000000;  // 2023-11-14; TLS needs real time
constexpr size_t k_topic_max = 96;

WiFiClientSecure tls;
MqttClient client(tls);
bool ntp_started = false;
bool rx_pending = false;
uint8_t rx_byte = 0;
uint32_t retry_at = 0;
char inbox[k_topic_max];
char outbox[k_topic_max];
char test_tx[k_topic_max];
char test_rx[k_topic_max];
enum class listen_t { hug, test, none };
listen_t listen = listen_t::hug;

void make_topic(char *out, size_t cap, const char *device_id, const char *leaf) {
  snprintf(out, cap, "%s/%s/%s", k_topic_prefix, device_id, leaf);
}

void stop_session() {
  ntp_started = false;
  rx_pending = false;
  retry_at = 0;
  tls.stop();
  client.stop();
}

void start_ntp() {
  if (!ntp_started) {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    ntp_started = true;
  }
}

bool try_connect() {
  if (k_host[0] == '\0' || k_token[0] == '\0') {
    log::error("MQTT secrets.h missing");
    return false;
  }

  tls.setCACert(k_amazon_root_ca1);
  tls.setHandshakeTimeout(k_connect_timeout_s);
  tls.setTimeout(k_connect_timeout_s);
  client.setId(k_device_id);
  client.setUsernamePassword(k_device_id, k_token);
  client.setCleanSession(listen != listen_t::hug);
  client.setKeepAliveInterval(k_keepalive_ms);
  client.setConnectionTimeout(k_connect_timeout_ms);

  if (!client.connect(k_host, k_port)) {
    log::error("MQTT connect failed");
    return false;
  }
  if (listen != listen_t::none) {
    const char *sub = (listen == listen_t::test) ? test_rx : inbox;
    if (!client.subscribe(sub)) {
      log::error("MQTT subscribe failed");
      client.stop();
      return false;
    }
  }
  log::warn("MQTT connected");
  return true;
}

bool publish_topic(const char *topic, uint8_t b) {
  if (!client.beginMessage(topic)) {
    return false;
  }
  client.write(b);
  return client.endMessage();
}

}  // namespace

void setup() {
  make_topic(inbox, sizeof(inbox), k_device_id, "inbox");
  make_topic(outbox, sizeof(outbox), k_peer_id, "inbox");
  make_topic(test_tx, sizeof(test_tx), k_device_id, "test-tx");
  make_topic(test_rx, sizeof(test_rx), k_device_id, "test-rx");
}

void listen_test_rx() { listen = listen_t::test; }

void listen_none() { listen = listen_t::none; }

void stop() { stop_session(); }

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (ntp_started) {
      stop_session();
    }
    return;
  }

  if (!client.connected()) {
    start_ntp();
    if (time(nullptr) < k_min_epoch || millis() < retry_at) {
      return;
    }
    try_connect();
    retry_at = millis() + k_retry_ms;
  }

  if (client.parseMessage() == 0) {
    return;
  }
  const int c = client.read();
  if (c < 0) {
    return;
  }
  rx_byte = static_cast<uint8_t>(c);
  rx_pending = true;
}

bool connected() { return client.connected(); }

bool take_received(uint8_t &b) {
  if (!rx_pending) {
    return false;
  }
  b = rx_byte;
  rx_pending = false;
  return true;
}

bool publish_hug() {
  if (!publish_topic(outbox, 'h')) {
    log::error("Hug send failed (MQTT)");
    return false;
  }
  log::warn("Hug sent");
  return true;
}

bool publish_test_tx(uint8_t b) { return publish_topic(test_tx, b); }

}  // namespace mqtt
}  // namespace hb
