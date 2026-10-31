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
uint32_t idleMs=0, activityFlags=0, timeoutMs=0, activityAt=0;
uint32_t transfers=0, errors=0, lastErrorAt=0, sleepAt=0, wakeAt=0;
int lastError=-1;
uint8_t phase=0;
uint32_t delayedAt=0, delayedCount=0, lastAckAt=0;
int delayed3c=-1, delayed3d=-1, delayed27=-1;
bool recovered=false;

struct Trace { uint32_t at; uint8_t phase, result; };
Trace trace[12]{}; unsigned traceCount=0;
void record(uint8_t result) {
 ++transfers;
 trace[traceCount++ % 12] = {millis(), phase, result};
 if(result) { ++errors; lastError=result; lastErrorAt=millis(); }
}

#if MACROPAD_OLED_ENABLED
#if MACROPAD_OLED_SH1106
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE, pins::scl, pins::sda);
#else
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE, pins::scl, pins::sda);
#endif
uint8_t trackedByte(u8x8_t *u8x8, uint8_t msg, uint8_t arg, void *ptr) {
 if(msg == U8X8_MSG_BYTE_END_TRANSFER) {
  const uint8_t result=Wire.endTransmission(); record(result); return 1;
 }
 return u8x8_byte_arduino_hw_i2c(u8x8,msg,arg,ptr);
}
bool probe() {
  Wire.beginTransmission(config::kOledAddress);
  const uint8_t result=Wire.endTransmission(); record(result); return result == 0;
}
#endif
} // namespace
void display::begin() {
#if MACROPAD_OLED_ENABLED
  phase=1;
  oled.getU8x8()->byte_cb=trackedByte;
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
  phase=2;
  // Stop touching a failed/disconnected display. Recovery is by reboot.
  if (!probe()) {
    present = false;
    LOGF("[OLED] lost; continuing headless\n");
    return;
  }
  if (state.asleep != sleeping) {
    sleeping = state.asleep;
    phase=sleeping?3:4;
    if(sleeping) sleepAt=millis(); else wakeAt=millis();
    oled.setPowerSave(sleeping);
  }
  if (sleeping)
    return;
  phase=5;
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
  phase=2;
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
  phase=5;
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
  phase=2;
  if (!probe())
    present = false;
#else
  (void)elapsed;
#endif
}

void display::activity(uint32_t idle,uint32_t flags,uint32_t timeout) {
 idleMs=idle;timeoutMs=timeout;
 if(flags) { activityFlags=flags;activityAt=millis(); }
}
void display::diagnostic(JsonObject d) {
 d["busHz"]=Wire.getClock();
 d["delayedAt"]=delayedAt;d["delayedCount"]=delayedCount;d["lastAckAt"]=lastAckAt;d["probe3c"]=delayed3c;d["probe3d"]=delayed3d;d["probe27"]=delayed27;
 d["uptimeMs"]=millis();d["idleMs"]=idleMs;d["timeoutMs"]=timeoutMs;
 d["lastActivityFlags"]=activityFlags;d["lastActivityAt"]=activityAt;
 d["present"]=present;d["sleeping"]=sleeping;d["sleepCommandAt"]=sleepAt;d["wakeCommandAt"]=wakeAt;
 d["transfers"]=transfers;d["errors"]=errors;d["lastError"]=lastError;d["lastErrorAt"]=lastErrorAt;
 auto a=d.createNestedArray("trace");
 unsigned start=traceCount>12?traceCount-12:0;
 for(unsigned i=start;i<traceCount;i++) {auto e=a.createNestedObject();auto &t=trace[i%12];e["ms"]=t.at;e["phase"]=t.phase;e["result"]=t.result;}
}

void display::pollDiagnostics(uint32_t now) {
 if(uint32_t(now-delayedAt)<1000) return;
 delayedAt=now; ++delayedCount;
 Wire.beginTransmission(0x3c);delayed3c=Wire.endTransmission();
 Wire.beginTransmission(0x3d);delayed3d=Wire.endTransmission();
 Wire.beginTransmission(0x27);delayed27=Wire.endTransmission();
 if(delayed3c==0) lastAckAt=now;
}
bool display::recover() {
 Wire.beginTransmission(0x3c);
 if(Wire.endTransmission()!=0) return false;
 sleeping=false;
 display::begin();
 recovered=present;
 return present;
}
bool display::takeRecovered() { bool value=recovered;recovered=false;return value; }
