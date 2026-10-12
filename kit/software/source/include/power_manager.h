#pragma once
#include "core.h"
class PowerManager {
public:
  void begin(uint32_t now) { lastActivity_ = now; }
  void activity(uint32_t now) { lastActivity_ = now; }
  bool update(uint32_t now);
  core::PowerState state() const { return state_; }

private:
  uint32_t lastActivity_ = 0;
  core::PowerState state_ = core::PowerState::Active;
};
