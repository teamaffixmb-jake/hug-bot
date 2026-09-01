#ifndef HUG_BOT_LOG_H
#define HUG_BOT_LOG_H

#include <cstddef>

namespace hb {
namespace log {

enum class level_t { warn, error };

constexpr size_t k_capacity = 24;
constexpr size_t k_line_max = 96;

void warn(const char *msg);
void error(const char *msg);

size_t size();

// index 0 is oldest. Returns false if index is out of range.
bool get(size_t index, level_t &level, char *msg, size_t msg_cap);

}  // namespace log
}  // namespace hb

#endif  // HUG_BOT_LOG_H
