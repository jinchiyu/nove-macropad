#include "keyboard_matrix.h"
#include "config.h"
#include "logging.h"
#include <Arduino.h>
#include <Wire.h>
namespace {
bool writePorts(uint8_t value) {
  Wire.beginTransmission(config::kPcfAddress);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}
} // namespace
bool KeyboardMatrix::read(uint16_t &mask) const {
  mask = 0;
  bool ok = true;
  for (uint8_t row = 0; row < 3; ++row) {
    if (!writePorts(uint8_t(0xff & ~(1 << row)))) {
      ok = false;
      break;
    }
    delayMicroseconds(150);
    if (Wire.requestFrom(config::kPcfAddress, uint8_t(1)) != 1) {
      ok = false;
      break;
    }
    const uint8_t ports = Wire.read();
    for (uint8_t col = 0; col < 3; ++col)
      if (!(ports & (1 << (4 + col))))
        mask |= 1 << (row * 3 + col);
  }
  const bool released = writePorts(0xff);
  return ok && released;
}
void KeyboardMatrix::begin(uint32_t now) {
  uint16_t mask = 0;
  online_ = read(mask);
  retryAt_ = now;
  for (uint8_t i = 0; i < 9; ++i)
    keys_[i].begin(online_ && (mask & (1 << i)), now);
  LOGF("[PCF] 0x27 %s\n", online_ ? "ready" : "absent");
}
void KeyboardMatrix::scan(uint32_t now, bool (&pressed)[9], bool &activity) {
  for (auto &p : pressed)
    p = false;
  if (!online_ && core::elapsed(now, retryAt_) < 1000)
    return;
  retryAt_ = now;
  uint16_t mask = 0;
  if (!read(mask)) {
    if (online_)
      LOGF("[PCF] communication lost\n");
    online_ = false;
    for (auto &key : keys_)
      key.begin(false, now);
    return;
  }
  if (!online_) {
    online_ = true;
    // Do not generate macro presses for keys held during reconnection.
    for (uint8_t i = 0; i < 9; ++i)
      keys_[i].begin(mask & (1 << i), now);
    LOGF("[PCF] recovered\n");
    return;
  }
  for (uint8_t i = 0; i < 9; ++i) {
    const bool changed = keys_[i].update(mask & (1 << i), now, config::kDebounceMs);
    pressed[i] = changed && keys_[i].down();
    activity |= changed;
  }
}
bool KeyboardMatrix::anyDown() const {
  for (const auto &key : keys_)
    if (key.down())
      return true;
  return false;
}
