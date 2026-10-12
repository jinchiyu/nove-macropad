#pragma once
#include <ArduinoJson.h>
#include <stdint.h>
#include <string.h>
#include "core.h"
namespace studio {
constexpr size_t MaxBytes = 32768;
struct Step {
  uint8_t kind = 0, modifiers = 0;
  uint16_t value = 0;
}; // 1 key, 2 media, 3 pause
struct Action {
  char label[49]{};
  uint8_t count = 0;
  Step steps[16]{};
};
struct Layer {
  char name[25]{};
  uint32_t colour = 0;
  Action actions[12]{};
};
struct Profile {
  uint8_t count = 0, brightness = 10;
  bool reverse = false;
  uint32_t ledTimeout = 30000, displayTimeout = 30000;
  Layer layers[8]{};
};
inline bool number(JsonVariantConst v, int lo, int hi) {
  return v.is<int>() && v.as<int>() >= lo && v.as<int>() <= hi;
}
inline bool text(JsonVariantConst v, size_t max, bool nonempty = false) {
  if (!v.is<const char *>())
    return false;
  const char *s = v.as<const char *>();
  size_t n = strlen(s);
  if (n > max || (nonempty && !n))
    return false;
  for (size_t i = 0; i < n; i++)
    if ((uint8_t)s[i] < 32 || s[i] == 127)
      return false;
  return true;
}
inline bool parse(JsonVariantConst root, Profile &p) {
  if (!root.is<JsonObjectConst>() || !number(root["schema"], 1, 1))
    return false;
  auto s = root["settings"];
  if (!s.is<JsonObjectConst>() || !number(s["brightness"], 0, 10) ||
      !s["encoderReverse"].is<bool>())
    return false;
  for (const char *field : {"ledTimeoutMs", "displayTimeoutMs"})
    if (!number(s[field], 0, 600000) || (s[field].as<int>() != 0 && s[field].as<int>() < 15000))
      return false;
  auto ls = root["layers"].as<JsonArrayConst>();
  if (ls.isNull() || ls.size() < 1 || ls.size() > 8)
    return false;
  p.count = ls.size();
  p.brightness = s["brightness"];
  p.reverse = s["encoderReverse"];
  p.ledTimeout = s["ledTimeoutMs"];
  p.displayTimeout = s["displayTimeoutMs"];
  for (size_t i = 0; i < ls.size(); i++) {
    auto l = ls[i];
    if (!text(l["name"], 24, true) || !text(l["color"], 7))
      return false;
    const char *c = l["color"];
    if (strlen(c) != 7 || c[0] != '#')
      return false;
    uint32_t rgb = 0;
    for (int j = 1; j < 7; j++) {
      char ch = c[j];
      int v = ch >= '0' && ch <= '9'   ? ch - '0'
              : ch >= 'a' && ch <= 'f' ? ch - 'a' + 10
              : ch >= 'A' && ch <= 'F' ? ch - 'A' + 10
                                       : -1;
      if (v < 0)
        return false;
      rgb = (rgb << 4) | v;
    }
    strcpy(p.layers[i].name, l["name"]);
    p.layers[i].colour = rgb;
    auto as = l["actions"].as<JsonArrayConst>();
    if (as.isNull() || as.size() != 12)
      return false;
    for (size_t j = 0; j < 12; j++) {
      auto a = as[j];
      if (!text(a["label"], 48))
        return false;
      auto steps = a["steps"].as<JsonArrayConst>();
      if (steps.isNull() || steps.size() > 16)
        return false;
      auto &dst = p.layers[i].actions[j];
      strcpy(dst.label, a["label"]);
      dst.count = steps.size();
      for (size_t k = 0; k < steps.size(); k++) {
        auto st = steps[k];
        auto &out = dst.steps[k];
        out = {};
        const char *kind = st["kind"] | "";
        if (!strcmp(kind, "key")) {
          if (!number(st["usage"], 4, 115) || !number(st["modifiers"], 0, 15))
            return false;
          out.kind = 1;
          out.value = st["usage"];
          out.modifiers = st["modifiers"];
        } else if (!strcmp(kind, "media")) {
          int u = st["usage"] | 0;
          if (!number(st["usage"], 0, 65535) ||
              (u != 181 && u != 182 && u != 183 && u != 205 && u != 226 && u != 233 && u != 234))
            return false;
          out.kind = 2;
          out.value = u;
        } else if (!strcmp(kind, "pause")) {
          if (!number(st["ms"], 50, 5000))
            return false;
          out.kind = 3;
          out.value = st["ms"];
        } else
          return false;
      }
    }
  }
  return true;
}
class Runner {
  Step queue_[64]{};
  size_t head_ = 0, size_ = 0;
  uint8_t state_ = 0;
  uint32_t since_ = 0, duration_ = 0;

public:
  bool busy() const { return size_ || state_; }
  void reset(core::ReportSink &sink) {
    head_ = size_ = state_ = 0;
    sink.release();
  }
  bool enqueue(const Action &a, bool ready) {
    if (!ready || !a.count || size_ + a.count > 64)
      return false;
    for (int i = 0; i < a.count; i++) {
      queue_[(head_ + size_) % 64] = a.steps[i];
      size_++;
    }
    return true;
  }
  void tick(uint32_t now, core::ReportSink &sink) {
    if (!sink.ready()) {
      if (busy())
        reset(sink);
      return;
    }
    if (state_ && core::elapsed(now, since_) >= duration_) {
      if (state_ == 1) {
        sink.release();
        state_ = 2;
        since_ = now;
        duration_ = 30;
      } else
        state_ = 0;
    }
    if (!state_ && size_) {
      auto s = queue_[head_];
      head_ = (head_ + 1) % 64;
      size_--;
      since_ = now;
      if (s.kind == 3) {
        state_ = 3;
        duration_ = s.value;
      } else {
        state_ = 1;
        duration_ = 30;
        sink.press({s.kind == 1 ? core::ActionKind::Keyboard : core::ActionKind::Consumer,
                    s.modifiers, s.value, ""});
      }
    }
  }
};
inline uint32_t checksum(const char *data, size_t n) {
  uint32_t h = 2166136261u;
  for (size_t i = 0; i < n; i++) {
    h ^= (uint8_t)data[i];
    h *= 16777619u;
  }
  return h;
}
} // namespace studio
