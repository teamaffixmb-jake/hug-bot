#include "hug.h"
#include "mqtt.h"

namespace hb {
namespace hug {

void setup() { mqtt::setup(); }

void loop() { mqtt::loop(); }

void send() { mqtt::publish_hug(); }

}  // namespace hug
}  // namespace hb
