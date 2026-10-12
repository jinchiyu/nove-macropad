#pragma once
#include "display.h"
#include <stdint.h>
class Ui {
public:
  void invalidate() { dirty_ = true; }
  void feedback(const char *text, uint32_t now);
  void update(uint32_t now, uint8_t layer, bool connected, bool ready, bool asleep);

private:
  char feedback_[24]{};
  uint32_t feedbackAt_ = 0, drawnAt_ = 0;
  uint8_t lastLayer_ = 255;
  bool dirty_ = true, lastConnected_ = false, lastReady_ = false, lastAsleep_ = false;
};
