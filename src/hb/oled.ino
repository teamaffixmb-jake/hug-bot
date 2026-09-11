#include "oled.h"
#include "log.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>
#include <cstdio>

namespace hb {
namespace oled {
namespace {

constexpr uint8_t k_width = 128;
constexpr uint8_t k_height = 64;

Adafruit_SSD1306 *panel = nullptr;
bool started = false;
char msg[32];

uint8_t probe_addr() {
  const uint8_t addrs[] = {0x3C, 0x3D};
  for (uint8_t i = 0; i < sizeof(addrs); ++i) {
    Wire.beginTransmission(addrs[i]);
    if (Wire.endTransmission() == 0) {
      return addrs[i];
    }
  }
  return 0;
}

void splash() {
  panel->clearDisplay();
  panel->setTextColor(SSD1306_WHITE);
  panel->setTextSize(2);
  panel->setCursor(16, 8);
  panel->print("Hug-Bot");

  const int16_t hx = 64;
  const int16_t hy = 40;
  panel->fillCircle(hx - 5, hy, 6, SSD1306_WHITE);
  panel->fillCircle(hx + 5, hy, 6, SSD1306_WHITE);
  panel->fillTriangle(hx - 11, hy + 2, hx + 11, hy + 2, hx, hy + 18,
                      SSD1306_WHITE);
  panel->display();
}

}  // namespace

void setup() {
  log::warn("OLED init");
  Wire.begin();

  const uint8_t addr = probe_addr();
  if (addr == 0) {
    log::error("OLED not on I2C 0x3C/0x3D");
    return;
  }
  snprintf(msg, sizeof(msg), "OLED I2C 0x%02X", addr);
  log::warn(msg);

  // Heap, not a global: Adafruit file-scope ctor can drop USB on this
  // board. RST is -1 (938 auto-reset; pin left open). periphBegin false
  // — Wire is already up.
  panel = new Adafruit_SSD1306(k_width, k_height, &Wire, -1);
  started = panel != nullptr &&
            panel->begin(SSD1306_SWITCHCAPVCC, addr, false, false);
  if (!started) {
    log::error("OLED init failed");
    return;
  }
  panel->setTextWrap(false);
  splash();
  log::warn("OLED ready");
}

void loop() {}

bool ready() { return started; }

void clear() {
  if (started) {
    panel->clearDisplay();
  }
}

void flush() {
  if (started) {
    panel->display();
  }
}

void set_contrast(uint8_t level) {
  if (started) {
    panel->ssd1306_command(SSD1306_SETCONTRAST);
    panel->ssd1306_command(level);
  }
}

void invert(bool on) {
  if (started) {
    panel->invertDisplay(on);
  }
}

void print_at(int16_t x, int16_t y, uint8_t size, const char *text) {
  if (!started || text == nullptr) {
    return;
  }
  if (size < 1) {
    size = 1;
  }
  panel->setTextColor(SSD1306_WHITE);
  panel->setTextSize(size);
  panel->setCursor(x, y);
  panel->print(text);
}

}  // namespace oled
}  // namespace hb
