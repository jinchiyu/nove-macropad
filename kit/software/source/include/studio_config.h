#pragma once
#include <Arduino.h>
#include "studio_model.h"
namespace studio {
extern Profile profile;
void begin();
void poll(bool connected, bool ready, uint8_t layer);
bool takeApplied();
bool testing();
void event(const char *control, int value);
} // namespace studio
