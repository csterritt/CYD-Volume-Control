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

  // 1. Time + Date on same line — "HH:MM  Month DD", font 6 (~48px tall), y=4
  if (tPtr != nullptr) {
    static const char* const MONTH_NAMES[] = {
      "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    int hour24 = atoi(tPtr + 1);
    int hour12 = hour24 % 12;
    if (hour12 == 0) {
      hour12 = 12;
    }
    int minute  = atoi(tPtr + 4);
    int month   = atoi(w.timestamp + 5);
    int day     = atoi(w.timestamp + 8);
    char timeBuf[40];
    char dateBuf[40];
    if (month >= 1 && month <= 12) {
      snprintf(timeBuf, sizeof(timeBuf), "%2d:%02d",
               hour12, minute);
      snprintf(dateBuf, sizeof(dateBuf), "%s %d",
               MONTH_NAMES[month], day);
    } else {
      snprintf(timeBuf, sizeof(timeBuf), "%2d:%02d", hour12, minute);
    }
    tft.setTextColor(WEATHER_TEXT_COLOR, SCREEN_BG);
    tft.drawString(timeBuf, WEATHER_LEFT_MARGIN, 4, 6);
    tft.drawString(dateBuf, WEATHER_LEFT_MARGIN + 125, 14, 4);
  }

  // 2. Temp line — Temp: XX  Feels like: YY, font 4 (~26px tall), y=56
  {
    char tempBuf[48];
    snprintf(tempBuf, sizeof(tempBuf), "Temp %.0f  Feel %.0f",
             w.current.temperature_2m,
             w.current.apparent_temperature);
    tft.setTextColor(WEATHER_TEXT_COLOR, SCREEN_BG);
    tft.drawString(tempBuf, WEATHER_LEFT_MARGIN, 56, 4);
  }

  // 3. Condition + Hum + Wind, font 4 (same as temp), y=86
  {
    char condBuf[64];
    snprintf(condBuf, sizeof(condBuf), "%s  H: %d%%  W: %.0f",
             weatherCodeDescription(w.current.weather_code),
             w.current.relative_humidity_2m,
             w.current.wind_speed_10m);
    tft.setTextColor(WEATHER_DIM_COLOR, SCREEN_BG);
    tft.drawString(condBuf, WEATHER_LEFT_MARGIN, 86, 4);
  }

  // 4+5. Week summary — two lines: 4 days on line 1, rest on line 2, font 2 (~16px), y=116/136
  if (w.daily.count > 0) {
    char line1[128];
    char line2[128];
    int pos1 = 0, pos2 = 0;
    line1[0] = '\0';
    line2[0] = '\0';
    int total = w.daily.count < 7 ? w.daily.count : 7;
    for (int d = 0; d < total; d++) {
      if (d < 4) {
        pos1 = appendDayEntry(line1, sizeof(line1), pos1,
                              w.daily.time[d],
                              w.daily.temperature_2m_max[d],
                              w.daily.temperature_2m_min[d]);
      } else {
        pos2 = appendDayEntry(line2, sizeof(line2), pos2,
                              w.daily.time[d],
                              w.daily.temperature_2m_max[d],
                              w.daily.temperature_2m_min[d]);
      }
    }
    tft.setTextColor(WEATHER_TEXT_COLOR, SCREEN_BG);
    tft.drawString(line1, WEATHER_LEFT_MARGIN, 116, 2);
    if (line2[0] != '\0') {
      tft.drawString(line2, WEATHER_LEFT_MARGIN, 136, 2);
    }
  }

  // Touch hint at the bottom
  tft.setTextColor(WEATHER_DIM_COLOR, SCREEN_BG);
  tft.drawString("Touch to return", WEATHER_LEFT_MARGIN, 225, 1);
}

#endif
