#ifndef HUG_BOT_HUG_H
#define HUG_BOT_HUG_H

#include <cstdint>

namespace hb {
namespace hug {

void setup();
void loop();

// Stops talking to AWS. Call when leaving hug ops so requests do not run
// under the captive-portal AP.
void pause();

// Sends a hug to AWS. Invoked from mode::short_press(). Ignored while
// offline, since AWS is the only place a hug can be counted.
void send();

// True once AWS has confirmed every press sent so far.
bool send_settled();

// oled_ui calls this when the received-hug animation finishes.
void receive_shown();

// From AWS: >0 pending response, <0 missed hugs, saturated at ±999.
// 0 until AWS has answered once.
int32_t pending_for_display();

}  // namespace hug
}  // namespace hb

#endif  // HUG_BOT_HUG_H
