#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include "pins.h"
#include "config.h"
#include "logging.h"
#include "keyboard_matrix.h"
#include "encoder.h"
#include "ble_hid.h"
#include "display.h"
#include "boot_animation.h"
#include "ui.h"
#include "studio_config.h"
namespace {
Adafruit_NeoPixel leds(config::kLedCount, pins::ledData, NEO_GRB + NEO_KHZ800);
KeyboardMatrix matrix;
BleHid ble;
studio::Runner runner;
Ui ui;
boot::Animation startup;
bool forceLed = true;
uint8_t layer = 0;
uint32_t session = 0, lastActivity = 0;
bool wasReady = false, wasTest = false;
void submit(int index, uint32_t now) {
  auto &a = studio::profile.layers[layer].actions[index];
  if (!a.count) {
    ui.feedback("Non assegnato", now);
    return;
  }
  if (runner.enqueue(a, ble.ready()))
    ui.feedback(a.label, now);
  else
    ui.feedback(ble.ready() ? "Coda piena" : "BLE non pronto", now);
}
} // namespace
void setup() {
  // Web Serial sends whole JSON lines in one burst (up to 2048 bytes).
  // HWCDC defaults to 256 bytes and drops incoming bytes on queue overflow.
  Serial.setRxBufferSize(4096);
  Serial.begin(115200);
  setCpuFrequencyMhz(160);
  leds.begin();
  leds.clear();
  leds.show();
  studio::begin();
  Wire.begin(pins::sda, pins::scl, config::kI2cHz);
  Wire.setTimeOut(30);
  display::begin();
  matrix.begin(millis());
  encoder::begin(millis());
  ble.begin();
  lastActivity = millis();
  startup.begin(millis());
  LOGF("[BOOT] NOVE Studio 1.1.0-rc.1\n");
}
void loop() {
  uint32_t now = millis();
  ble.poll();
  studio::poll(ble.connected(), ble.ready(), layer);
  bool ready = ble.ready(), testing = studio::testing();
  if (session != ble.session() || wasReady != ready || wasTest != testing) {
    runner.reset(ble);
    session = ble.session();
    wasReady = ready;
    wasTest = testing;
    lastActivity = now;
  }
  bool applied = studio::takeApplied();
  if (applied) {
    runner.reset(ble);
    layer = 0;
    ui.invalidate();
    ui.feedback("Profilo salvato", now);
    lastActivity = now;
  }
  bool pressed[9]{}, activity = false;
  matrix.scan(now, pressed, activity);
  auto enc = encoder::poll(now);
  if (startup.update(now, applied || testing || activity || enc.activity || matrix.anyDown() ||
                              enc.down)) {
    ui.invalidate();
    forceLed = true;
  }
  if (activity || enc.activity || matrix.anyDown() || enc.down || runner.busy())
    lastActivity = now;
  if (enc.button == core::ButtonEvent::LongPress) {
    if (testing)
      studio::event("hold", 1);
    else {
      runner.reset(ble);
      layer = (layer + 1) % studio::profile.count;
      ui.feedback("Layer cambiato", now);
    }
  }
  for (int i = 0; i < 9; i++)
    if (pressed[i]) {
      if (testing)
        studio::event("key", i);
      else
        submit(i, now);
    }
  int steps = studio::profile.reverse ? -enc.steps : enc.steps;
  if (steps) {
    if (testing)
      studio::event("encoder", steps);
    else
      for (int i = 0; i < abs(steps); i++)
        submit(steps > 0 ? 9 : 10, now);
  }
  if (enc.button == core::ButtonEvent::Click) {
    if (testing)
      studio::event("click", 1);
    else
      submit(11, now);
  }
  if (!testing)
    runner.tick(now, ble);
  auto &p = studio::profile;
  uint32_t idle = now - lastActivity;
  bool screenOff = p.displayTimeout && idle >= p.displayTimeout && !testing;
  if (startup.active) {
    if (startup.oledDue(now))
      display::renderBoot(startup.elapsed(now));
    if (startup.ledDue(now)) {
      uint32_t rgb = p.layers[0].colour;
      for (uint16_t i = 0; i < config::kLedCount; i++) {
        uint8_t level = boot::level(startup.elapsed(now), i, config::kLedCount, p.brightness);
        leds.setPixelColor(i,
                           leds.Color(((rgb >> 16) & 255) * level / 255,
                                      ((rgb >> 8) & 255) * level / 255, (rgb & 255) * level / 255));
      }
      leds.show();
    }
    return;
  }
  ui.update(now, layer, ble.connected(), ready, screenOff);
  static uint32_t lastColour = 0;
  uint32_t colour = 0;
  if (ready && (!p.ledTimeout || idle < p.ledTimeout)) {
    uint32_t rgb = p.layers[layer].colour;
    colour = leds.Color(((rgb >> 16) & 255) * p.brightness / 100,
                        ((rgb >> 8) & 255) * p.brightness / 100, (rgb & 255) * p.brightness / 100);
  }
  if (forceLed || colour != lastColour) {
    forceLed = false;
    for (uint16_t i = 0; i < config::kLedCount; i++)
      leds.setPixelColor(i, colour);
    leds.show();
    lastColour = colour;
  }
  delay(1);
}
