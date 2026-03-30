# Test Planning

## No Existing Tests
This project currently has no automated test infrastructure.

## Tests Needed for OKLCH Color Cycling

1. **Unit test for daytime detection**
   - Verify `calculateDaytimeHue()` returns -1 outside 6 AM - 9 PM
   - Verify it returns valid hue (0-360) within daytime hours
   - Test boundary conditions: 6:00, 5:59, 21:00, 20:59

2. **Unit test for hue calculation**
   - At 6:00 (0 min since 6 AM): hue should be 0° (red)
   - At 6:05 (5 min since 6 AM): hue should be 2°
   - At 12:00 (360 min since 6 AM): hue should be 144°
   - At 21:00 - 5 min (895 min since 6 AM): hue should be near 358°
   - At 21:00 (900 min since 6 AM): hue should be 360° → 0° (wraps)

3. **Unit test for OKLCH to RGB565 conversion**
   - L=0.7, C=0.2, h=0° should produce red-ish color
   - L=0.7, C=0.2, h=120° should produce green-ish color
   - L=0.7, C=0.2, h=240° should produce blue-ish color
   - Verify output is valid RGB565 (0-65535 range)

4. **Integration test for color application**
   - Verify text color changes during 6 AM - 9 PM
   - Verify white/gray colors used outside daytime
   - Background remains black in all cases

## Test Infrastructure to Add (if desired)
- PlatformIO test framework or Arduino-Unit for ESP32
- Mock `tft` object for isolated testing
- CI pipeline to run tests on push
