#pragma once
#include "config.h"
#include <Arduino.h>
#if MACROPAD_LOGGING
// Never wait for a USB host: standalone boot must work.
#define LOGF(...)                                                                                  \
  do {                                                                                             \
    if (Serial)                                                                                    \
      Serial.printf(__VA_ARGS__);                                                                  \
  } while (0)
#else
#define LOGF(...)                                                                                  \
  do {                                                                                             \
  } while (0)
#endif
