#pragma once
#include <stdint.h>

// GPIO numbers, not board header indices. XIAO ESP32-S3, NOT Plus.
namespace pins {
constexpr uint8_t ledData = 1;       // D0
constexpr uint8_t sda = 3;           // D2; D4 left unused after hardware test
constexpr uint8_t scl = 6;           // D5
constexpr uint8_t encoderA = 7;      // D8
constexpr uint8_t encoderB = 8;      // D9
constexpr uint8_t encoderButton = 9; // D10 (NOT GPIO10)
} // namespace pins
