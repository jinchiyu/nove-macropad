#include "core.h"
#include "config.h"
#include "keymap.h"
#include "hid_reports.h"
#include "battery.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <vector>
#include <string>

struct Sink : core::ReportSink {
  bool online = true;
  std::vector<std::string> events;
  bool ready() const override { return online; }
  void press(const core::Action &a) override { events.push_back(a.label); }
  void release() override { events.push_back("release"); }
};

void debounce() {
  core::Debouncer d;
  d.begin(false, 0);
  assert(!d.update(true, 1, 8));
  assert(!d.update(false, 3, 8));
  assert(!d.update(true, 5, 8));
  assert(!d.update(true, 12, 8));
  assert(d.update(true, 13, 8) && d.down());
  assert(!d.update(true, 90, 8));
  assert(!d.update(false, 91, 8));
  assert(d.update(false, 99, 8) && !d.down());
  d.begin(false, 0xfffffff0u);
  assert(!d.update(true, 0xfffffffcu, 8));
  assert(!d.update(true, 3, 8));
  assert(d.update(true, 4, 8));
  // Nine independent keys: a rectangle or all keys produce exactly nine edges.
  core::Debouncer matrix[9];
  int edges = 0;
  for (auto &k : matrix) {
    k.begin(false, 0);
    k.update(true, 1, 8);
    edges += k.update(true, 9, 8);
  }
  assert(edges == 9);
}

void gestures() {
  core::ButtonGesture b;
  using E = core::ButtonEvent;
  b.begin(false, 0);
  assert(b.update(true, 10, 700) == E::None);
  assert(b.update(false, 100, 700) == E::Click);
  b.update(true, 200, 700);
  assert(b.update(true, 900, 700) == E::LongPress);
  assert(b.update(true, 1000, 700) == E::None);
  assert(b.update(false, 1100, 700) == E::None); // no mute after changing layer
  b.begin(true, 0);
  assert(b.update(true, 1000, 700) == E::None); // button held during boot ignored
  assert(b.update(false, 1001, 700) == E::None);
  b.update(true, 1100, 700);
  assert(b.update(false, 1900, 700) == E::LongPress); // slow polling still long, not click
  b.begin(false, 0xfffffff0u);
  b.update(true, 0xfffffff0u, 700);
  assert(b.update(true, 684, 700) == E::LongPress);
}

void quadrature() {
  core::Quadrature q;
  q.begin(3);
  int n = 0;
  for (auto ab : {1, 0, 2, 3})
    n += q.update(ab, 4);
  assert(n == 1);
  n = 0;
  for (auto ab : {2, 0, 1, 3})
    n += q.update(ab, 4);
  assert(n == -1);
  q.begin(3);
  n = 0;
  for (auto ab : {1, 3, 1, 0, 1, 0, 2, 3})
    n += q.update(ab, 4);
  assert(n == 1); // contact bounce cancels
  q.begin(3);
  assert(q.update(0, 4) == 0); // impossible jump
  assert(q.update(2, 4) == 0 && q.update(3, 4) == 0);
  q.begin(3);
  assert(q.update(1, 2) == 0 && q.update(0, 2) == 1);
}

void macros() {
  core::MacroQueue q;
  Sink s;
  const auto copy = keymap::layers[0].keys[0], paste = keymap::layers[0].keys[1];
  assert(!q.enqueue(copy, false));
  assert(!q.enqueue(core::none(), true));
  assert(q.enqueue(copy, true) && q.enqueue(paste, true));
  q.tick(0, s, 30, 30);
  q.tick(29, s, 30, 30);
  assert(s.events == std::vector<std::string>{"Copy"});
  q.tick(30, s, 30, 30);
  q.tick(60, s, 30, 30);
  q.tick(90, s, 30, 30);
  q.tick(120, s, 30, 30);
  assert((s.events == std::vector<std::string>{"Copy", "release", "Paste", "release"}));
  assert(!q.busy());
  q.enqueue(copy, true);
  q.tick(121, s, 30, 30);
  q.enqueue(paste, true);
  s.online = false;
  q.tick(122, s, 30, 30);
  assert(!q.busy());
  const auto count = s.events.size();
  s.online = true;
  q.tick(200, s, 30, 30);
  assert(s.events.size() == count); // no replay of offline inputs
  for (size_t i = 0; i < q.kCapacity; ++i)
    assert(q.enqueue(copy, true));
  assert(!q.enqueue(paste, true));
  q.reset();
  assert(!q.busy());
  // All nine simultaneous macro taps are preserved and separated by releases.
  s.events.clear();
  for (int i = 0; i < 9; ++i)
    assert(q.enqueue(copy, true));
  for (uint32_t t = 0; t <= 540; ++t)
    q.tick(t, s, 30, 30);
  assert(s.events.size() == 18 && !q.busy());
  q.enqueue(copy, true);
  s.events.clear();
  q.tick(0xfffffff0u, s, 30, 30);
  q.tick(14, s, 30, 30);
  assert(s.events.back() == "release");
}

void reportsAndPower() {
  uint8_t keyboard[8], consumer[2];
  hid::keyboard(keymap::layers[0].keys[0], keyboard);
  assert(keyboard[0] == 0x08 && keyboard[1] == 0 && keyboard[2] == 0x06 && keyboard[7] == 0);
  hid::consumer(core::media(0x02AB, "test"), consumer);
  assert(consumer[0] == 0xAB && consumer[1] == 0x02);
  for (const auto &layer : keymap::layers)
    for (const auto &a : layer.keys) {
      assert(a.kind != core::ActionKind::Keyboard || a.usage <= 0x65);
      assert(a.kind != core::ActionKind::Consumer || a.usage <= 0x3ff);
    }
  using P = core::PowerState;
  assert(core::powerState(29999, 0, 30000, 120000) == P::Active);
  assert(core::powerState(30000, 0, 30000, 120000) == P::Idle);
  assert(core::powerState(120000, 0, 30000, 120000) == P::Sleep);
  assert(core::powerState(120001, 120001, 30000, 120000) == P::Active);
  assert(core::powerState(29984, 0xfffffff0u, 30000, 120000) == P::Idle);
  const auto battery = battery::read();
  assert(!battery.available && isnan(battery.volts) && battery.percent == -1);
}

void reportDescriptor() {
  // Parse short HID items; verify actual report lengths, including LED padding.
  unsigned bits[3][3] = {}, reportId = 0, count = 0, size = 0;
  for (size_t i = 0; i < sizeof(hid::kReportMap);) {
    const auto tag = hid::kReportMap[i++];
    assert(tag != 0xFE);
    const unsigned bytes = (tag & 3) == 3 ? 4 : tag & 3;
    assert(i + bytes <= sizeof(hid::kReportMap));
    unsigned value = 0;
    for (unsigned n = 0; n < bytes; ++n)
      value |= unsigned(hid::kReportMap[i++]) << (8 * n);
    if ((tag & 0xFC) == 0x84)
      reportId = value;
    if ((tag & 0xFC) == 0x74)
      size = value;
    if ((tag & 0xFC) == 0x94)
      count = value;
    assert(reportId <= 2);
    if ((tag & 0xFC) == 0x80)
      bits[reportId][0] += count * size;
    if ((tag & 0xFC) == 0x90)
      bits[reportId][1] += count * size;
  }
  assert(bits[1][0] == 64 && bits[1][1] == 8);
  assert(bits[2][0] == 16 && bits[2][1] == 0);
}
int main() {
  debounce();
  gestures();
  quadrature();
  macros();
  reportsAndPower();
  reportDescriptor();
  puts("PASS: debounce, nine-key events, gestures, quadrature, macro lifecycle, HID "
       "bytes/descriptor, unavailable battery, power, millis rollover");
}
