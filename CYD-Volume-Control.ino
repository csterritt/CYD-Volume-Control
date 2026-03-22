/*******************************************************************
 Volume Control for ESP32 Cheap Yellow Display
 Sends HTTP POST requests to a web server to adjust volume.
 Set WIFI_SSID, WIFI_PASSWORD, and WEB_SERVER_ADDRESS in
 common_definitions.h before flashing.
 *******************************************************************/

#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>
#include <JsmnStream.h>

#include "common_definitions.h"
#include "ui_elements.h"
#include "wifi_http.h"
#include "weather_data.h"
#include "weather_display.h"

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

// Screen mode
enum ScreenMode {
  MODE_VOLUME,
  MODE_WEATHER
};

// Screen power management / mode switching
const unsigned long SCREEN_TIMEOUT = 30000;     // 30 seconds
const unsigned long WEATHER_REFRESH_INTERVAL = 30000;  // 30 seconds
unsigned long lastTouchTime = 0;
unsigned long lastWeatherRefresh = 0;
bool displayOn = true;
ScreenMode screenMode = MODE_VOLUME;

// Weather data
static const size_t WEATHER_BUF_SIZE = 2048;
static char weatherBuf[WEATHER_BUF_SIZE];
WeatherData weatherData;

// State tracking
ButtonId activeButton        = BTN_NONE;
unsigned long pressStartTime = 0;
unsigned long lastRepeatTime = 0;
bool initialCommandSent      = false;
bool repeatStarted           = false;
bool wasConnected            = false;

// Forward declarations
void drawConnectionStatus(bool connected);
void refreshWeatherDisplay();
void switchToWeatherMode();
void switchToVolumeMode();
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
  // Draw buttons and initial status before blocking WiFi connect
  drawAllButtons();
  drawConnectionStatus(false);
  // Initialize screen power management
  lastTouchTime = millis();
  // WiFi setup
  initializeWiFi();
  drawConnectionStatus(isWiFiConnected());
  Serial.println("Volume Control ready!");
}

void loop() {
  // Update WiFi connection status indicator when state changes (volume mode only)
  if (screenMode == MODE_VOLUME) {
    bool connected = isWiFiConnected();
    if (connected != wasConnected) {
      wasConnected = connected;
      drawConnectionStatus(connected);
    }
  }

  // Switch to weather mode after timeout
  if (screenMode == MODE_VOLUME && (millis() - lastTouchTime > SCREEN_TIMEOUT)) {
    switchToWeatherMode();
  }

  updateTouch();

  if (screenMode == MODE_WEATHER) {
    // Any touch switches back to volume control
    if (touch.justPressed) {
      switchToVolumeMode();
      return;
    }
    // Periodic refresh — re-fetch data and redraw (handles night-mode transitions too)
    if (millis() - lastWeatherRefresh >= WEATHER_REFRESH_INTERVAL) {
      refreshWeatherDisplay();
    }
    delay(20);
    return;
  }

  // Volume mode — update touch time and handle buttons
  if (touch.justPressed) {
    lastTouchTime = millis();
  }

  ButtonId currentBtn = getActiveButton();
  if (touch.justPressed && currentBtn != BTN_NONE) {
    handleButtonPress(currentBtn);
  } else if (touch.isPressed && activeButton != BTN_NONE) {
    if (currentBtn == activeButton) {
      handleButtonHeld(activeButton);
    }
  }

  if (touch.justReleased && activeButton != BTN_NONE) {
    handleButtonRelease(activeButton);
  }

  delay(20);
}

// Draw a small WiFi status label in the strip below the buttons
void drawConnectionStatus(bool connected) {
  tft.fillRect(0, 226, 320, 14, SCREEN_BG);
  tft.setTextColor(connected ? 0x07E0 : 0xF800, SCREEN_BG);
  tft.drawCentreString(connected ? "WiFi: CONNECTED" : "WiFi: WAITING...", 160, 228, 1);
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

// Dispatch the appropriate HTTP command for a button
void sendCommandForButton(ButtonId btn) {
  switch (btn) {
    case BTN_VOL_UP:   sendVolumeUp();   break;
    case BTN_MUTE:     sendMute();       break;
    case BTN_VOL_DOWN: sendVolumeDown(); break;
    default: break;
  }
}

// Returns true if hour (0-23) falls in the night blackout window (22:00 – 05:59)
static bool isNightHour(int hour) {
  return hour >= 22 || hour < 6;
}

// Fetch weather, apply night-mode logic, and draw
void refreshWeatherDisplay() {
  lastWeatherRefresh = millis();
  memset(&weatherData, 0, sizeof(weatherData));
  if (fetchWeather(weatherBuf, WEATHER_BUF_SIZE)) {
    parseWeatherJson(weatherBuf, weatherData);
  }
  // Determine hour from parsed timestamp
  int currentHour = -1;
  if (weatherData.valid) {
    const char* tPtr = strchr(weatherData.current.time, 'T');
    if (tPtr != nullptr) {
      currentHour = atoi(tPtr + 1);
    }
  }
  if (currentHour >= 0 && isNightHour(currentHour)) {
    if (displayOn) {
      tft.fillScreen(TFT_BLACK);
      digitalWrite(21, LOW);   // backlight off
      displayOn = false;
      Serial.println("Night mode: display off");
    }
  } else {
    if (!displayOn) {
      digitalWrite(21, HIGH);  // backlight on
      displayOn = true;
      Serial.println("Day mode: display on");
    }
    drawWeatherScreen(weatherData);
  }
}

// Switch to weather display mode — fetch data and draw
void switchToWeatherMode() {
  screenMode = MODE_WEATHER;
  Serial.println("Switching to weather mode");
  refreshWeatherDisplay();
}

// Switch back to volume control mode
void switchToVolumeMode() {
  screenMode = MODE_VOLUME;
  lastTouchTime = millis();
  activeButton = BTN_NONE;
  repeatStarted = false;
  initialCommandSent = false;
  drawAllButtons();
  drawConnectionStatus(isWiFiConnected());
  Serial.println("Switching to volume mode");
}
