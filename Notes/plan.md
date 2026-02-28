# CYD Volume Control — Screen Power Management Implementation Plan

## Assumptions

1. **Standard ESP32 CYD** — ILI9341 TFT + XPT2046 touchscreen, WiFi capable. BLE HID was attempted but abandoned; using WiFi + HTTP POST instead.
2. **WiFi credentials and server address** are supplied as compile-time constants: `WIFI_SSID`, `WIFI_PASSWORD`, `WEB_SERVER_ADDRESS`.
3. **Mute does not repeat** — Volume Up/Down repeat while held; Mute fires once on press.
4. **Backlight control** — Pin 21 controls the screen backlight (already used in current setup).
5. **Screen blanking** — Clear screen to black when inactive for power savings.

## Answer

Add screen power management to the existing CYD volume control app. The screen backlight turns off and screen blanks after 30 seconds of inactivity. Touch wakes the screen, but requires a second touch to activate buttons.

## Plan

### Implementation Steps

1. **Add power management state tracking** — Add variables to `CYD-Volume-Control.ino`:
   - `lastTouchTime` to track last activity
   - `screenAwake` to track screen state
   - `firstTouchAfterWake` to handle two-touch requirement

2. **Add timeout constants** — Define 30-second timeout for screen power-off.

3. **Modify `loop()` function** — Add power management logic:
   - Check for inactivity timeout
   - Turn off backlight and blank screen when timeout reached
   - Handle wake-up on first touch
   - Require second touch for button activation

4. **Update touch handling** — Modify button press logic to respect the two-touch requirement after wake.

5. **Add screen redraw functions** — Functions to blank screen and restore UI when waking.

### Key Changes

#### New State Variables
```cpp
unsigned long lastTouchTime = 0;
bool screenAwake = true;
bool firstTouchAfterWake = false;
const unsigned long SCREEN_TIMEOUT = 30000; // 30 seconds
```

#### Modified Loop Logic
- Check `millis() - lastTouchTime > SCREEN_TIMEOUT` to power off screen
- On first touch after screen off: wake screen but don't activate buttons
- On second touch: allow normal button operation

#### Backlight Control
- `digitalWrite(21, HIGH)` for backlight on
- `digitalWrite(21, LOW)` for backlight off
- `tft.fillScreen(SCREEN_BG)` to blank/unblank screen

### Button Layout (320×240 landscape)

- **Volume Up**: (15, 15, 90, 210)
- **Mute**: (115, 15, 90, 210)
- **Volume Down**: (215, 15, 90, 210)

### HTTP Endpoints

- `POST http://<WEB_SERVER_ADDRESS>/api/v1/volume-up`
- `POST http://<WEB_SERVER_ADDRESS>/api/v1/mute`
- `POST http://<WEB_SERVER_ADDRESS>/api/v1/volume-down`

### Repeat Logic

- On press: send HTTP POST immediately
- While held (Vol Up/Down only): after 400ms initial delay, repeat every 100ms
- On release: stop repeating, restore button to normal colors

## Test Plan

| # | Test | Expected Result |
|---|------|----------------|
| 1 | Power on, screen active | Screen backlight on, buttons visible |
| 2 | No touch for 30 seconds | Backlight turns off, screen goes blank |
| 3 | Touch blank screen after timeout | Backlight turns on, screen redraws, buttons visible but inactive |
| 4 | Second touch after wake | Buttons respond normally to touches |
| 5 | Touch before 30-second timeout | Screen stays active, lastTouchTime updates |
| 6 | Rapid touches within 30 seconds | Screen stays active, buttons work normally |
| 7 | Button operation after wake | Volume Up/Down/Mute send HTTP POSTs as before |

## Pitfalls

1. **Touch detection during screen off** — Touchscreen may still register touches when backlight is off; need to handle wake properly.
2. **Screen redraw timing** — Must ensure screen is fully redrawn before accepting button input to avoid ghost touches.
3. **Power consumption** — Backlight control saves power but touchscreen still consumes power; consider deeper sleep if needed.
4. **WiFi connection during sleep** — WiFi may disconnect during long inactivity; current code doesn't handle reconnection.
5. **Touch debouncing** — Wake-up touch might be registered as button press; need careful state management.
