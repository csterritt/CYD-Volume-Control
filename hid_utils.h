#ifndef HID_UTILS_H
#define HID_UTILS_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <BLEHIDDevice.h>

// Consumer control HID report descriptor.
// 16-bit array: sends one usage code at a time (0x0000 = release).
static const uint8_t CONSUMER_REPORT_DESC[] = {
  0x05, 0x0C,        // Usage Page (Consumer)
  0x09, 0x01,        // Usage (Consumer Control)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x01,        //   Report ID (1)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x03,  //   Logical Maximum (1023)
  0x19, 0x00,        //   Usage Minimum (0)
  0x2A, 0xFF, 0x03,  //   Usage Maximum (1023)
  0x75, 0x10,        //   Report Size (16 bits)
  0x95, 0x01,        //   Report Count (1)
  0x81, 0x00,        //   Input (Data, Array, Absolute)
  0xC0               // End Collection
};

static BLEHIDDevice* hidDevice = nullptr;
static BLECharacteristic* inputReport = nullptr;
static volatile bool hidConnected = false;

class HIDServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) override {
    hidConnected = true;
    Serial.println("HID: client connected");
  }
  void onDisconnect(BLEServer* server) override {
    hidConnected = false;
    Serial.println("HID: client disconnected, re-advertising");
    BLEDevice::startAdvertising();
  }
};

// Initialize BLE HID — call in setup()
void initializeHID() {
  BLEDevice::init("CYD Volume");
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new HIDServerCallbacks());

  hidDevice = new BLEHIDDevice(server);
  hidDevice->manufacturer()->setValue("Espressif");
  hidDevice->pnp(0x02, 0x045E, 0x0000, 0x0110);
  hidDevice->hidInfo(0x00, 0x01);
  hidDevice->setBatteryLevel(100);
  hidDevice->reportMap((uint8_t*)CONSUMER_REPORT_DESC, sizeof(CONSUMER_REPORT_DESC));
  inputReport = hidDevice->inputReport(1);
  hidDevice->startServices();

  BLEAdvertising* advertising = server->getAdvertising();
  advertising->setAppearance(HID_KEYBOARD);
  advertising->addServiceUUID(hidDevice->hidService()->getUUID());
  advertising->start();
  Serial.println("BLE HID advertising as 'CYD Volume'");
}

// Returns true when a host has connected and paired
bool isHIDConnected() {
  return hidConnected;
}

// Send a 16-bit consumer usage code then release (0x0000)
static void sendConsumerKey(uint16_t usageCode) {
  if (!inputReport) return;
  Serial.printf("HID send: connected=%d usage=0x%04X\n", (bool)hidConnected, usageCode);
  uint8_t report[2] = { (uint8_t)(usageCode & 0xFF), (uint8_t)(usageCode >> 8) };
  inputReport->setValue(report, 2);
  inputReport->notify();
  delay(10);
  uint8_t release[2] = {0x00, 0x00};
  inputReport->setValue(release, 2);
  inputReport->notify();
}

void sendVolumeUp() {
  sendConsumerKey(0x00E9);  // Volume Increment
  Serial.println("VOL_UP");
}

void sendVolumeDown() {
  sendConsumerKey(0x00EA);  // Volume Decrement
  Serial.println("VOL_DN");
}

void sendMute() {
  sendConsumerKey(0x00E2);  // Mute
  Serial.println("MUTE");
}

#endif
