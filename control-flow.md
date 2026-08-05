# CYD-Volume-Control: Mode Transition Flow

*2026-08-05T15:23:36Z by Showboat 0.6.1*
<!-- showboat-id: 42d23a6c-ffc3-46a7-9173-09cdd7fd3417 -->

This document traces how CYD-Volume-Control.ino transitions between its screen and night modes. The firmware manages two orthogonal pieces of state: a ScreenMode (VOLUME vs WEATHER) and a NightModeState (OFF, WEATHER, VOLUME). The loop() function is the central dispatcher; transitions are triggered by timeouts, touch events, and the current hour derived from weather data.

## 1. State variables

Two enums and a handful of timing globals define every mode the device can be in. screenMode selects what is drawn on the TFT; nightModeState is only meaningful when isNightHour(currentHour) is true and gates whether the backlight is on.

```bash
sed -n '49,73p' CYD-Volume-Control.ino
```

```output
// Screen mode
enum ScreenMode {
  MODE_VOLUME,
  MODE_WEATHER
};

// Night mode state
enum NightModeState {
  NIGHT_OFF,
  NIGHT_WEATHER,
  NIGHT_VOLUME
};

// Screen power management / mode switching
const unsigned long SCREEN_TIMEOUT = 30000;     // 30 seconds
const unsigned long WEATHER_REFRESH_INTERVAL = 30000;  // 30 seconds
const unsigned long NIGHT_TIMEOUT = 15000;      // 15 seconds for night mode
const unsigned long NIGHT_MODE_DEBOUNCE = 500;  // 500ms debounce after night mode transition
unsigned long lastTouchTime = 0;
unsigned long lastWeatherRefresh = 0;
unsigned long nightModeLastTouchTime = 0;
unsigned long lastNightModeTransition = 0;
bool displayOn = true;
ScreenMode screenMode = MODE_VOLUME;
NightModeState nightModeState = NIGHT_OFF;
```

## 2. setup() — initial state

setup() boots into MODE_VOLUME with the backlight on. It draws the volume buttons and connection status, seeds lastTouchTime (so the 30s screen timeout starts counting), then blocks on initializeWiFi(). No mode transitions happen here; the device always starts in volume mode.

```bash
sed -n '104,128p' CYD-Volume-Control.ino
```

```output
void setup() {
  Serial.begin(115200);

  // Initialize random seed from (unconnected) analog pin 21
  randomSeed(analogRead(21));

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
```

## 3. loop() — the dispatcher

loop() runs four logical phases every iteration:

1. **Maintenance**: maintainWiFiConnection() and maintainWeatherConnection() keep the network and weather data alive.
2. **Night detection**: currentHour is parsed from weatherData.timestamp; isNightHour() decides whether the night-mode state machine is active.
3. **Night-mode state machine** (only if isNight): handles 15s timeout, touch-driven transitions, and short-circuits the rest of the loop with early returns.
4. **Day-mode flow**: WiFi status indicator, 30s screen timeout → weather mode, touch handling, and button repeat logic.

The early returns inside the night-mode branch and inside MODE_WEATHER are the key control-flow features: they prevent the day-mode button logic from running when it should not.

```bash
sed -n '130,201p' CYD-Volume-Control.ino
```

```output
void loop() {
  // Keep WiFi alive — reconnects automatically every 30s if disconnected
  maintainWiFiConnection();

  // Keep weather data alive — refetches automatically every 30s if invalid
  maintainWeatherConnection();

  // Get current hour for night mode detection
  int currentHour = -1;
  if (weatherData.valid) {
    const char* tPtr = strchr(weatherData.timestamp, 'T');
    if (tPtr != nullptr) {
      currentHour = atoi(tPtr + 1);
    }
  }

  // Night mode state machine
  bool isNight = (currentHour >= 0 && isNightHour(currentHour));

  if (!isNight) {
    // Daytime: ensure backlight is on and reset night mode state
    if (!displayOn) {
      digitalWrite(21, HIGH);
      displayOn = true;
    }
    if (nightModeState != NIGHT_OFF) {
      nightModeState = NIGHT_OFF;
    }
  } else {
    // Night mode: handle state machine
    // Handle 15-second timeout
    if ((nightModeState == NIGHT_WEATHER || nightModeState == NIGHT_VOLUME) &&
        (millis() - nightModeLastTouchTime > NIGHT_TIMEOUT)) {
      nightModeState = NIGHT_OFF;
      tft.fillScreen(SCREEN_BG);
      digitalWrite(21, LOW);
      displayOn = false;
      Serial.println("Night mode timeout: backlight off");
    }

    // Handle touch transitions
    if (touch.justPressed && (millis() - lastNightModeTransition > NIGHT_MODE_DEBOUNCE)) {
      nightModeLastTouchTime = millis();
      if (nightModeState == NIGHT_OFF) {
        nightModeState = NIGHT_WEATHER;
        lastNightModeTransition = millis();
        digitalWrite(21, HIGH);
        displayOn = true;
        switchToWeatherMode();
        return;
      } else if (nightModeState == NIGHT_WEATHER) {
        nightModeState = NIGHT_VOLUME;
        lastNightModeTransition = millis();
        switchToVolumeMode();
        return;
      } else if (nightModeState == NIGHT_VOLUME) {
        // Stay in volume mode, just update touch time
        lastNightModeTransition = millis();
      }
    }

    // If in NIGHT_OFF state, skip rest of loop but still refresh weather periodically
    if (nightModeState == NIGHT_OFF) {
      updateTouch();
      // Periodic weather refresh to detect time changes (e.g., 6am transition)
      if (millis() - lastWeatherRefresh >= WEATHER_REFRESH_INTERVAL) {
        refreshWeatherDisplay();
      }
      delay(20);
      return;
    }
  }
```

```bash
sed -n '203,252p' CYD-Volume-Control.ino
```

```output
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
```

## 4. Day-mode transitions

During the day (isNight == false), the night-mode state machine is bypassed and only ScreenMode matters. Two transitions exist:

- **VOLUME → WEATHER**: triggered automatically when millis() - lastTouchTime exceeds SCREEN_TIMEOUT (30s of inactivity). switchToWeatherMode() is called and the loop continues into the MODE_WEATHER branch, which returns early.
- **WEATHER → VOLUME**: triggered by any touch (touch.justPressed). switchToVolumeMode() is called and loop() returns immediately.

lastTouchTime is bumped on every press in volume mode, so active button use keeps the device on the volume screen.

## 5. Night-mode state machine

When isNightHour(currentHour) is true, the night-mode state machine takes over and the day-mode timeout logic is effectively shadowed. The three states form a linear progression driven by touches, with a 500ms debounce (NIGHT_MODE_DEBOUNCE) between transitions and a 15s inactivity timeout (NIGHT_TIMEOUT) that returns the device to NIGHT_OFF.

- **NIGHT_OFF**: backlight off, screen cleared. A touch wakes the device into NIGHT_WEATHER via switchToWeatherMode(), then returns.
- **NIGHT_WEATHER**: weather screen with red text. A touch advances to NIGHT_VOLUME via switchToVolumeMode(), then returns.
- **NIGHT_VOLUME**: volume buttons active. A touch keeps the device here (only updates nightModeLastTouchTime). 15s of inactivity in NIGHT_WEATHER or NIGHT_VOLUME drops back to NIGHT_OFF.

While in NIGHT_OFF at night, loop() still calls updateTouch() and periodically refreshes weather (every WEATHER_REFRESH_INTERVAL) so the device can detect the 6am transition back to daytime.

## 6. Transition helpers

switchToWeatherMode() and switchToVolumeMode() are the only functions that mutate screenMode. Both also re-derive currentHour and, if it is night, set the corresponding nightModeState and reset nightModeLastTouchTime. This keeps the two state machines in sync: entering weather mode at night also enters NIGHT_WEATHER, and entering volume mode at night also enters NIGHT_VOLUME.

```bash
sed -n '376,419p' CYD-Volume-Control.ino
```

```output
// Switch to weather display mode — fetch data and draw
void switchToWeatherMode() {
  screenMode = MODE_WEATHER;
  Serial.println("Switching to weather mode");
  refreshWeatherDisplay();

  // Update night mode state if at night
  int currentHour = -1;
  if (weatherData.valid) {
    const char* tPtr = strchr(weatherData.timestamp, 'T');
    if (tPtr != nullptr) {
      currentHour = atoi(tPtr + 1);
    }
  }
  if (currentHour >= 0 && isNightHour(currentHour)) {
    nightModeState = NIGHT_WEATHER;
    nightModeLastTouchTime = millis();
  }
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

  // Update night mode state if at night
  int currentHour = -1;
  if (weatherData.valid) {
    const char* tPtr = strchr(weatherData.timestamp, 'T');
    if (tPtr != nullptr) {
      currentHour = atoi(tPtr + 1);
    }
  }
  if (currentHour >= 0 && isNightHour(currentHour)) {
    nightModeState = NIGHT_VOLUME;
    nightModeLastTouchTime = millis();
  }
}
```

## 7. refreshWeatherDisplay() — the night-mode enforcer

refreshWeatherDisplay() is called from switchToWeatherMode(), from the periodic refresh in both MODE_WEATHER and NIGHT_OFF, and from the day-mode weather branch. It is the single place that turns the backlight on/off based on time of day:

- **At night, NIGHT_OFF, backlight on**: clears the screen and turns the backlight off (this is how the device enters NIGHT_OFF after the 6am→sunset transition is detected via a weather refresh).
- **At night, NIGHT_WEATHER**: draws the weather screen with red text.
- **At night, NIGHT_VOLUME**: does nothing — volume mode owns the screen.
- **Daytime, backlight off**: turns the backlight back on and draws the weather screen.

This means weather refreshes are not just data updates; they are also the mechanism that flips the device into and out of night mode as the hour changes.

```bash
sed -n '314,349p' CYD-Volume-Control.ino
```

```output
// Fetch weather, apply night-mode logic, and draw
void refreshWeatherDisplay() {
  lastWeatherRefresh = millis();
  if (fetchWeather(weatherBuf, WEATHER_BUF_SIZE)) {
    parseWeatherJson(weatherBuf, weatherData);
  }
  // Determine hour from parsed timestamp
  int currentHour = -1;
  if (weatherData.valid) {
    const char* tPtr = strchr(weatherData.timestamp, 'T');
    if (tPtr != nullptr) {
      currentHour = atoi(tPtr + 1);
    }
  }
  if (currentHour >= 0 && isNightHour(currentHour)) {
    // At night: only turn off backlight if in NIGHT_OFF state
    if (nightModeState == NIGHT_OFF && displayOn) {
      tft.fillScreen(SCREEN_BG);
      digitalWrite(21, LOW);   // backlight off
      displayOn = false;
      Serial.println("Night mode: display off");
    } else if (nightModeState == NIGHT_WEATHER) {
      // Show weather with red text
      drawWeatherScreen(weatherData);
    }
    // In NIGHT_VOLUME state, don't draw weather (volume mode handles it)
  } else {
    // Daytime: ensure backlight is on and draw weather
    if (!displayOn) {
      digitalWrite(21, HIGH);  // backlight on
      displayOn = true;
      Serial.println("Day mode: display on");
    }
    drawWeatherScreen(weatherData);
  }
}
```

## 8. Button handling (volume mode only)

When the loop reaches the button-handling section, the device is in MODE_VOLUME and (during the day) MODE_VOLUME only — at night, NIGHT_VOLUME falls through to this same code. Three handlers cooperate:

- handleButtonPress: fires once on the rising edge; sends the command immediately and starts the repeat timer.
- handleButtonHeld: after REPEAT_INITIAL_DELAY (400ms) begins repeating every REPEAT_INTERVAL (100ms). Mute (BTN_MUTE) is excluded — it is a toggle, not a repeat.
- handleButtonRelease: restores the button visuals and clears activeButton.

These handlers never change screenMode or nightModeState; they only send HTTP commands. Mode transitions are exclusively the responsibility of the loop() dispatcher and the two switch* helpers.

```bash
sed -n '261,312p' CYD-Volume-Control.ino
```

```output
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
```

## 9. Summary of transitions

Day mode (isNight == false):
  MODE_VOLUME --30s idle--> MODE_WEATHER --any touch--> MODE_VOLUME
  nightModeState is forced to NIGHT_OFF and the backlight stays on.

Night mode (isNight == true):
  NIGHT_OFF (backlight off) --touch--> NIGHT_WEATHER --touch--> NIGHT_VOLUME --touch--> NIGHT_VOLUME
  NIGHT_WEATHER / NIGHT_VOLUME --15s idle--> NIGHT_OFF
  NIGHT_OFF still runs updateTouch() and periodic weather refreshes so it can detect the 6am transition back to day mode.

Cross-cutting:
  switchToWeatherMode() sets screenMode = MODE_WEATHER and, if at night, nightModeState = NIGHT_WEATHER.
  switchToVolumeMode() sets screenMode = MODE_VOLUME and, if at night, nightModeState = NIGHT_VOLUME.
  refreshWeatherDisplay() is the only function that turns the backlight off (entering night) or back on (entering day) based on the current hour.
