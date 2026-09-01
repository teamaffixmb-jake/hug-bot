#ifndef HUG_BOT_STA_H
#define HUG_BOT_STA_H

#include "wifi_vo.h"

#include <cstddef>
#include <cstdint>

namespace hb {
namespace sta {

using PumpFn = void (*)();

struct ScanAp {
  char ssid[wifi_t::kSsidMax + 1];
  int32_t rssi;
  bool open;
};

void setup();
void loop();
void SetPump(PumpFn pump);

bool Connect(const char *ssid, const char *psk);
bool ConnectSaved();
void Disconnect();

bool wanted();

size_t Scan(ScanAp *out, size_t cap);

}  // namespace sta
}  // namespace hb

#endif  // HUG_BOT_STA_H
