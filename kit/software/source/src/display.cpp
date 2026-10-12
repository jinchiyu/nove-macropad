#include "display.h"
#include "config.h"
#include "pins.h"
#include "logging.h"
#include "studio_config.h"
#include "battery.h"
#include <Wire.h>
#if MACROPAD_OLED_ENABLED
#include <U8g2lib.h>
#endif

namespace {
bool present = false, sleeping = false;
#if MACROPAD_OLED_ENABLED
#if MACROPAD_OLED_SH1106
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE, pins::scl, pins::sda);
#else
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE, pins::scl, pins::sda);
#endif
bool probe() {
  Wire.beginTransmission(config::kOledAddress);
  return Wire.endTransmission() == 0;
}
#endif
} // namespace
void display::begin() {
#if MACROPAD_OLED_ENABLED
  Wire.begin(pins::sda, pins::scl, config::kI2cHz);
  Wire.setTimeOut(30);
  present = probe();
  if (!present) {
    LOGF("[OLED] not detected; HID remains available\n");
    return;
  }
  oled.setI2CAddress(config::kOledAddress << 1);
  oled.setBusClock(config::kI2cHz);
  Wire.end(); // U8g2 initializes the shared bus on the explicit D2/D5 pins.
  oled.begin();
  Wire.setTimeOut(30);
  LOGF("[OLED] detected 0x%02X (controller selected at build time)\n", config::kOledAddress);
#endif
}
bool display::available() { return present; }
void display::render(const DisplayState &state) {
#if MACROPAD_OLED_ENABLED
  if (!present)
    return;
  // Stop touching a failed/disconnected display. Recovery is by reboot.
  if (!probe()) {
    present = false;
    LOGF("[OLED] lost; continuing headless\n");
    return;
  }
  if (state.asleep != sleeping) {
    sleeping = state.asleep;
    oled.setPowerSave(sleeping);
  }
  if (sleeping)
    return;
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(0, 10, state.ready ? "BLE OK" : state.connected ? "BLE WAIT" : "BLE ADV");
  oled.drawStr(102, 10, "USB");
  oled.drawHLine(0, 15, 128);
  oled.setFont(u8g2_font_9x15B_tf);
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawUTF8(0, 33, studio::profile.layers[state.layer].name);
  oled.setFont(u8g2_font_6x10_tf);
  char number[16];
  snprintf(number, sizeof(number), "LAYER %u", state.layer + 1);
  oled.drawStr(78, 47, number);
  oled.drawStr(0, 47, "NOVE");
  oled.drawUTF8(0, 62, state.feedback ? state.feedback : "Hold: cambia layer");
  oled.sendBuffer();
  if (!probe())
    present = false;
#else
  (void)state;
#endif
}

void display::renderBoot(uint32_t elapsed) {
#if MACROPAD_OLED_ENABLED
  if (!present)
    return;
  if (!probe()) {
    present = false;
    return;
  }
  if (sleeping) {
    oled.setPowerSave(false);
    sleeping = false;
  }
  oled.clearBuffer();
  oled.setFont(u8g2_font_logisoso24_tf);
  const char *logo = "NOVE";
  int width = oled.getStrWidth(logo), left = (128 - width) / 2;
  int reveal = elapsed >= 800 ? width : width * elapsed / 800;
  if (reveal > 0) {
    oled.setClipWindow(left, 0, left + reveal, 64);
    oled.drawStr(left, 44, logo);
    oled.setMaxClipWindow();
  }
  oled.sendBuffer();
  if (!probe())
    present = false;
#else
  (void)elapsed;
#endif
}
