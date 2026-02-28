# CYD Volume Control — Implementation Plan

## Assumptions

1. **Standard ESP32 CYD** — ILI9341 TFT + XPT2046 touchscreen, WiFi capable. BLE HID was attempted but abandoned; using WiFi + HTTP POST instead.
2. **WiFi credentials and server address** are supplied as compile-time constants: `WIFI_SSID`, `WIFI_PASSWORD`, `WEB_SERVER_ADDRESS`.
3. **Mute does not repeat** — Volume Up/Down repeat while held; Mute fires once on press.

## Answer

A single-purpose CYD app with three full-screen buttons (Volume Up, Mute, Volume Down). When a button is pressed, it sends an HTTP POST to a web server at `http://WEB_SERVER_ADDRESS/api/v1/<action>`. WiFi is connected at startup; the connection status is shown on screen.

## Plan

### File Structure

```
CYD-Volume-Control/
├── CYD-Volume-Control.ino   # Main entry: setup(), loop(), button logic
├── common_definitions.h      # TouchState struct, externs, color/layout constants
├── ui_elements.h             # updateTouch(), drawVolumeButton(), drawAllButtons()
├── wifi_http.h               # WiFi init, HTTP POST helpers, connection status
├── User_Setup.h              # TFT_eSPI pin config
└── Notes/
    └── plan.md               # This file
```

### Implementation Steps

1. **`common_definitions.h`** — Add `WIFI_SSID`, `WIFI_PASSWORD`, `WEB_SERVER_ADDRESS` constants. Keep `TouchState`, color constants, button geometry.
2. **`wifi_http.h`** (replaces `hid_utils.h`) — `initializeWiFi()` connects to WiFi, `isWiFiConnected()` returns status, `sendVolumeUp()` / `sendVolumeDown()` / `sendMute()` each do an HTTP POST to the appropriate endpoint.
3. **`ui_elements.h`** — No change needed; status label text updated in main sketch.
4. **`CYD-Volume-Control.ino`** — Remove BLE includes/calls, add WiFi init, update `drawConnectionStatus()` to show WiFi status, dispatch button presses via HTTP.

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

Manual testing on hardware with Serial debug output.

| # | Test | Expected Result |
|---|------|----------------|
| 1 | Power on, WiFi connects | Status bar shows "WiFi: CONNECTED" in green |
| 2 | Power on, WiFi unavailable | Status bar shows "WiFi: WAITING..." in red |
| 3 | Tap Volume Up | Button inverts, serial prints POST URL, server receives POST /api/v1/volume-up |
| 4 | Tap Volume Down | Button inverts, serial prints POST URL, server receives POST /api/v1/volume-down |
| 5 | Tap Mute | Button inverts once, server receives POST /api/v1/mute |
| 6 | Hold Volume Up | Volume-up POSTs repeat at ~100ms after 400ms delay |
| 7 | Hold Volume Down | Volume-down POSTs repeat similarly |
| 8 | Hold Mute | Only one POST sent; button stays inverted until release |
| 9 | Release any button | Button returns to normal colors |

## Pitfalls

1. **WiFi credentials** — Must be set as compile-time constants before flashing; no runtime config UI.
2. **HTTP blocking** — `HTTPClient::POST` blocks while waiting for server response. If the server is slow/down, the UI freezes. Consider a timeout (default Arduino HTTPClient timeout is 5s).
3. **Touch calibration** — ADC-to-pixel mapping constants may need tuning per unit.
4. **Mute repeat** — Single-fire by design; rapid toggle is undesirable.
5. **Server must accept POST with empty body** — The endpoints receive POST with no body; server should respond 200 OK.
