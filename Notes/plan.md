# Weather Display Rearrangement Plan

## Goal
Rearrange `weather_display.h` so all content is left-justified and laid out as:

1. **Time** — HH:MM (24h), font 6, left edge, top of screen
2. **Date** — Month DD, font 4 (next smaller), below time
3. **Temp + Feels like** — `Temp: XX  Feels like: YY`, font 4, below date
4. **Condition + Hum + Wind** — `<condition>  Hum: XX%  Wind YY`, font 2, below temp
5. **Week summary** — all 7 days on one line: `Mo 72/55  Tu 68/50  ...`, font 2, below condition

## Screen dimensions
320 × 240 (landscape). Left margin: 4 px.

## Font sizes (TFT_eSPI built-in)
- Font 6: large digits/text (~52px tall) — used for time
- Font 4: medium (~26px tall) — used for date and temp line
- Font 2: small (~16px tall) — used for condition/hum/wind and week summary

## Y positions (approximate, leaving 4px gaps)
| Row | Font | Approx Y |
|-----|------|----------|
| Time | 6 | 4 |
| Date | 4 | 60 |
| Temp/Feels | 4 | 90 |
| Condition/Hum/Wind | 2 | 120 |
| Week summary | 2 | 140 |
| Touch hint | 1 | 225 |

## Weather condition mapping
Map `weather_code` to a short description string (WMO codes).

## Changes
- `weather_display.h`: full rewrite of `drawWeatherScreen()`
- No changes to `weather_data.h`, `CYD-Volume-Control.ino`, or any other file

## Tests
See `Notes/3-Plan-Tests.md` for the full manual on-device test plan.
