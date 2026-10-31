#include "ble_hid.h"
#include "ble_recovery.h"
#include "hid_reports.h"
#include "config.h"
#include "logging.h"
#include <NimBLEDevice.h>
#include <atomic>
#include <esp_system.h>
#include <esp32s3/rom/rtc.h>

namespace {
std::atomic<bool> linked{false}, encrypted{false}, subscribedKeyboard{false},
    subscribedMedia{false};
std::atomic<bool> suspended{false}, notifyError{false};
std::atomic<uint32_t> generation{0};
std::atomic<uint16_t> handle{0xffff};
NimBLEServer *server = nullptr;
NimBLECharacteristic *keyboardInput = nullptr;
NimBLECharacteristic *consumerInput = nullptr;
bool idleRequested = false;
bool wasReady = false;


portMUX_TYPE diagnosticMux=portMUX_INITIALIZER_UNLOCKED;
struct BleEvent { uint32_t ms; uint8_t kind,flags; int value; };
BleEvent events[16]{};
uint32_t eventCount=0;
std::atomic<uint32_t> connections{0},disconnects{0},authEvents{0},subscribeEvents{0},controlEvents{0},notificationErrors{0};
std::atomic<uint32_t> lastReadyAt{0};
std::atomic<bool> authenticated{false},bonded{false};
uint32_t bootId=0;
uint8_t flags() { return (linked?1:0)|(encrypted?2:0)|(subscribedKeyboard?4:0)|(subscribedMedia?8:0)|(suspended?16:0); }
void note(uint8_t kind,int value=0) {
 BleEvent e{millis(),kind,flags(),value};
 portENTER_CRITICAL(&diagnosticMux);
 events[eventCount++%16]=e;
 portEXIT_CRITICAL(&diagnosticMux);
}
const char *resetName(esp_reset_reason_t reason) {
 switch(reason) {
 case ESP_RST_POWERON:return "power_on";
 case ESP_RST_EXT:return "external";
 case ESP_RST_SW:return "software";
 case ESP_RST_PANIC:return "panic";
 case ESP_RST_INT_WDT:return "interrupt_watchdog";
 case ESP_RST_TASK_WDT:return "task_watchdog";
 case ESP_RST_WDT:return "other_watchdog";
 case ESP_RST_DEEPSLEEP:return "deep_sleep";
 case ESP_RST_BROWNOUT:return "brownout";
 case ESP_RST_SDIO:return "sdio";
 default:return "unknown";
 }
}

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *s, ble_gap_conn_desc *desc) override {
    handle = desc->conn_handle;
    linked = true;
    encrypted = false;
    subscribedKeyboard = false;
    subscribedMedia = false;
    suspended = false;
    ++generation;
    authenticated=false;bonded=false;++connections;note(1);
    s->updateConnParams(desc->conn_handle, 12, 24, 0, 400);
    NimBLEDevice::startSecurity(desc->conn_handle);
  }
  void onDisconnect(NimBLEServer *) override {
    linked = false;
    encrypted = false;
    handle = 0xffff;
    subscribedKeyboard = false;
    subscribedMedia = false;
    ++generation;
    authenticated=false;bonded=false;++disconnects;note(2);
    NimBLEDevice::startAdvertising();
  }
  void onAuthenticationComplete(ble_gap_conn_desc *desc) override {
    encrypted = desc->sec_state.encrypted;
    authenticated=desc->sec_state.authenticated;bonded=desc->sec_state.bonded;
    ++authEvents;note(3, encrypted?1:0);
    if (!encrypted)
      server->disconnect(desc->conn_handle);
  }
} serverCallbacks;

class InputCallbacks : public NimBLECharacteristicCallbacks {
  void onSubscribe(NimBLECharacteristic *c, ble_gap_conn_desc *, uint16_t value) override {
    if (c == keyboardInput)
      subscribedKeyboard = (value & 1) != 0;
    if (c == consumerInput)
      subscribedMedia = (value & 1) != 0;
    ++subscribeEvents;note(c==keyboardInput?4:5,value);
    ++generation; // invalidate pending input when the host changes report subscriptions
  }
  void onStatus(NimBLECharacteristic *, Status status, int code) override {
    if (status != SUCCESS_NOTIFY) {
      ++notificationErrors;note(7,code);notifyError=true;
    }
  }
} inputCallbacks;
class ControlCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *c) override {
    const auto value = c->getValue();
    if (value.size() == 1 && (value[0] == 0 || value[0] == 1)) {
      suspended = value[0] == 0;
      ++controlEvents;note(6,static_cast<uint8_t>(value[0]));
      ++generation;
    }
  }
} controlCallbacks;

NimBLECharacteristic *report(NimBLEService *service, uint8_t id, uint8_t type, uint16_t size) {
  const uint32_t properties =
      type == 1 ? NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::NOTIFY
                : NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE |
                      NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::WRITE_ENC;
  auto *c = service->createCharacteristic(uint16_t(0x2A4D), properties, size);
  const uint8_t reference[] = {id, type};
  c->createDescriptor(uint16_t(0x2908), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC, 2)
      ->setValue(reference, sizeof(reference));
  return c;
}
} // namespace

void BleHid::begin() {
  bootId=esp_random();note(0,esp_reset_reason());
  NimBLEDevice::init(config::kName);
  NimBLEDevice::setSecurityAuth(true, false, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  server = NimBLEDevice::createServer();
  server->setCallbacks(&serverCallbacks, false);
  auto *service = server->createService(uint16_t(0x1812));
  const uint8_t info[] = {0x11, 0x01, 0x00, 0x02}; // HID 1.11, normally connectable
  service->createCharacteristic(uint16_t(0x2A4A), NIMBLE_PROPERTY::READ)
      ->setValue(info, sizeof(info));
  service->createCharacteristic(uint16_t(0x2A4B), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC)
      ->setValue(hid::kReportMap, sizeof(hid::kReportMap));
  auto *control = service->createCharacteristic(
      uint16_t(0x2A4C), NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::WRITE_ENC, 1);
  control->setCallbacks(&controlCallbacks);
  // Report-only HID: omit Protocol Mode and Boot characteristics together.
  keyboardInput = report(service, 1, 1, 8);
  consumerInput = report(service, 2, 1, 2);
  keyboardInput->setCallbacks(&inputCallbacks);
  consumerInput->setCallbacks(&inputCallbacks);
  const uint8_t led = 0;
  report(service, 1, 2, 1)->setValue(&led, 1);
  release();
  service->start();
  // Deliberately no Battery Service: no battery measurement exists in v1.
  auto *infoService = server->createService(uint16_t(0x180A));
  infoService->createCharacteristic(uint16_t(0x2A29), NIMBLE_PROPERTY::READ)
      ->setValue(std::string("99BIT"));
  infoService->createCharacteristic(uint16_t(0x2A24), NIMBLE_PROPERTY::READ)
      ->setValue(std::string("MacroPad prototype v1"));
  infoService->start();
  auto *adv = NimBLEDevice::getAdvertising();
  adv->setAppearance(0x03C1);
  adv->addServiceUUID(service->getUUID());
  adv->setScanResponse(true);
  adv->start();
  LOGF("[BLE] advertising\n");
}
bool BleHid::connected() const { return linked.load(); }
uint32_t BleHid::session() const { return generation.load(); }
bool BleHid::ready() const {
  return linked && encrypted && subscribedKeyboard && subscribedMedia && !suspended;
}
bool BleHid::resumeOnInput(bool input) {
  if (!ble_recovery::resume(input, linked, encrypted, subscribedKeyboard, subscribedMedia,
                            suspended))
    return false;
  note(9, 1);
  ++generation; // discard any input queued before host suspension
  LOGF("[BLE] resume on local input; wake gesture consumed\n");
  return true;
}
void BleHid::poll() {
  if (notifyError.exchange(false)) {
    LOGF("[BLE] notification failed; disconnect to clear host keys\n");
    if (linked)
      server->disconnect(handle.load());
  }
  const bool current = ready();
  if (current != wasReady) {
    if(current) lastReadyAt=millis();
    note(8,current?1:0);
    release(); // clear report values and notify neutral state on readiness changes
    LOGF("[BLE] %s\n", current ? "ready" : "not ready");
    wasReady = current;
  }
}
void BleHid::press(const core::Action &action) {
  if (!ready())
    return;
  if (action.kind == core::ActionKind::Keyboard) {
    uint8_t bytes[8];
    hid::keyboard(action, bytes);
    keyboardInput->setValue(bytes, sizeof(bytes));
    keyboardInput->notify();
  } else if (action.kind == core::ActionKind::Consumer) {
    uint8_t bytes[2];
    hid::consumer(action, bytes);
    consumerInput->setValue(bytes, sizeof(bytes));
    consumerInput->notify();
  }
}
void BleHid::release() {
  const uint8_t keyboard[8] = {}, media[2] = {};
  keyboardInput->setValue(keyboard, sizeof(keyboard));
  consumerInput->setValue(media, sizeof(media));
  if (linked && encrypted) {
    if (subscribedKeyboard)
      keyboardInput->notify();
    if (subscribedMedia)
      consumerInput->notify();
  }
}
void BleHid::setIdle(bool idle) {
  idleRequested = idle;
  if (linked)
    server->updateConnParams(handle.load(), 12, 24, idleRequested ? 4 : 0, 400);
}
void BleHid::forgetBonds() {
  if (linked)
    server->disconnect(handle.load());
  NimBLEDevice::deleteAllBonds();
  ++generation;
  LOGF("[BLE] bonds removed; forget device on Mac too\n");
}

void ble_diagnostic::snapshot(JsonObject d) {
 const int rawReset=rtc_get_reset_reason(0);
 d["resetRawCpu0"]=rawReset;d["resetRawCpu1"]=static_cast<int>(rtc_get_reset_reason(1));
 d["resetDetail"]=rawReset==USB_UART_CHIP_RESET?"usb_uart":rawReset==USB_JTAG_CHIP_RESET?"usb_jtag":rawReset==POWER_GLITCH_RESET?"power_glitch":rawReset==GLITCH_RTC_RESET?"clock_glitch":resetName(esp_reset_reason());
 d["bootId"]=bootId;d["uptimeMs"]=millis();
 d["resetReason"]=static_cast<int>(esp_reset_reason());d["resetName"]=resetName(esp_reset_reason());
 uint8_t f=flags();
 d["connected"]=bool(f&1);d["encrypted"]=bool(f&2);d["keyboardSubscribed"]=bool(f&4);d["mediaSubscribed"]=bool(f&8);d["suspended"]=bool(f&16);
 d["ready"]=(f&15)==15 && !(f&16);
 d["authenticated"]=authenticated.load();d["bonded"]=bonded.load();
 d["session"]=generation.load();d["lastReadyAt"]=lastReadyAt.load();
 d["connections"]=connections.load();d["disconnects"]=disconnects.load();d["authEvents"]=authEvents.load();d["subscribeEvents"]=subscribeEvents.load();d["controlEvents"]=controlEvents.load();d["notificationErrors"]=notificationErrors.load();
 auto missing=d.createNestedArray("waitReasons");
 if(!(f&1)) missing.add("disconnected");
 if(!(f&2)) missing.add("encryption");
 if(!(f&4)) missing.add("keyboard_subscription");
 if(!(f&8)) missing.add("media_subscription");
 if(f&16) missing.add("host_suspend");
 BleEvent copy[16];uint32_t count;
 portENTER_CRITICAL(&diagnosticMux);
 count=eventCount;for(unsigned i=0;i<16;i++)copy[i]=events[i];
 portEXIT_CRITICAL(&diagnosticMux);
 d["eventsSinceBoot"]=count;
 auto list=d.createNestedArray("events");
 for(uint32_t i=count>16?count-16:0;i<count;i++) {
  auto &e=copy[i%16];auto o=list.createNestedObject();o["ms"]=e.ms;o["kind"]=e.kind;o["flags"]=e.flags;o["value"]=e.value;
 }
}
