#ifndef WEATHER_DISPLAY_H
#define WEATHER_DISPLAY_H

#include <math.h>

#include "common_definitions.h"
#include "weather_data.h"

#define WEATHER_TIMESTAMP_COLOR 0xF800  // Red
#define WEATHER_TEXT_COLOR      0xFFFF  // White
#define WEATHER_DIM_COLOR       0x8410  // Gray

#define WEATHER_LEFT_MARGIN 4

// Map WMO weather interpretation code to a short description.
static const char* weatherCodeDescription(int code) {
  if (code == 0)                   return "Clear";
  if (code == 1)                   return "Mainly clear";
  if (code == 2)                   return "Partly cloudy";
  if (code == 3)                   return "Overcast";
  if (code <= 49)                  return "Foggy";
  if (code <= 55)                  return "Drizzle";
  if (code <= 57)                  return "Freezing drizzle";
  if (code <= 65)                  return "Rain";
  if (code <= 67)                  return "Freezing rain";
  if (code <= 77)                  return "Snow";
  if (code <= 82)                  return "Rain showers";
  if (code <= 86)                  return "Snow showers";
  if (code <= 99)                  return "Thunderstorm";
  return "Unknown";
}

// Build a two-letter day name + hi/lo entry into buf at pos; returns new pos.
static int appendDayEntry(char* buf, size_t bufSize, int pos,
                          const char* dateStr, float hi, float lo) {
  static const char* const DAY_NAMES[] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" };
  static const int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
  int y = atoi(dateStr);
  int m = atoi(dateStr + 5);
  int day = atoi(dateStr + 8);
  if (m < 3) y--;
  int dow = (y + y/4 - y/100 + y/400 + t[m-1] + day) % 7;
  char entry[20];
  snprintf(entry, sizeof(entry), "%s %.0f/%.0f  ", DAY_NAMES[dow], hi, lo);
  size_t entryLen = strlen(entry);
  if ((size_t)pos + entryLen < bufSize - 1) {
    strcpy(buf + pos, entry);
    pos += (int)entryLen;
  }
  return pos;
}

int abs(int x) {
  return x < 0 ? -x : x;
}

// Minimum delta between last and new top position
const int minDelta = 25;

// Where to start drawing the weather data
int top = 68;

// Last minute when top was changed
int lastLocationChangeMinute = 1;

int distantRandom(int min, int max) {
  int result;
  do {
    result = random(min, max + 1);  // Random value between min and max inclusive
  } while (abs(result - top) < minDelta);
  return result;
}

// OKLCH constants
const float OKLCH_L = 0.7f;   // Luminance
const float OKLCH_C = 0.2f;   // Chroma
const int DAY_START_HOUR = 6;   // 6 AM
const int DAY_END_HOUR = 21;    // 9 PM (21:00)
const int DAY_MINUTES = 900;    // 15 hours * 60 minutes

// Convert OKLCH to RGB565 for TFT display
// L: 0-1, C: 0-0.4 (typical), h: 0-360 degrees
uint16_t oklchToRgb565(float L, float C, float hDegrees) {
  // Convert hue to radians
  float h = hDegrees * M_PI / 180.0f;

  // OKLCH to Oklab: a = C * cos(h), b = C * sin(h)
  float a = C * cos(h);
  float b = C * sin(h);

  // Oklab to linear RGB (using Oklab inverse transform)
  // l_ = L + 0.3963377774f * a + 0.2158037573f * b
  // m_ = L - 0.1055613458f * a - 0.0638541728f * b
  // s_ = L - 0.0894841775f * a - 1.2914855480f * b
  float l_ = L + 0.3963377774f * a + 0.2158037573f * b;
  float m_ = L - 0.1055613458f * a - 0.0638541728f * b;
  float s_ = L - 0.0894841775f * a - 1.2914855480f * b;

  // Cube to get linear RGB
  float l = l_ * l_ * l_;
  float m = m_ * m_ * m_;
  float s = s_ * s_ * s_;

  // Linear RGB to sRGB (apply gamma correction)
  // Using sRGB gamma curve approximation
  auto gammaCorrect = [](float c) -> float {
    if (c <= 0.0031308f) {
      return c * 12.92f;
    } else {
      return 1.055f * pow(c, 1.0f / 2.4f) - 0.055f;
    }
  };

  float r = gammaCorrect(l);
  float g = gammaCorrect(m);
  float bVal = gammaCorrect(s);

  // Clamp to 0-1 range
  r = fmaxf(0.0f, fminf(1.0f, r));
  g = fmaxf(0.0f, fminf(1.0f, g));
  bVal = fmaxf(0.0f, fminf(1.0f, bVal));

  // Convert to RGB565: 5 bits R, 6 bits G, 5 bits B
  uint8_t r5 = (uint8_t)(r * 31.0f + 0.5f);  // 0-31
  uint8_t g6 = (uint8_t)(g * 63.0f + 0.5f);  // 0-63
  uint8_t b5 = (uint8_t)(bVal * 31.0f + 0.5f);  // 0-31

  return (r5 << 11) | (g6 << 5) | b5;
}

// Calculate hue based on time of day (6 AM to 9 PM)
// Returns hue in degrees (0-360), or -1 if outside daytime hours
float calculateDaytimeHue(int hour24, int minute) {
  if (hour24 < DAY_START_HOUR || hour24 >= DAY_END_HOUR) {
    return -1;  // Outside daytime hours
  }

  // Minutes since 6 AM
  int minutesSince6AM = (hour24 - DAY_START_HOUR) * 60 + minute;

  // Hue cycles from 0 to 360 over 15 hours (900 minutes)
  // Every 5 minutes = 2 degrees
  float hue = (minutesSince6AM / 5.0f) * 2.0f;

  // Wrap to 0-360
  while (hue >= 360.0f) {
    hue -= 360.0f;
  }

  return hue;
}

// Draw the weather/clock screen using parsed weather data.
// Clears the screen and draws (all left-justified):
//   1. Time + Date — HH:MM  Month DD, font 6
//   2. Temp line   — Temp: XX  Feels like: YY, font 4
//   3. Cond line   — <condition>  Hum: XX%  Wind YY, font 3
//   4. Week line 1 — first 4 days: Mo HH/LL  Tu HH/LL  ..., font 2
//   5. Week line 2 — remaining days, font 2
void drawWeatherScreen(const WeatherData& w) {
  tft.fillScreen(SCREEN_BG);

  if (!w.valid) {
    tft.setTextColor(WEATHER_TIMESTAMP_COLOR, SCREEN_BG);
    tft.drawString("No weather data", WEATHER_LEFT_MARGIN, 110, 2);
    return;
  }

  const char* tPtr = strchr(w.timestamp, 'T');
  uint16_t textColor;

  // 1. Time + Date on same line — "HH:MM  Month DD", font 6 (~48px tall), y=4
  if (tPtr != nullptr) {
    static const char* const MONTH_NAMES[] = {
      "", "January", "February", "March", "April", "May", "June",
      "July", "August", "September", "October", "November", "December"
    };
    int hour24 = atoi(tPtr + 1);
    int hour12 = hour24 % 12;
    if (hour12 == 0) {
      hour12 = 12;
    }
    int minute  = atoi(tPtr + 4);
    if (minute % 5 == 0 && minute != lastLocationChangeMinute) {
      top = distantRandom(4, 140);  // Random value between 4 and 140 inclusive
      lastLocationChangeMinute = minute;
    }

    // Calculate daytime color (6 AM to 9 PM)
    float hue = calculateDaytimeHue(hour24, minute);
    Serial.print("hue is ");
    Serial.println(hue);
    if (hue >= 0) {
      textColor = oklchToRgb565(OKLCH_L, OKLCH_C, hue);
    } else {
      textColor = WEATHER_TEXT_COLOR;  // White outside daytime
    }
    Serial.print("textColor is ");
    Serial.println(textColor);
    int month   = atoi(w.timestamp + 5);
    int day     = atoi(w.timestamp + 8);
    char timeBuf[40];
    char dateBuf[40];
    char descBuf[40];
    if (month >= 1 && month <= 12) {
      snprintf(timeBuf, sizeof(timeBuf), "%2d:%02d",
               hour12, minute);
      snprintf(dateBuf, sizeof(dateBuf), "%s %d",
               MONTH_NAMES[month], day);
    } else {
      snprintf(timeBuf, sizeof(timeBuf), "%2d:%02d", hour12, minute);
    }
    snprintf(descBuf, sizeof(descBuf), "%s", weatherCodeDescription(w.current.weather_code));
    tft.setTextColor(textColor, SCREEN_BG);
    tft.drawString(timeBuf, WEATHER_LEFT_MARGIN, top, 6);
    tft.drawString(dateBuf, WEATHER_LEFT_MARGIN + 125, top - 2, 4);
    tft.drawString(descBuf, WEATHER_LEFT_MARGIN + 125, top + 18, 4);
  }

  // 2. Temp/humidity/wind line — Temp XX Feel YY Hum %ZZ Wind WW, font 4 (~26px tall), y=56
  {
    char tempBuf[64];
    snprintf(tempBuf, sizeof(tempBuf), "T %.0f F %.0f H %d%% W %.0f",
             w.current.temperature_2m,
             w.current.apparent_temperature,
             w.current.relative_humidity_2m,
             w.current.wind_speed_10m);
    tft.drawString(tempBuf, WEATHER_LEFT_MARGIN, top + 52, 4);
  }

  // 3. Week summary — two lines: 4 days on line 1, rest on line 2, font 2 (~16px), y=116/136
  if (w.daily.count > 0) {
    char line1[128];
    int pos1 = 0;
    line1[0] = '\0';
    int total = w.daily.count < 5 ? w.daily.count : 5;
    for (int d = 1; d < total; d++) {
      pos1 = appendDayEntry(line1, sizeof(line1), pos1,
                            w.daily.time[d],
                            w.daily.temperature_2m_max[d],
                            w.daily.temperature_2m_min[d]);
    }
    tft.setTextColor(textColor, SCREEN_BG);
    tft.drawString(line1, WEATHER_LEFT_MARGIN, top + 82, 2);
  }
}

#endif
