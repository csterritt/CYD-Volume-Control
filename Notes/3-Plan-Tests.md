Plan tests, and modifications to existing tests as needed.

## Manual On-Device Tests

These are manual tests since the CYD has no test framework; verify by observation after flashing.

| # | Test | Steps | Expected Result |
|---|------|-------|----------------|
| 1 | Boot — volume UI visible | Power on device | Backlight on, three buttons (V+, M, V-) visible, WiFi status at bottom |
| 2 | Timeout → weather screen | Boot, wait 30 seconds without touching | Screen switches to weather display; timestamp shown in red top-left |
| 3 | Weather data displayed | As above, with server reachable | Current temp, feels-like, humidity, wind, today H/L all visible |
| 4 | No server → graceful fallback | Boot with server unreachable, wait 30s | Weather screen shows "No weather data" (no crash) |
| 5 | Touch returns to volume UI | While on weather screen, tap anywhere | Immediately returns to volume button UI |
| 6 | Timer resets on touch | Touch volume UI at 25s, then wait 30s | Timeout clock restarts from the touch; weather screen appears 30s after that touch |
| 7 | Volume Up sends POST | Press V+ button | HTTP POST to `/api/v1/volume-up`; button visually inverts |
| 8 | Volume Up repeat-hold | Press and hold V+ for >400ms | Repeated POSTs every 100ms while held |
| 9 | Mute fires once | Press and hold M | Single POST to `/api/v1/mute`; no repeat |
| 10 | Volume Down sends POST | Press V- button | HTTP POST to `/api/v1/volume-down` |
| 11 | WiFi disconnect recovery | Disconnect WiFi AP, press volume button | Serial logs "WiFi not connected, skipping POST"; no crash |
| 12 | Weather screen → volume → weather cycle | Cycle through the two modes multiple times | No flickering, no stale state, each mode redraws cleanly |

## Existing Behavior — No Regression

The volume button logic (press, hold-repeat, release) is unchanged from before. Tests 7–10 above cover regression on that path.
