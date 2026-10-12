#pragma once
#include <stdint.h>
namespace boot {
constexpr uint32_t Duration = 2000;
struct Animation {
  bool active = false;
  uint32_t start = 0, oledAt = 0, ledAt = 0;
  void begin(uint32_t now) {
    active = true;
    start = now;
    oledAt = now - 100;
    ledAt = now - 33;
  }
  bool update(uint32_t now, bool interrupt) {
    if (!active)
      return false;
    if (interrupt || uint32_t(now - start) >= Duration) {
      active = false;
      return true;
    }
    return false;
  }
  uint32_t elapsed(uint32_t now) const { return now - start; }
  bool oledDue(uint32_t now) {
    if (uint32_t(now - oledAt) < 100)
      return false;
    oledAt = now;
    return true;
  }
  bool ledDue(uint32_t now) {
    if (uint32_t(now - ledAt) < 33)
      return false;
    ledAt = now;
    return true;
  }
};
inline uint8_t level(uint32_t elapsed, uint16_t index, uint16_t count, uint8_t brightness) {
  if (!count || elapsed >= Duration)
    return 0;
  uint16_t head = elapsed * count / Duration;
  uint16_t distance = (head + count - index) % count;
  if (distance > 3)
    return 0;
  return (brightness > 10 ? 10 : brightness) * (4 - distance) * 255 / 400;
}
} // namespace boot
