## This is a program that runs on a "CYD" (Cheap Yellow Display) ESP32 device.

The program is designed to control the volume of a connected computer.

To do this, it will contact a [server program](https://github.com/csterritt/volume-web) running on a mac computer. The server will adjust the volume. The server also retrieves weather information for the display.

There will be three buttons drawn on the screen:
- Volume up
- Mute
- Volume down

In that order from left to right. The CYD will be in landscape mode. The buttons will be drawn
with a border and a label in black, and a background of medium blue.

When a user presses a button, the corresponding keyboard command will be sent to the computer.

While a button is being held down, the keyboard command should be sent repeatedly to the computer,
and the colors of the button should invert to indicate that it is being held down.

After 30 seconds of no button presses, the display will switch to showing the current weather information. At night, the display will be turned off, but will turn on for 30 seconds after a touch event and allow adjusting the volume.

Inspiration, and code, from the
[CYD-MIDI-Controller](https://github.com/NickCulbertson/CYD-MIDI-Controller.git) project.

### Customization and build notes

Before building, create a `local_settings.h` file based on `local_settings.h.template` and fill
in your WiFi credentials and web server address.

This compiles with the [Arduino IDE](https://www.arduino.cc/en/software) and the
[ESP32 Board Support Package](https://docs.espressif.com/projects/arduino-esp32/en/latest/).

### License

It is released under the Mozilla Public License 2.0. See `LICENSE.txt` for details.
