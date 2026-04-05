# Night Mode Implementation Plan

## Answer
Implement a state machine for nighttime operation (10pm-6am) with three states: backlight off, weather display, and volume control. Weather text uses red at night. Touch transitions between states, with 15-second inactivity returning to backlight-off state.

## Plan

### 1. Add night mode state tracking
- Add enum `NightModeState` with values: `NIGHT_OFF`, `NIGHT_WEATHER`, `NIGHT_VOLUME`
- Add global variable `NightModeState nightModeState = NIGHT_OFF`
- Add global variable `unsigned long nightModeLastTouchTime = 0`
- Add constant `const unsigned long NIGHT_TIMEOUT = 15000;` (15 seconds)

### 2. Modify weather_display.h
- Add constant `#define NIGHT_TEXT_COLOR 0xF800` (red)
- Modify `drawWeatherScreen()` to use `NIGHT_TEXT_COLOR` when `isNightHour(currentHour)` is true
- Update `calculateDaytimeHue()` to return -1 at night (already does this)

### 3. Modify CYD-Volume-Control.ino

#### Modify `refreshWeatherDisplay()`
- When at night and in `NIGHT_WEATHER` state, draw weather with red text
- When at night and in `NIGHT_OFF` state, turn off backlight (current behavior)
- When at night and in `NIGHT_VOLUME` state, don't call this function

#### Modify `loop()`
- Add night mode state machine logic before existing code:
  - Check if current hour is in night window (10pm-6am)
  - If not night: ensure backlight on, reset night state to NIGHT_OFF
  - If night:
    - Handle 15-second timeout: if in NIGHT_WEATHER or NIGHT_VOLUME and `millis() - nightModeLastTouchTime > NIGHT_TIMEOUT`, transition to NIGHT_OFF (backlight off)
    - Handle touch transitions:
      - If in NIGHT_OFF and touch detected: transition to NIGHT_WEATHER (backlight on, show weather)
      - If in NIGHT_WEATHER and touch detected: transition to NIGHT_VOLUME (show volume controls)
      - If in NIGHT_VOLUME and touch detected: stay in NIGHT_VOLUME (update touch time)
- Keep existing daytime behavior unchanged

#### Modify `switchToVolumeMode()`
- If at night, set `nightModeState = NIGHT_VOLUME` and update `nightModeLastTouchTime`
- Keep existing behavior

#### Modify `switchToWeatherMode()`
- If at night, set `nightModeState = NIGHT_WEATHER` and update `nightModeLastTouchTime`
- Keep existing behavior

### 4. Test scenarios
- Verify daytime behavior unchanged (30-second timeout to weather, normal colors)
- Verify night mode starts with backlight off
- Verify first touch at night shows weather with red text
- Verify second touch at night shows volume controls
- Verify 15-second inactivity at night turns off backlight
- Verify transitions work correctly when crossing hour boundaries

## Pitfalls
- **State confusion**: Ensure night mode state doesn't interfere with daytime `ScreenMode` enum. They are separate concerns.
- **Touch handling**: Night mode touch detection must not interfere with existing volume button touch handling.
- **Time boundary**: Ensure smooth transition when crossing 6am or 10pm - state should reset appropriately.
- **Backlight control**: Ensure backlight is only turned off at night, never during day.
- **Color application**: Red text should only apply to weather display, not volume buttons or other UI elements.
