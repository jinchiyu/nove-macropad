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

#include <ArduinoJson.h>
namespace display { void diagnostic(JsonObject data); void activity(uint32_t idle, uint32_t flags, uint32_t timeout); }

namespace display { void pollDiagnostics(uint32_t now); bool recover(); bool takeRecovered(); }
