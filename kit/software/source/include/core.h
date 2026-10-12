#pragma once
#include <stddef.h>
#include <stdint.h>

namespace core {
inline uint32_t elapsed(uint32_t now, uint32_t since) { return now - since; }

class Debouncer {
public:
  void begin(bool down, uint32_t now) {
    stable_ = candidate_ = down;
    since_ = now;
  }
  bool update(bool down, uint32_t now, uint32_t debounceMs) {
    if (down != candidate_) {
      candidate_ = down;
      since_ = now;
    }
    if (stable_ != candidate_ && elapsed(now, since_) >= debounceMs) {
      stable_ = candidate_;
      return true;
    }
    return false;
  }
  bool down() const { return stable_; }

private:
  bool stable_ = false, candidate_ = false;
  uint32_t since_ = 0;
};

enum class ButtonEvent { None, Click, LongPress };
class ButtonGesture {
public:
  void begin(bool down, uint32_t now) {
    down_ = down;
    started_ = now;
    longSent_ = false;
    armed_ = !down;
  }
  ButtonEvent update(bool down, uint32_t now, uint32_t longMs) {
    if (down && !down_) {
      started_ = now;
      longSent_ = false;
    }
    ButtonEvent result = ButtonEvent::None;
    if (armed_ && down && !longSent_ && elapsed(now, started_) >= longMs) {
      result = ButtonEvent::LongPress;
      longSent_ = true;
    }
    if (!down && down_) {
      if (armed_ && !longSent_)
        result = elapsed(now, started_) >= longMs ? ButtonEvent::LongPress : ButtonEvent::Click;
      armed_ = true;
    }
    down_ = down;
    return result;
  }

private:
  bool down_ = false, longSent_ = false, armed_ = true;
  uint32_t started_ = 0;
};

// Call on every A/B transition. Invalid two-bit jumps reset the partial step.
class Quadrature {
public:
  void begin(uint8_t ab) {
    previous_ = ab & 3;
    partial_ = 0;
  }
  int update(uint8_t ab, int transitions) {
    static const int8_t delta[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
    ab &= 3;
    if ((ab ^ previous_) == 3)
      partial_ = 0;
    else
      partial_ += delta[(previous_ << 2) | ab];
    previous_ = ab;
    if (partial_ >= transitions) {
      partial_ = 0;
      return 1;
    }
    if (partial_ <= -transitions) {
      partial_ = 0;
      return -1;
    }
    return 0;
  }

private:
  uint8_t previous_ = 3;
  int partial_ = 0;
};

enum class ActionKind : uint8_t { None, Keyboard, Consumer };
struct Action {
  ActionKind kind;
  uint8_t modifiers;
  uint16_t usage;
  const char *label;
};
constexpr Action none(const char *label = "Non assegnato") {
  return {ActionKind::None, 0, 0, label};
}
constexpr Action key(uint8_t modifiers, uint8_t usage, const char *label) {
  return {ActionKind::Keyboard, modifiers, usage, label};
}
constexpr Action media(uint16_t usage, const char *label) {
  return {ActionKind::Consumer, 0, usage, label};
}

class ReportSink {
public:
  virtual ~ReportSink() = default;
  virtual bool ready() const = 0;
  virtual void press(const Action &action) = 0;
  virtual void release() = 0;
};

// Macro taps, not held physical key passthrough. Each chord has a separate release.
class MacroQueue {
public:
  static constexpr size_t kCapacity = 32;
  bool enqueue(const Action &action, bool ready) {
    if (!ready || action.kind == ActionKind::None || size_ == kCapacity)
      return false;
    queue_[(head_ + size_) % kCapacity] = action;
    ++size_;
    return true;
  }
  void reset() {
    head_ = size_ = 0;
    state_ = State::Idle;
  }
  bool busy() const { return size_ != 0 || state_ != State::Idle; }
  void tick(uint32_t now, ReportSink &sink, uint32_t holdMs, uint32_t gapMs) {
    if (!sink.ready()) {
      reset();
      return;
    }
    if (state_ == State::Pressed && elapsed(now, since_) >= holdMs) {
      sink.release();
      state_ = State::Gap;
      since_ = now;
    } else if (state_ == State::Gap && elapsed(now, since_) >= gapMs) {
      state_ = State::Idle;
    }
    if (state_ == State::Idle && size_) {
      sink.press(queue_[head_]);
      head_ = (head_ + 1) % kCapacity;
      --size_;
      state_ = State::Pressed;
      since_ = now;
    }
  }

private:
  enum class State { Idle, Pressed, Gap };
  Action queue_[kCapacity]{};
  size_t head_ = 0, size_ = 0;
  State state_ = State::Idle;
  uint32_t since_ = 0;
};

enum class PowerState { Active, Idle, Sleep };
inline PowerState powerState(uint32_t now, uint32_t activity, uint32_t idleMs, uint32_t sleepMs) {
  const uint32_t age = elapsed(now, activity);
  return age >= sleepMs ? PowerState::Sleep : age >= idleMs ? PowerState::Idle : PowerState::Active;
}
} // namespace core
