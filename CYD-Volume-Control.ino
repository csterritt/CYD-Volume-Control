/*******************************************************************
 Volume Control for ESP32 Cheap Yellow Display
 Sends BLE HID media key commands (Volume Up, Mute, Volume Down)
 via Bluetooth Low Energy keyboard emulation.
 *******************************************************************/

#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>

#include "common_definitions.h"
#include "ui_elements.h"
#include "hid_utils.h"

// Hardware setup — XPT2046 touchscreen on VSPI
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

// Global objects
SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();

// Touch state
TouchState touch;

// Button definitions — Volume Up | Mute | Volume Down (left to right)
ButtonGeometry buttons[NUM_BUTTONS] = {
  { BTN_START_X,                          BTN_Y, BTN_WIDTH, BTN_HEIGHT, "V +", BTN_VOL_UP   },
  { BTN_START_X + BTN_WIDTH + BTN_GAP,    BTN_Y, BTN_WIDTH, BTN_HEIGHT, "M", BTN_MUTE     },
  { BTN_START_X + 2*(BTN_WIDTH + BTN_GAP), BTN_Y, BTN_WIDTH, BTN_HEIGHT, "V -", BTN_VOL_DOWN }
};

// Repeat-while-held timing
const unsigned long REPEAT_INITIAL_DELAY = 400;  // ms before first repeat
const unsigned long REPEAT_INTERVAL      = 100;  // ms between subsequent repeats

// State tracking
ButtonId activeButton       = BTN_NONE;
unsigned long pressStartTime = 0;
unsigned long lastRepeatTime = 0;
bool initialCommandSent      = false;
bool repeatStarted           = false;
bool wasConnected            = false;

// Forward declaration
void drawConnectionStatus(bool connected);

// Forward declarations
void handleButtonPress(ButtonId btn);
void handleButtonHeld(ButtonId btn);
void handleButtonRelease(ButtonId btn);
void sendCommandForButton(ButtonId btn);

void setup() {
  Serial.begin(115200);
  // Touch setup
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(1);
  // Display setup
  tft.init();
  tft.setRotation(1);
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);
  // BLE HID setup
  initializeHID();
  Serial.println("BLE HID keyboard starting...");
  // Draw the three buttons and initial status
  drawAllButtons();
  drawConnectionStatus(false);
  Serial.println("Volume Control ready!");
}

void loop() {
  // Update BLE connection status indicator when state changes
  bool connected = isHIDConnected();
  if (connected != wasConnected) {
    wasConnected = connected;
    drawConnectionStatus(connected);
  }

  updateTouch();
  ButtonId currentBtn = getActiveButton();
  if (touch.justPressed && currentBtn != BTN_NONE) {
    handleButtonPress(currentBtn);
  } else if (touch.isPressed && activeButton != BTN_NONE) {
    // Finger may have slid off the original button
    if (currentBtn == activeButton) {
      handleButtonHeld(activeButton);
    }
  }

  if (touch.justReleased && activeButton != BTN_NONE) {
    handleButtonRelease(activeButton);
  }

  delay(20);
}

// Draw a small BLE status label in the strip below the buttons
void drawConnectionStatus(bool connected) {
  tft.fillRect(0, 226, 320, 14, SCREEN_BG);
  tft.setTextColor(connected ? 0x07E0 : 0xF800, SCREEN_BG);
  tft.drawCentreString(connected ? "BLE: CONNECTED" : "BLE: WAITING...", 160, 228, 1);
}

// Called once on the rising edge of a touch inside a button
void handleButtonPress(ButtonId btn) {
  activeButton = btn;
  pressStartTime = millis();
  lastRepeatTime = pressStartTime;
  initialCommandSent = true;
  repeatStarted = false;
  // Visual feedback — invert colors
  drawVolumeButton(buttons[btn], true);
  // Send the command immediately
  sendCommandForButton(btn);
}

// Called every loop iteration while the same button is held
void handleButtonHeld(ButtonId btn) {
  // Mute should not repeat — it is a toggle
  if (btn == BTN_MUTE) return;
  unsigned long now = millis();
  if (!repeatStarted) {
    // Waiting for initial delay before repeating
    if (now - pressStartTime >= REPEAT_INITIAL_DELAY) {
      repeatStarted = true;
      lastRepeatTime = now;
      sendCommandForButton(btn);
    }
  } else {
    // Repeating at interval
    if (now - lastRepeatTime >= REPEAT_INTERVAL) {
      lastRepeatTime = now;
      sendCommandForButton(btn);
    }
  }
}

// Called once when the finger lifts
void handleButtonRelease(ButtonId btn) {
  // Restore normal button appearance
  drawVolumeButton(buttons[btn], false);
  activeButton = BTN_NONE;
  initialCommandSent = false;
  repeatStarted = false;
}

// Dispatch the appropriate HID command for a button
void sendCommandForButton(ButtonId btn) {
  switch (btn) {
    case BTN_VOL_UP:   sendVolumeUp();   break;
    case BTN_MUTE:     sendMute();       break;
    case BTN_VOL_DOWN: sendVolumeDown(); break;
    default: break;
  }
}
