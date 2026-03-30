# Plan: OKLCH Color Cycling for Daytime Text

## Goal
During hours between 6 AM and 9 PM, cycle text colors through the rainbow using OKLCH color space with constant luminance 0.7 and chroma 0.2. The hue cycles from 0° to 360° based on time of day, changing every 5 minutes.

## Implementation Steps

1. **Add OKLCH to RGB conversion functions**
   - Convert OKLCH → Oklab (L, C, h → L, a, b)
   - Convert Oklab → linear RGB
   - Apply gamma correction (linear → sRGB)
   - Convert sRGB to 16-bit RGB565 for TFT

2. **Add time-based hue calculation**
   - Calculate minutes since 6 AM (0 to 899)
   - Map to hue: (minutes / 5) * 2 degrees % 360
   - 15 hours = 180 5-minute intervals = 360° cycle

3. **Apply cycling color to text**
   - During 6 AM - 9 PM: use calculated color for text
   - Outside hours: use original colors (white/gray)
   - Background stays black

## Pitfalls

- OKLCH→RGB conversion requires math functions (sin, cos, pow) - may impact performance
- Gamma correction needed for perceptually accurate colors
- Must handle time boundary at midnight correctly
- TFT uses RGB565 format (5R-6G-5B), not RGB888
- Math operations on ESP32 are slower; consider lookup table if needed

## Files to Modify
- `/Users/chris/hacks/esp32/CYD-Volume-Control/weather_display.h` - Add conversion functions and color calculation
