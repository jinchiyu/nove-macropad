#pragma once
struct BatteryStatus {
  bool available;
  float volts;
  int percent; // -1 = unavailable, never render as a percentage
};
namespace battery {
BatteryStatus read();
}
