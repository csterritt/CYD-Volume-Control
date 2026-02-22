# CYD Volume Control — Implementation Plan

## Assumptions

1. **ESP32-S3 CYD variant** — The standard CYD (ESP32-2432S028R) uses a CH340 USB-serial chip and cannot do native USB HID. Since the spec explicitly requires USB HID, this plan assumes an ESP32-S3 based CYD board that has native USB OTG support. If you have a standard ESP32 CYD, swap to the `ESP32-BLE-Keyboard` library for BLE HID instead.
2. **Same display/touch hardware** — ILI9341 TFT + XPT2046 touchscreen, same `User_Setup.h` pin config as the CYD-MIDI-Controller reference project. Adjust GPIO pins if your S3 board differs.
3. **Mute does not repeat** — Volume Up/Down repeat while held; Mute fires once on press (repeating mute would toggle rapidly, which is undesirable). Visual feedback still shows while Mute is held.

## Answer

A single-purpose CYD app with three full-screen buttons (Volume Up, Mute, Volume Down) that sends USB HID Consumer Control commands. Uses the ESP32-S3 `USBHIDConsumerControl` API for native USB keyboard media keys. Display uses TFT_eSPI + XPT2046 touchscreen, matching the reference project's hardware setup.

## Plan

### File Structure

```
CYD-Volume-Control/
├── CYD-Volume-Control.ino   # Main entry: setup(), loop(), button logic
├── common_definitions.h      # TouchState struct, externs, color constants
├── ui_elements.h             # updateTouch(), drawVolumeButton()
├── hid_utils.h               # USB HID consumer control wrappers
├── User_Setup.h              # TFT_eSPI pin config (from reference project)
└── Notes/
    └── plan.md               # This file
```

### Implementation Steps

1. **`common_definitions.h`** — Define `TouchState` struct, color constants (medium blue `0x441F`, black `0x0000`), extern declarations for `tft`, `ts`, `touch`.
2. **`ui_elements.h`** — `updateTouch()` (from reference), `drawVolumeButton()` with normal/pressed states, `isButtonPressed()` hit testing.
3. **`hid_utils.h`** — Wrap `USBHIDConsumerControl` with `sendVolumeUp()`, `sendVolumeDown()`, `sendMute()`.
4. **`User_Setup.h`** — Copy from reference project (identical CYD hardware).
5. **`CYD-Volume-Control.ino`** — Setup hardware (SPI, touch, display, USB HID), draw 3 buttons, main loop handles touch → command dispatch with repeat-while-held logic.

### Button Layout (320×240 landscape)

- **Volume Up**: (15, 15, 90, 210)
- **Mute**: (115, 15, 90, 210)
- **Volume Down**: (215, 15, 90, 210)

### Colors

- **Normal**: medium blue background (`0x441F`), black border, black label
- **Pressed**: black background, medium blue border, medium blue label (inverted)

### Repeat Logic

- On press: send command immediately, record timestamp
- While held (Vol Up/Down only): after 400ms initial delay, repeat every 100ms
- On release: stop repeating, restore button to normal colors

## Test Plan

Since this is an Arduino/ESP32 project, testing is manual on hardware with Serial debug output.

| # | Test | Expected Result |
|---|------|----------------|
| 1 | Power on | 3 buttons render correctly in landscape, medium blue with black labels |
| 2 | Tap Volume Up | Button inverts colors, serial prints "VOL_UP", host volume increases |
| 3 | Tap Volume Down | Button inverts colors, serial prints "VOL_DN", host volume decreases |
| 4 | Tap Mute | Button inverts colors, serial prints "MUTE", host mutes/unmutes |
| 5 | Hold Volume Up | Volume keeps increasing at ~100ms intervals after 400ms delay |
| 6 | Hold Volume Down | Volume keeps decreasing at ~100ms intervals after 400ms delay |
| 7 | Hold Mute | Mute fires once only, button stays inverted until release |
| 8 | Release any button | Button returns to normal colors, repeating stops |
| 9 | USB enumeration | Device appears as HID keyboard on host computer |

No existing tests to modify (new project).

## Pitfalls

1. **ESP32 vs ESP32-S3** — Standard ESP32 CYD cannot do USB HID. Must have S3 variant or switch to BLE HID.
2. **User_Setup.h GPIO pins** — S3 boards may have different pin assignments than standard CYD. Verify against your board's schematic.
3. **Touch calibration** — The ADC-to-pixel mapping constants (200, 3700, 240, 3800) are specific to the CYD touchscreen. May need adjustment per unit.
4. **Mute repeat** — Implemented as single-fire to avoid rapid toggle. Change if your OS handles mute-repeat gracefully.
5. **USB initialization timing** — `USB.begin()` must be called after `USBHIDConsumerControl.begin()`. The host may take 1-2 seconds to enumerate the device after boot.
