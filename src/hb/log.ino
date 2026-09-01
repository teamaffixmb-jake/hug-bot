#include "log.h"

#include <Arduino.h>

#include <cstring>

namespace hb {
namespace log {
namespace {

struct Entry {
  Level level;
  char msg[kLineMax];
};

Entry entries[kCapacity];
size_t count = 0;
size_t next = 0;

void Push(Level level, const char *msg) {
  Entry &slot = entries[next];
  slot.level = level;
  slot.msg[0] = '\0';
  if (msg != nullptr) {
    strncpy(slot.msg, msg, kLineMax - 1);
    slot.msg[kLineMax - 1] = '\0';
  }

  next = (next + 1) % kCapacity;
  if (count < kCapacity) {
    ++count;
  }

  Serial.print(level == Level::Error ? "E " : "W ");
  Serial.println(slot.msg);
}

}  // namespace

void Warn(const char *msg) { Push(Level::Warn, msg); }

void Error(const char *msg) { Push(Level::Error, msg); }

size_t Size() { return count; }

bool Get(size_t index, Level &level, char *msg, size_t msg_cap) {
  if (index >= count || msg == nullptr || msg_cap == 0) {
    return false;
  }
  const size_t oldest = (count < kCapacity) ? 0 : next;
  const Entry &entry = entries[(oldest + index) % kCapacity];
  level = entry.level;
  strncpy(msg, entry.msg, msg_cap - 1);
  msg[msg_cap - 1] = '\0';
  return true;
}

}  // namespace log
}  // namespace hb
