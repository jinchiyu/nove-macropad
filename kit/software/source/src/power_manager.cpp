#include "power_manager.h"
#include "config.h"
#include "logging.h"
bool PowerManager::update(uint32_t now) {
  const auto next = core::powerState(now, lastActivity_, config::kIdleMs, config::kSleepMs);
  if (next == state_)
    return false;
  state_ = next;
  // Application-level SLEEP: no esp_light_sleep_start/deep_sleep, BLE remains alive.
  const uint32_t mhz = next == core::PowerState::Sleep ? 80 : 160;
  const bool ok = setCpuFrequencyMhz(mhz);
  (void)ok;
  LOGF("[POWER] %s CPU=%luMHz (%s)\n",
       next == core::PowerState::Active ? "active"
       : next == core::PowerState::Idle ? "idle"
                                        : "sleep-connected",
       (unsigned long)mhz, ok ? "ok" : "unchanged");
  return true;
}
