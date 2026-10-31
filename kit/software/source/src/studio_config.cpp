#include "studio_config.h"
#include "display.h"
#include "ble_hid.h"
#include "studio_defaults.h"
#include <LittleFS.h>
#include <Preferences.h>
namespace studio {
Profile profile;
namespace {
struct Allocator {
  void *allocate(size_t n) { return psramFound() ? ps_malloc(n) : malloc(n); }
  void deallocate(void *p) { free(p); }
  void *reallocate(void *p, size_t n) {
    return psramFound() ? heap_caps_realloc(p, n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
                        : realloc(p, n);
  }
};
using Doc = BasicJsonDocument<Allocator>;
Profile candidate;
String current, incoming, line;
uint32_t revision = 0, uploadAt = 0, heartbeat = 0;
int activeSlot = -1;
bool fsReady = false, applied = false, test = false, dropping = false, uploading = false;
struct Header {
  uint32_t magic, revision, length, hash;
};
constexpr uint32_t Magic = 0x39394231;
const char *slot(int i) { return i ? "/studio1.bin" : "/studio0.bin"; }
bool validate(const String &raw, Profile &out) {
  if (raw.length() > MaxBytes)
    return false;
  Doc doc(98304);
  if (deserializeJson(doc, raw))
    return false;
  return parse(doc.as<JsonVariantConst>(), out);
}
bool readSlot(int n, String &raw, uint32_t &rev) {
  if (!LittleFS.exists(slot(n)))
    return false;
  File f = LittleFS.open(slot(n), "r");
  if (!f)
    return false;
  Header h{};
  if (f.read((uint8_t *)&h, sizeof(h)) != sizeof(h) || h.magic != Magic || h.length > MaxBytes ||
      h.length + sizeof(h) != f.size())
    return false;
  raw = f.readString();
  rev = h.revision;
  return raw.length() == h.length && checksum(raw.c_str(), raw.length()) == h.hash &&
         validate(raw, candidate);
}
bool save(const String &raw) {
  if (!fsReady || !validate(raw, candidate))
    return false;
  int target = activeSlot == 0 ? 1 : 0;
  Header h{Magic, revision + 1, (uint32_t)raw.length(), checksum(raw.c_str(), raw.length())};
  File f = LittleFS.open(slot(target), "w");
  if (!f)
    return false;
  bool ok = f.write((uint8_t *)&h, sizeof(h)) == sizeof(h) &&
            f.write((const uint8_t *)raw.c_str(), raw.length()) == raw.length();
  f.flush();
  f.close();
  String check;
  uint32_t rev = 0;
  if (!ok || !readSlot(target, check, rev) || check != raw || rev != h.revision) {
    LittleFS.remove(slot(target));
    return false;
  }
  current = raw;
  profile = candidate;
  revision = rev;
  activeSlot = target;
  applied = true;
  return true;
}
void send(JsonDocument &d) {
  serializeJson(d, Serial);
  Serial.println();
}
void handle(const String &input, bool connected, bool ready, uint8_t layer) {
  static StaticJsonDocument<4096> req;
  static StaticJsonDocument<4096> res;
  req.clear();
  res.clear();
  if (deserializeJson(req, input)) {
    res["id"] = 0;
    res["ok"] = false;
    res["error"] = "Messaggio JSON non valido";
    send(res);
    return;
  }
  res["id"] = req["id"];
  res["ok"] = false;
  const char *op = req["op"] | "";
  auto fail = [&](const char *msg) {
    res["error"] = msg;
    send(res);
  };
  if (!req["id"].is<uint32_t>() || !req["v"].is<int>() || req["v"].as<int>() != 1) {
    fail("Protocollo incompatibile");
    return;
  }
  auto data = res.createNestedObject("data");
  if (!strcmp(op, "hello")) {
    data["device"] = "NOVE MacroPad";
    data["protocol"] = 1;
    data["firmware"] = "studio-1.1.0-ble-recovery.8";
    data["maxBytes"] = MaxBytes;
    data["storage"] = fsReady;
  } else if (!strcmp(op, "oled_recover")) {
    data["recovered"]=display::recover();
  } else if (!strcmp(op, "ble_diag")) {
    ble_diagnostic::snapshot(data);
  } else if (!strcmp(op, "oled_diag")) {
    display::diagnostic(data);
  } else if (!strcmp(op, "status")) {
    data["connected"] = connected;
    data["ready"] = ready;
    data["layer"] = layer;
    data["testing"] = test;
    data["revision"] = revision;
  } else if (!strcmp(op, "get")) {
    if (!number(req["offset"], 0, current.length())) {
      fail("Offset non valido");
      return;
    }
    size_t offset = req["offset"], end = min(offset + 512, current.length());
    while (end > offset && end < current.length() && ((uint8_t)current[end] & 0xc0) == 0x80)
      --end;
    data["chunk"] = current.substring(offset, end);
    data["total"] = current.length();
    data["revision"] = revision;
  } else if (!strcmp(op, "begin")) {
    if (!fsReady) {
      fail("Archivio non disponibile");
      return;
    }
    if (!req["revision"].is<uint32_t>() || req["revision"].as<uint32_t>() != revision) {
      fail("Profilo cambiato: rileggere il dispositivo");
      return;
    }
    incoming = "";
    uploading = true;
    uploadAt = millis();
  } else if (!strcmp(op, "chunk")) {
    if (!uploading || !req["chunk"].is<const char *>() || !number(req["offset"], 0, MaxBytes) ||
        req["offset"].as<size_t>() != incoming.length()) {
      fail("Trasferimento non valido");
      return;
    }
    String chunk = req["chunk"].as<String>();
    if (chunk.length() > 768 || incoming.length() + chunk.length() > MaxBytes) {
      uploading = false;
      incoming = "";
      fail("Configurazione troppo grande");
      return;
    }
    incoming += chunk;
    uploadAt = millis();
    data["offset"] = incoming.length();
  } else if (!strcmp(op, "commit")) {
    if (!uploading) {
      fail("Nessun trasferimento attivo");
      return;
    }
    uploading = false;
    if (!save(incoming)) {
      incoming = "";
      fail("Configurazione non valida o salvataggio fallito");
      return;
    }
    incoming = "";
    data["revision"] = revision;
  } else if (!strcmp(op, "test")) {
    if (!req["enabled"].is<bool>()) {
      fail("Stato prova non valido");
      return;
    }
    test = req["enabled"];
    heartbeat = millis();
    data["testing"] = test;
  } else if (!strcmp(op, "heartbeat")) {
    heartbeat = millis();
    data["testing"] = test;
  } else {
    fail("Operazione sconosciuta");
    return;
  }
  res["ok"] = true;
  send(res);
}
} // namespace
void begin() {
  current = kStudioDefaults;
  validate(current, profile);
  Preferences marker;
  marker.begin("studiofs", false);
  bool initialized = marker.getBool("initialized", false);
  fsReady = LittleFS.begin(false);
  if (!fsReady && !initialized) {
    fsReady = LittleFS.format() && LittleFS.begin(false);
  }
  if (fsReady)
    marker.putBool("initialized", true);
  marker.end();
  if (fsReady) {
    String raw;
    uint32_t rev = 0;
    for (int i = 0; i < 2; i++)
      if (readSlot(i, raw, rev) && (activeSlot < 0 || rev > revision)) {
        activeSlot = i;
        revision = rev;
        current = raw;
        profile = candidate;
      }
  }
  line.reserve(2048);
  incoming.reserve(MaxBytes);
}
void poll(bool connected, bool ready, uint8_t layer) {
  uint32_t now = millis();
  if (test && now - heartbeat >= 3000)
    test = false;
  if (uploading && now - uploadAt > 10000) {
    uploading = false;
    incoming = "";
  }
  // Bounded work per loop so incomplete or malicious messages cannot stop scanning.
  for (int i = 0; i < 256 && Serial.available(); i++) {
    char c = Serial.read();
    if (c == '\r')
      continue;
    if (c == '\n') {
      if (!dropping && line.length())
        handle(line, connected, ready, layer);
      line = "";
      dropping = false;
    } else if (!dropping) {
      if (line.length() >= 2048) {
        dropping = true;
        line = "";
      } else
        line += c;
    }
  }
}
bool takeApplied() {
  bool value = applied;
  applied = false;
  return value;
}
bool testing() { return test; }
void event(const char *control, int value) {
  if (!test)
    return;
  StaticJsonDocument<192> d;
  d["event"] = "input";
  d["control"] = control;
  d["value"] = value;
  send(d);
}
} // namespace studio
