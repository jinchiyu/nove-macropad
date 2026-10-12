#include <cassert>
#include <iostream>
#include "studio_model.h"
#include "studio_defaults.h"
struct Sink : core::ReportSink {
  bool on = true;
  int presses = 0, releases = 0;
  uint16_t last = 0;
  bool ready() const override { return on; }
  void press(const core::Action &a) override {
    presses++;
    last = a.usage;
  }
  void release() override { releases++; }
};
int main() {
  ArduinoJson::DynamicJsonDocument doc(98304);
  assert(!deserializeJson(doc, kStudioDefaults));
  studio::Profile p;
  assert(studio::parse(doc.as<JsonVariantConst>(), p));
  assert(p.count == 3);
  assert(p.layers[0].actions[0].steps[0].value == 6);
  doc["settings"]["brightness"] = 11;
  assert(!studio::parse(doc.as<JsonVariantConst>(), p));
  doc["settings"]["brightness"] = 10;
  doc["layers"][0]["actions"][0]["steps"][0]["modifiers"] = 256;
  assert(!studio::parse(doc.as<JsonVariantConst>(), p));
  assert(!deserializeJson(doc, kStudioDefaults));
  doc["settings"]["displayTimeoutMs"] = 1;
  assert(!studio::parse(doc.as<JsonVariantConst>(), p));
  Sink sink;
  studio::Runner r;
  studio::Action a;
  a.count = 3;
  a.steps[0] = {1, 8, 6};
  a.steps[1] = {3, 0, 500};
  a.steps[2] = {1, 8, 25};
  assert(r.enqueue(a, true));
  r.tick(0, sink);
  assert(sink.presses == 1 && sink.last == 6);
  r.tick(30, sink);
  assert(sink.releases == 1);
  r.tick(60, sink);
  r.tick(559, sink);
  assert(sink.presses == 1);
  r.tick(560, sink);
  assert(sink.presses == 2 && sink.last == 25);
  r.tick(590, sink);
  r.tick(620, sink);
  assert(!r.busy());
  a.count = 16;
  for (int i = 0; i < 16; i++)
    a.steps[i] = {1, 0, 4};
  for (int i = 0; i < 4; i++)
    assert(r.enqueue(a, true));
  assert(!r.enqueue(a, true));
  r.reset(sink);
  assert(!r.busy());
  assert(r.enqueue(a, true));
  sink.on = false;
  r.tick(700, sink);
  assert(!r.busy());
  assert(!r.enqueue(a, false));
  assert(studio::checksum("abc", 3) != studio::checksum("abd", 3));
  std::cout << "PASS Studio: defaults, validation, pauses, atomic queue, cancellation, disconnect, "
               "checksum\n";
}
