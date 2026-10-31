#pragma once
#include <atomic>

namespace ble_recovery {
// Local input is the only fallback trigger; never resume on a timer or USB query.
inline bool resume(bool input, bool linked, bool encrypted, bool keyboard, bool media,
                   std::atomic<bool> &suspended) {
  if (!input || !linked || !encrypted || !keyboard || !media)
    return false;
  return suspended.exchange(false);
}
} // namespace ble_recovery
