#include "encoder.h"
#include "config.h"
#include "pins.h"
#include <Arduino.h>
#include <driver/gpio.h>

namespace {
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
constexpr uint8_t kBufferSize = 128;
volatile uint8_t samples[kBufferSize];
volatile uint8_t head = 0, tail = 0;
volatile bool overflow = false;
volatile bool moved = false;
core::Quadrature quadrature;
core::Debouncer button;
core::ButtonGesture gesture;
uint8_t IRAM_ATTR readAB() {
  return (gpio_get_level(static_cast<gpio_num_t>(pins::encoderA)) << 1) |
         gpio_get_level(static_cast<gpio_num_t>(pins::encoderB));
}
void IRAM_ATTR changed() {
  portENTER_CRITICAL_ISR(&mux);
  const uint8_t ab = readAB();
  moved = true;
  const uint8_t next = (head + 1) % kBufferSize;
  if (next == tail)
    overflow = true;
  else {
    samples[head] = ab;
    head = next;
  }
  portEXIT_CRITICAL_ISR(&mux);
}
} // namespace
void encoder::begin(uint32_t now) {
  pinMode(pins::encoderA, INPUT_PULLUP);
  pinMode(pins::encoderB, INPUT_PULLUP);
  pinMode(pins::encoderButton, INPUT_PULLUP);
  quadrature.begin(readAB());
  const bool down = digitalRead(pins::encoderButton) == LOW;
  button.begin(down, now);
  gesture.begin(down, now);
  attachInterrupt(digitalPinToInterrupt(pins::encoderA), changed, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pins::encoderB), changed, CHANGE);
}
EncoderEvents encoder::poll(uint32_t now) {
  uint8_t local[kBufferSize], count = 0;
  portENTER_CRITICAL(&mux);
  const bool lost = overflow;
  overflow = false;
  while (tail != head) {
    local[count++] = samples[tail];
    tail = (tail + 1) % kBufferSize;
  }
  const uint8_t current = readAB();
  const bool activity = moved;
  moved = false;
  portEXIT_CRITICAL(&mux);
  int steps = 0;
  if (lost)
    quadrature.begin(current); // discard ambiguous motion, never invent a detent
  else
    for (uint8_t i = 0; i < count; ++i)
      steps += quadrature.update(local[i], config::kEncoderTransitions);
  const bool edge =
      button.update(digitalRead(pins::encoderButton) == LOW, now, config::kDebounceMs);
  return {config::kEncoderReverse ? -steps : steps,
          gesture.update(button.down(), now, config::kLongPressMs), activity || edge,
          button.down()};
}
