#pragma once
#include <stdint.h>
struct DisplayState {
  uint8_t layer;
  bool connected;
  bool ready;
  bool asleep;
  const char *feedback;
};
namespace display {
void begin();
void renderBoot(uint32_t elapsed);
void render(const DisplayState &state);
bool available();
} // namespace display
