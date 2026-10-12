#include "ui.h"
#include "config.h"
#include "core.h"
#include <stdio.h>
void Ui::feedback(const char *text, uint32_t now) {
  snprintf(feedback_, sizeof(feedback_), "%s", text);
  feedbackAt_ = now;
  dirty_ = true;
}
void Ui::update(uint32_t now, uint8_t layer, bool connected, bool ready, bool asleep) {
  if (feedback_[0] && core::elapsed(now, feedbackAt_) >= config::kFeedbackMs) {
    feedback_[0] = 0;
    dirty_ = true;
  }
  dirty_ |= layer != lastLayer_ || connected != lastConnected_ || ready != lastReady_ ||
            asleep != lastAsleep_;
  if (!dirty_ || core::elapsed(now, drawnAt_) < config::kDisplayFrameMs)
    return;
  display::render({layer, connected, ready, asleep, feedback_[0] ? feedback_ : nullptr});
  drawnAt_ = now;
  lastLayer_ = layer;
  lastConnected_ = connected;
  lastReady_ = ready;
  lastAsleep_ = asleep;
  dirty_ = false;
}
