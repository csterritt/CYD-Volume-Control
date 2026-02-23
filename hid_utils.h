#ifndef HID_UTILS_H
#define HID_UTILS_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <BLEHIDDevice.h>

// Consumer control HID report descriptor.
// Bits: 0=Volume Increment (0xE9), 1=Volume Decrement (0xEA), 2=Mute (0xE2),
//       3-7=padding to fill 1 byte.
static const uint8_t CONSUMER_REPORT_DESC[] = {
  0x05, 0x0C,        // Usage Page (Consumer)
  0x09, 0x01,        // Usage (Consumer Control)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x01,        //   Report ID (1)
  0x09, 0xE9,        //   Usage (Volume Increment)
  0x09, 0xEA,        //   Usage (Volume Decrement)
  0x09, 0xE2,        //   Usage (Mute)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1 bit)
  0x95, 0x03,        //   Report Count (3)
  0x81, 0x02,        //   Input (Data, Variable, Absolute)
  0x95, 0x05,        //   Report Count (5) — padding to byte boundary
  0x81, 0x03,        //   Input (Constant)
  0xC0               // End Collection
};

static BLEHIDDevice* hidDevice = nullptr;
static BLECharacteristic* inputReport = nullptr;
static bool hidConnected = false;

class HIDServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) override {
    hidConnected = true;
  }
  void onDisconnect(BLEServer* server) override {
    hidConnected = false;
    BLEDevice::startAdvertising();
  }
};

// Initialize BLE HID — call in setup()
void initializeHID() {
  BLEDevice::init("CYD Volume");
  // macOS requires bonding + encryption for BLE HID devices
  BLESecurity* security = new BLESecurity();
  security->setAuthenticationMode(ESP_LE_AUTH_BOND);
  security->setCapability(ESP_IO_CAP_NONE);
  security->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new HIDServerCallbacks());

  hidDevice = new BLEHIDDevice(server);
  hidDevice->manufacturer()->setValue("Espressif");
  hidDevice->pnp(0x02, 0x045E, 0x0000, 0x0110);
  hidDevice->hidInfo(0x00, 0x01);
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

// Send a 1-byte consumer control report then release
static void sendConsumerKey(uint8_t mask) {
  if (!hidConnected || !inputReport) return;
  inputReport->setValue(&mask, 1);
  inputReport->notify();
  delay(10);
  uint8_t release = 0x00;
  inputReport->setValue(&release, 1);
  inputReport->notify();
}

void sendVolumeUp() {
  sendConsumerKey(0x01);
  Serial.println("VOL_UP");
}

void sendVolumeDown() {
  sendConsumerKey(0x02);
  Serial.println("VOL_DN");
}

void sendMute() {
  sendConsumerKey(0x04);
  Serial.println("MUTE");
}

#endif
