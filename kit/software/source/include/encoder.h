#pragma once
#include "core.h"
struct EncoderEvents {
  int steps;
  core::ButtonEvent button;
  bool activity;
  bool down;
};
namespace encoder {
void begin(uint32_t now);
EncoderEvents poll(uint32_t now);
} // namespace encoder
