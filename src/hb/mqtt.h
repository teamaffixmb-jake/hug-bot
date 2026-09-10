#ifndef HUG_BOT_MQTT_H
#define HUG_BOT_MQTT_H

#include <cstdint>

#if __has_include("secrets.h")
#include "secrets.h"
#else
namespace hb {
namespace mqtt {
constexpr char k_host[] = "";
constexpr char k_device_a_token[] = "";
constexpr char k_device_b_token[] = "";
}  // namespace mqtt
}  // namespace hb
#endif

#define HB_DEVICE_B 0

namespace hb {
namespace mqtt {

constexpr char k_device_a_id[] = "hug-a";
constexpr char k_device_b_id[] = "hug-b";

#if HB_DEVICE_B
constexpr const char *k_device_id = k_device_b_id;
constexpr const char *k_peer_id = k_device_a_id;
constexpr const char *k_token = k_device_b_token;
#else
constexpr const char *k_device_id = k_device_a_id;
constexpr const char *k_peer_id = k_device_b_id;
constexpr const char *k_token = k_device_a_token;
#endif

// Requires the ArduinoMqttClient library.
void setup();
void loop();
bool connected();

// Publishes a one-byte hug to the peer inbox (QoS 1, not retained).
bool publish_hug();

// Test mailboxes hugbot/<id>/test-tx and hugbot/<id>/test-rx.
void listen_test_rx();
void listen_none();
bool publish_test_tx(uint8_t b);

// True if a message arrived since the last call (clears the flag).
bool take_received(uint8_t &b);

}  // namespace mqtt
}  // namespace hb

#endif  // HUG_BOT_MQTT_H
