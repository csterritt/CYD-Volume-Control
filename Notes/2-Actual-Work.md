This is a program that runs on a "CYD" (Cheap Yellow Display) ESP32 device.

The program is designed to control the volume of a connected computer.

To do this, it will send keyboard commands to the computer to adjust the volume
using USB HID (Human Interface Device) protocol.

The USB connector of the CYD will be connected to the computer.

There will be three buttons drawn on the screen:
- Volume up
- Mute
- Volume down

In that order from left to right. The CYD will be in landscape mode. The buttons will be drawn
with a border and a label in black, and a background of medium blue.

When a user presses a button, the corresponding keyboard command will be sent to the computer.

While a button is being held down, the keyboard command should be sent repeatedly to the computer,
and the colors of the button should invert to indicate that it is being held down.

References:
- The files in /Users/chris/hacks/esp32/CYD-MIDI-Controller show how to set up:
  - The display
  - The buttons
  - The Arduino-style '.ino' file structure
- In particular, look at the `CODEBASE_KNOWLEDGE.md` file for information about the codebase,
  including a lot of useful functions and patterns.
