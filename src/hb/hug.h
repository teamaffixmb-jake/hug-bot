#ifndef HUG_BOT_HUG_H
#define HUG_BOT_HUG_H

namespace hb {
namespace hug {

void setup();
void loop();

// Stops MQTT/TLS. Call when leaving hug ops so the session is not left
// up under the captive-portal AP.
void pause();

// Publishes a hug to the peer inbox. Invoked from mode::short_press().
void send();

}  // namespace hug
}  // namespace hb

#endif  // HUG_BOT_HUG_H
