#include "battery.h"
#include <math.h>
BatteryStatus battery::read() { return {false, NAN, -1}; }
