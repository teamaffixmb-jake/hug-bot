#ifndef HUG_BOT_API_H
#define HUG_BOT_API_H

#include <cstdint>

#if __has_include("secrets.h")
#include "secrets.h"
#else
namespace hb {
namespace api {
constexpr char k_host[] = "";
constexpr char k_device_a_token[] = "";
constexpr char k_device_b_token[] = "";
}  // namespace api
}  // namespace hb
#endif

#define HB_DEVICE_B 1

namespace hb {
namespace api {

#if HB_DEVICE_B
constexpr const char *k_token = k_device_b_token;
#else
constexpr const char *k_token = k_device_a_token;
#endif

// Client for the hug Lambda (infra/hug-bot.yaml). AWS holds all hug state;
// this board only shows it. Requests run on their own FreeRTOS task so a
// slow TLS round trip never stalls the animations. Uses HTTPClient from the
// ESP32 core (no extra library).
void setup();

// Polls every few seconds while started. Both are idempotent.
void start();
void stop();

// The last request reached AWS.
bool online();

// Set when AWS stops answering. Cleared on read.
enum class connect_attempt_t : uint8_t { none, fail };
connect_attempt_t take_connect_attempt();

// What AWS last said, from this board's point of view.
struct view_t {
  bool valid;        // false until the first reply
  int32_t pending;   // >0 my hugs unanswered, <0 their hugs I have not answered
  uint32_t received; // hugs the other board has ever sent me
  uint32_t seen;     // of those, how many I already animated
};
view_t view();

// Queues one press; it is retried until AWS takes it, and AWS counts each
// press once. False (nothing queued) if offline.
bool send_hug();

// Presses AWS has not confirmed yet.
uint8_t hugs_in_flight();

// Tells AWS the received animation has played through this count.
void mark_seen(uint32_t through);

}  // namespace api
}  // namespace hb

#endif  // HUG_BOT_API_H
