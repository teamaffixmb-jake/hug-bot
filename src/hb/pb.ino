#include "pb.h"

#include <Arduino.h>

namespace hb {
namespace pb {
namespace {

constexpr uint8_t k_pin = A1;

}  // namespace

void setup() { pinMode(k_pin, INPUT_PULLUP); }

bool down() { return digitalRead(k_pin) == LOW; }

}  // namespace pb
}  // namespace hb
