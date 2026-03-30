# Test Planning

## No Existing Tests
This project currently has no automated test infrastructure.

## Tests Needed for This Change

1. **Unit test for modulo condition**
   - Verify `minute % 5 == 0` correctly triggers at minutes 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55
   - Verify it does NOT trigger at other minute values

2. **Unit test for random range**
   - Mock `random()` calls to verify range is 4 to 141 (exclusive upper bound for Arduino)
   - Verify `top` is set within bounds [4, 140]

3. **Integration test**
   - Verify display positioning is correct with various `top` values (edge cases at 4 and 140)
   - Screen height is likely 240px, so 140 leaves room for text elements

## Test Infrastructure to Add (if desired)
- PlatformIO test framework or Arduino-Unit for ESP32
- Mock `random()` and `tft` objects for isolated testing
- CI pipeline to run tests on push
