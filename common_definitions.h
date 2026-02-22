#ifndef COMMON_DEFINITIONS_H
#define COMMON_DEFINITIONS_H

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// Button colors
#define BTN_BG       0x441F  // Medium blue (R:66, G:129, B:255)
#define BTN_FG       0x0000  // Black
#define SCREEN_BG    0x0000  // Black screen background

// Touch handling
struct TouchState {
  bool wasPressed = false;
  bool isPressed = false;
  bool justPressed = false;
  bool justReleased = false;
  int x = 0, y = 0;
};

// Button identifiers
enum ButtonId {
  BTN_NONE = -1,
  BTN_VOL_UP = 0,
  BTN_MUTE = 1,
  BTN_VOL_DOWN = 2
};

// Button geometry
struct ButtonGeometry {
  int x;
  int y;
  int w;
  int h;
  const char* label;
  ButtonId id;
};

// Layout constants (320x240 landscape, 3 equal buttons)
const int NUM_BUTTONS = 3;
const int BTN_WIDTH = 90;
const int BTN_HEIGHT = 210;
const int BTN_Y = 15;
const int BTN_GAP = 10;
const int BTN_START_X = 15;

// Global objects - declared in main file
extern TFT_eSPI tft;
extern XPT2046_Touchscreen ts;
extern TouchState touch;
extern ButtonGeometry buttons[];

#endif
