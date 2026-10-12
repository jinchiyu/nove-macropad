#pragma once
#include "core.h"
class KeyboardMatrix {
public:
  void begin(uint32_t now);
  void scan(uint32_t now, bool (&pressed)[9], bool &activity);
  bool anyDown() const;

private:
  bool read(uint16_t &mask) const;
  bool online_ = false;
  uint32_t retryAt_ = 0;
  core::Debouncer keys_[9];
};
