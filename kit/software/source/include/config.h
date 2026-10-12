#pragma once
#include <stdint.h>

#ifndef MACROPAD_LOGGING
#define MACROPAD_LOGGING 1
#endif
#ifndef MACROPAD_OLED_ENABLED
#define MACROPAD_OLED_ENABLED 1
#endif
#ifndef MACROPAD_OLED_SH1106
#define MACROPAD_OLED_SH1106 0
#endif

namespace config {
constexpr uint16_t kLedCount = 24;
constexpr char kName[] = "NOVE MacroPad";
constexpr uint32_t kDebounceMs = 8;
constexpr uint32_t kLongPressMs = 700;
constexpr uint32_t kFeedbackMs = 700;
constexpr uint32_t kIdleMs = 30000;
constexpr uint32_t kSleepMs = 120000;
constexpr uint32_t kReportHoldMs = 30;
constexpr uint32_t kReportGapMs = 30;
constexpr uint32_t kDisplayFrameMs = 100;
constexpr int kEncoderTransitions = 4; // 4 or 2, verify against physical detents
// Hardware direction corrected against clockwise rotation on the assembled MacroPad.
constexpr bool kEncoderReverse = true;
constexpr uint8_t kOledAddress = 0x3C; // change to 0x3D after I2C verification
constexpr uint8_t kPcfAddress = 0x27;  // measured on assembled hardware
constexpr uint32_t kI2cHz = 100000;
static_assert(kEncoderTransitions == 2 || kEncoderTransitions == 4, "Encoder: 2 or 4 transitions");
static_assert(kIdleMs < kSleepMs, "Power timeouts must increase");
} // namespace config
