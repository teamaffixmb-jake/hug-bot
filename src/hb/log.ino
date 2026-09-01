#include "log.h"

#include <Arduino.h>

#include <cstring>

namespace hb {
namespace log {
namespace {

struct entry_t {
  level_t level;
  char msg[k_line_max];
};

entry_t entries[k_capacity];
size_t count = 0;
size_t next = 0;

void push(level_t level, const char *msg) {
  entry_t &slot = entries[next];
  slot.level = level;
  slot.msg[0] = '\0';
  if (msg != nullptr) {
    strncpy(slot.msg, msg, k_line_max - 1);
    slot.msg[k_line_max - 1] = '\0';
  }

  next = (next + 1) % k_capacity;
  if (count < k_capacity) {
    ++count;
  }

  Serial.print(level == level_t::error ? "E " : "W ");
  Serial.println(slot.msg);
}

}  // namespace

void warn(const char *msg) { push(level_t::warn, msg); }

void error(const char *msg) { push(level_t::error, msg); }

size_t size() { return count; }

bool get(size_t index, level_t &level, char *msg, size_t msg_cap) {
  if (index >= count || msg == nullptr || msg_cap == 0) {
    return false;
  }
  const size_t oldest = (count < k_capacity) ? 0 : next;
  const entry_t &entry = entries[(oldest + index) % k_capacity];
  level = entry.level;
  strncpy(msg, entry.msg, msg_cap - 1);
  msg[msg_cap - 1] = '\0';
  return true;
}

}  // namespace log
}  // namespace hb
