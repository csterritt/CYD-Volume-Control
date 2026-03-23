#ifndef WEATHER_DISPLAY_H
#define WEATHER_DISPLAY_H

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

// Draw the weather/clock screen using parsed weather data.
// Clears the screen and draws (all left-justified):
//   1. Time        — HH:MM, font 6
//   2. Date        — Month DD, font 4
//   3. Temp line   — Temp: XX  Feels like: YY, font 4
//   4. Cond line   — <condition>  Hum: XX%  Wind YY, font 2
//   5. Week summary — Mo HH/LL  Tu HH/LL  ..., font 2
void drawWeatherScreen(const WeatherData& w) {
  tft.fillScreen(SCREEN_BG);

  if (!w.valid) {
    tft.setTextColor(WEATHER_TIMESTAMP_COLOR, SCREEN_BG);
    tft.drawString("No weather data", WEATHER_LEFT_MARGIN, 110, 2);
    return;
  }

  const char* tPtr = strchr(w.timestamp, 'T');

  // 1. Time — HH:MM, font 6 (largest that works for mixed text, ~48px tall)
  if (tPtr != nullptr) {
    int hour24 = atoi(tPtr + 1);
    int minute = atoi(tPtr + 4);
    char timeBuf[16];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", hour24, minute);
    tft.setTextColor(WEATHER_TEXT_COLOR, SCREEN_BG);
    tft.drawString(timeBuf, WEATHER_LEFT_MARGIN, 4, 6);
  }

  // 2. Date — Month DD, font 4 (~26px tall), y=56
  if (tPtr != nullptr) {
    static const char* const MONTH_NAMES[] = {
      "", "January", "February", "March", "April", "May", "June",
      "July", "August", "September", "October", "November", "December"
    };
    int month = atoi(w.timestamp + 5);
    int day   = atoi(w.timestamp + 8);
    char dateBuf[24];
    if (month >= 1 && month <= 12) {
      snprintf(dateBuf, sizeof(dateBuf), "%s %d", MONTH_NAMES[month], day);
    } else {
      size_t dateLen = (size_t)(tPtr - w.timestamp);
      if (dateLen >= sizeof(dateBuf)) dateLen = sizeof(dateBuf) - 1;
      strncpy(dateBuf, w.timestamp, dateLen);
      dateBuf[dateLen] = '\0';
    }
    tft.setTextColor(WEATHER_DIM_COLOR, SCREEN_BG);
    tft.drawString(dateBuf, WEATHER_LEFT_MARGIN, 56, 4);
  }

  // 3. Temp line — Temp: XX  Feels like: YY, font 4, y=86
  {
    char tempBuf[48];
    snprintf(tempBuf, sizeof(tempBuf), "Temp: %.0f  Feels like: %.0f",
             w.current.temperature_2m,
             w.current.apparent_temperature);
    tft.setTextColor(WEATHER_TEXT_COLOR, SCREEN_BG);
    tft.drawString(tempBuf, WEATHER_LEFT_MARGIN, 86, 4);
  }

  // 4. Condition + Hum + Wind, font 2 (~16px tall), y=116
  {
    char condBuf[64];
    snprintf(condBuf, sizeof(condBuf), "%s  Hum: %d%%  Wind %.0f",
             weatherCodeDescription(w.current.weather_code),
             w.current.relative_humidity_2m,
             w.current.wind_speed_10m);
    tft.setTextColor(WEATHER_DIM_COLOR, SCREEN_BG);
    tft.drawString(condBuf, WEATHER_LEFT_MARGIN, 116, 2);
  }

  // 5. Week summary — two-letter day name + high/low for each day, font 2, y=140
  if (w.daily.count > 0) {
    static const char* const DAY_NAMES[] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" };
    char weekBuf[128];
    int pos = 0;
    weekBuf[0] = '\0';
    for (int d = 0; d < w.daily.count && d < 7; d++) {
      // w.daily.time[d] is "YYYY-MM-DD"; compute day-of-week via simple Zeller-like formula
      const char* dateStr = w.daily.time[d];
      int y = atoi(dateStr);
      int m = atoi(dateStr + 5);
      int day = atoi(dateStr + 8);
      // Tomohiko Sakamoto's algorithm
      static const int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
      if (m < 3) y--;
      int dow = (y + y/4 - y/100 + y/400 + t[m-1] + day) % 7;
      char entry[20];
      snprintf(entry, sizeof(entry), "%s %.0f/%.0f  ",
               DAY_NAMES[dow],
               w.daily.temperature_2m_max[d],
               w.daily.temperature_2m_min[d]);
      size_t entryLen = strlen(entry);
      if (pos + entryLen < sizeof(weekBuf) - 1) {
        strcpy(weekBuf + pos, entry);
        pos += (int)entryLen;
      }
    }
    tft.setTextColor(WEATHER_TEXT_COLOR, SCREEN_BG);
    tft.drawString(weekBuf, WEATHER_LEFT_MARGIN, 140, 2);
  }

  // Touch hint at the bottom
  tft.setTextColor(WEATHER_DIM_COLOR, SCREEN_BG);
  tft.drawString("Touch to return", WEATHER_LEFT_MARGIN, 225, 1);
}

#endif
