#include "ble_hid.h"
#include "hid_reports.h"
#include "config.h"
#include "logging.h"
#include <NimBLEDevice.h>
#include <atomic>

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

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *s, ble_gap_conn_desc *desc) override {
    handle = desc->conn_handle;
    linked = true;
    encrypted = false;
    subscribedKeyboard = false;
    subscribedMedia = false;
    suspended = false;
    ++generation;
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
    NimBLEDevice::startAdvertising();
  }
  void onAuthenticationComplete(ble_gap_conn_desc *desc) override {
    encrypted = desc->sec_state.encrypted;
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
    ++generation; // invalidate pending input when the host changes report subscriptions
  }
  void onStatus(NimBLECharacteristic *, Status status, int) override {
    if (status != SUCCESS_NOTIFY)
      notifyError = true;
  }
} inputCallbacks;
class ControlCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *c) override {
    const auto value = c->getValue();
    if (value.size() == 1) {
      suspended = value[0] == 0;
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
void BleHid::poll() {
  if (notifyError.exchange(false)) {
    LOGF("[BLE] notification failed; disconnect to clear host keys\n");
    if (linked)
      server->disconnect(handle.load());
  }
  const bool current = ready();
  if (current != wasReady) {
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
