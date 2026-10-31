#include "ble_recovery.h"
#include <cassert>
#include <iostream>

int main() {
  // Cover every combination, including missing encryption/subscriptions and no input.
  for (unsigned bits = 0; bits < 64; ++bits) {
    const bool initiallySuspended = bits & 32;
    std::atomic<bool> suspended{initiallySuspended};
    const bool expected = bits == 63;
    const bool recovered = ble_recovery::resume(bits & 1, bits & 2, bits & 4, bits & 8,
                                                bits & 16, suspended);
    assert(recovered == expected);
    assert(suspended == (initiallySuspended && !expected));
    if (recovered)
      assert(!ble_recovery::resume(true, true, true, true, true, suspended));
  }
  std::atomic<bool> suspended{true};
  // Repeated idle polling must never end host suspension.
  for (int i = 0; i < 10000; ++i)
    assert(!ble_recovery::resume(false, true, true, true, true, suspended));
  assert(suspended);
  assert(ble_recovery::resume(true, true, true, true, true, suspended));
  suspended = true; // a later host sleep must still be respected
  assert(!ble_recovery::resume(false, true, true, true, true, suspended));
  assert(ble_recovery::resume(true, true, true, true, true, suspended));
  std::cout << "PASS: BLE resume prerequisites, idle, repeated sleep and one-shot recovery\n";
}
