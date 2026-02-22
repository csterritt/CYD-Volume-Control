#ifndef UI_ELEMENTS_H
#define UI_ELEMENTS_H

#include "common_definitions.h"

// Function declarations
void updateTouch();
bool isButtonPressed(int bx, int by, int bw, int bh);
void drawVolumeButton(const ButtonGeometry& btn, bool pressed);
void drawAllButtons();
ButtonId getActiveButton();

// Touch update — reads XPT2046, computes edge states
void updateTouch() {
  touch.wasPressed = touch.isPressed;
  touch.isPressed = ts.tirqTouched() && ts.touched();
  touch.justPressed = touch.isPressed && !touch.wasPressed;
  touch.justReleased = !touch.isPressed && touch.wasPressed;
  if (touch.isPressed) {
    TS_Point p = ts.getPoint();
    touch.x = map(p.x, 200, 3700, 0, 320);
    touch.y = map(p.y, 240, 3800, 0, 240);
  }
}

// Rectangle hit test against current touch position
bool isButtonPressed(int bx, int by, int bw, int bh) {
  return touch.x >= bx && touch.x <= bx + bw &&
         touch.y >= by && touch.y <= by + bh;
}

// Draw a single volume button with normal or pressed (inverted) state
void drawVolumeButton(const ButtonGeometry& btn, bool pressed) {
  uint16_t bgColor = pressed ? BTN_FG : BTN_BG;
  uint16_t fgColor = pressed ? BTN_BG : BTN_FG;
  // Fill background
  tft.fillRect(btn.x + 2, btn.y + 2, btn.w - 4, btn.h - 4, bgColor);
  // Draw border (2px thick)
  tft.drawRect(btn.x, btn.y, btn.w, btn.h, fgColor);
  tft.drawRect(btn.x + 1, btn.y + 1, btn.w - 2, btn.h - 2, fgColor);
  // Draw label centered
  tft.setTextColor(fgColor, bgColor);
  tft.drawCentreString(btn.label, btn.x + btn.w / 2, btn.y + btn.h / 2 - 8, 4);
}

// Draw all three buttons in normal state
void drawAllButtons() {
  tft.fillScreen(SCREEN_BG);
  for (int i = 0; i < NUM_BUTTONS; i++) {
    drawVolumeButton(buttons[i], false);
  }
}

// Return which button is currently touched, or BTN_NONE
ButtonId getActiveButton() {
  if (!touch.isPressed) return BTN_NONE;
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (isButtonPressed(buttons[i].x, buttons[i].y, buttons[i].w, buttons[i].h)) {
      return buttons[i].id;
    }
  }
  return BTN_NONE;
}

#endif
