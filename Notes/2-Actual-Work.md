This is a program that runs on a "CYD" (Cheap Yellow Display) ESP32 device.

The program is designed to control the volume of a connected computer.

We were not able to get the ESP32 to work with a Bluetooth keyboard interface, so we are
using a web server instead.

It will connect to a web server, and send HTTP requests to it to adjust the volume.

It will get the Wifi credentials from variables, WIFI_SSID and WIFI_PASSWORD,
and connect to the Wifi network.

The web server address is stored in a variable, WEB_SERVER_ADDRESS.

There are three buttons drawn on the screen:
- Volume up
- Mute
- Volume down

When the 'Volume up' button is pressed, it will send a HTTP POST request to the web server at
`http://WEB_SERVER_ADDRESS/api/v1/volume-up`.
When the 'Mute' button is pressed, it will send a HTTP POST request to the web server at
`http://WEB_SERVER_ADDRESS/api/v1/mute`.
When the 'Volume down' button is pressed, it will send a HTTP POST request to the web server at
`http://WEB_SERVER_ADDRESS/api/v1/volume-down`.
