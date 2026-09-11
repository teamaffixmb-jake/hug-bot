#ifndef HUG_BOT_OLED_H
#define HUG_BOT_OLED_H

#include <cstdint>

namespace hb {
namespace oled {

// Adafruit 938, 128x64 SSD1306, I2C (STEMMA default; do not cut J1/J2):
//   Vin  3V3 or VUSB
//   GND  GND
//   Data A4  SDA
//   Clk  A5  SCL
// CS, DC, Rst, 3Vo left open.
// Requires Library Manager: Adafruit GFX Library, Adafruit SSD1306,
// Adafruit BusIO.
void setup();
void loop();

bool ready();
void clear();
void flush();
void set_contrast(uint8_t level);
void invert(bool on);
void print_at(int16_t x, int16_t y, uint8_t size, const char *text);

}  // namespace oled
}  // namespace hb

#endif  // HUG_BOT_OLED_H
