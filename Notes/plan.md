# Plan: Random Top Position Every 5 Minutes

## Goal
When the minute extracted from the weather timestamp is divisible by 5 (e.g., :00, :05, :10, etc.), randomly set the `top` vertical position for the weather display between 4 and 140 pixels.

## Implementation Steps

1. **Add random include** - Add `#include <stdlib.h>` or Arduino's random function
2. **Modify drawWeatherScreen()** - After extracting `minute` on line 83, check if `minute % 5 == 0`
3. **Randomize top** - If condition met, set `top = random(4, 141)` (Arduino's random is exclusive of upper bound)
4. **Update global top** - The `top` variable is already a global at line 52, so modifications persist

## Pitfalls

- Arduino's `random(min, max)` is exclusive of max, so use 141 to include 140
- Ensure `randomSeed()` is called elsewhere in the app to avoid repeatable patterns
- The `top` value affects all text positioning; very low/high values may cause overlap or clipping
- No existing tests, so manual verification will be needed

## Files to Modify
- `/Users/chris/hacks/esp32/CYD-Volume-Control/weather_display.h` - Add modulo check and random assignment
