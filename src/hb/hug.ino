#include "hug.h"
#include "log.h"

namespace hb {
namespace hug {

void setup() {}

void loop() {}

void send() {
  log::warn("Hug send (broker not wired yet)");
}

}  // namespace hug
}  // namespace hb
