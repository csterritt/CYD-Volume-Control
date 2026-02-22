#ifndef HID_UTILS_H
#define HID_UTILS_H

#include "USB.h"
#include "USBHIDConsumerControl.h"

USBHIDConsumerControl ConsumerControl;

// Initialize USB HID — call in setup() before USB.begin()
void initializeHID() {
  ConsumerControl.begin();
  USB.begin();
}

// Send a single volume-up consumer control press
void sendVolumeUp() {
  ConsumerControl.press(CONSUMER_CONTROL_VOLUME_INCREMENT);
  ConsumerControl.release();
  Serial.println("VOL_UP");
}

// Send a single volume-down consumer control press
void sendVolumeDown() {
  ConsumerControl.press(CONSUMER_CONTROL_VOLUME_DECREMENT);
  ConsumerControl.release();
  Serial.println("VOL_DN");
}

// Send a single mute toggle consumer control press
void sendMute() {
  ConsumerControl.press(CONSUMER_CONTROL_MUTE);
  ConsumerControl.release();
  Serial.println("MUTE");
}

#endif
