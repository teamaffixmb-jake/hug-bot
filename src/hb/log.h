#ifndef HUG_BOT_LOG_H
#define HUG_BOT_LOG_H

#include <cstddef>

namespace hb {
namespace log {

enum class Level { Warn, Error };

constexpr size_t kCapacity = 24;
constexpr size_t kLineMax = 96;

void Warn(const char *msg);
void Error(const char *msg);

size_t Size();

// index 0 is oldest. Returns false if index is out of range.
bool Get(size_t index, Level &level, char *msg, size_t msg_cap);

}  // namespace log
}  // namespace hb

#endif  // HUG_BOT_LOG_H
