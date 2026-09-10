#ifndef HUG_BOT_HUG_H
#define HUG_BOT_HUG_H

namespace hb {
namespace hug {

void setup();
void loop();

// Physical trigger is not chosen yet. Call this from that layer when it
// exists. Broker publish is not wired yet; this is the send entrypoint.
void send();

}  // namespace hug
}  // namespace hb

#endif  // HUG_BOT_HUG_H
