#pragma once
#include "core.h"
class BleHid : public core::ReportSink {
public:
  void begin();
  void poll();
  bool resumeOnInput(bool input);
  bool ready() const override;
  bool connected() const;
  uint32_t session() const;
  void press(const core::Action &action) override;
  void release() override;
  void setIdle(bool idle);
  void forgetBonds();
};

#include <ArduinoJson.h>
namespace ble_diagnostic { void snapshot(JsonObject data); }
