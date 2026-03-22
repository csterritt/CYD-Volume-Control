#ifndef WEATHER_DISPLAY_H
#define WEATHER_DISPLAY_H

#include "common_definitions.h"
#include "weather_data.h"

#define WEATHER_TIMESTAMP_COLOR 0xF800  // Red
#define WEATHER_TEXT_COLOR      0xFFFF  // White
#define WEATHER_DIM_COLOR       0x8410  // Gray

// Draw the weather/clock screen using parsed weather data.
// Clears the screen and draws:
//   - timestamp in red, top-left
//   - current temperature, feels-like, humidity, wind
//   - today's high/low
void drawWeatherScreen(const WeatherData& w) {
  tft.fillScreen(SCREEN_BG);

  if (!w.valid) {
    tft.setTextColor(WEATHER_TIMESTAMP_COLOR, SCREEN_BG);
    tft.drawCentreString("No weather data", 160, 110, 2);
    return;
  }

  // Parse timestamp "YYYY-MM-DDThh:mm" into formatted strings
  const char* tPtr = strchr(w.timestamp, 'T');

  // Time — convert 24h "HH:MM" to 12h "H:MM", centered, largest (font 8, which only has digits)
  if (tPtr != nullptr) {
    int hour24 = atoi(tPtr + 1);
    int minute = atoi(tPtr + 4);
    int hour12 = hour24 % 12;
    if (hour12 == 0) hour12 = 12;
    char timeBuf[16];
    snprintf(timeBuf, sizeof(timeBuf), "%d:%02d", hour12, minute);
    tft.setTextColor(WEATHER_TEXT_COLOR, SCREEN_BG);
    tft.drawCentreString(timeBuf, 160, 10, 8);
  }

  // Date — convert "YYYY-MM-DD" to "Month DD", centered beneath time
  if (tPtr != nullptr) {
    static const char* const MONTH_NAMES[] = {
      "", "January", "February", "March", "April", "May", "June",
      "July", "August", "September", "October", "November", "December"
    };
    // w.timestamp starts with "YYYY-MM-DD"
    int month = atoi(w.timestamp + 5);
    int day   = atoi(w.timestamp + 8);
    char dateBuf[24];
    if (month >= 1 && month <= 12) {
      snprintf(dateBuf, sizeof(dateBuf), "%s %d", MONTH_NAMES[month], day);
    } else {
      // Fallback: raw date slice
      size_t dateLen = (size_t)(tPtr - w.timestamp);
      if (dateLen >= sizeof(dateBuf)) dateLen = sizeof(dateBuf) - 1;
      strncpy(dateBuf, w.timestamp, dateLen);
      dateBuf[dateLen] = '\0';
    }
    tft.setTextColor(WEATHER_DIM_COLOR, SCREEN_BG);
    tft.drawCentreString(dateBuf, 160, 96, 2);
  }

  // Current temperature — smaller than time (font 4), centered
  char tempBuf[32];
  snprintf(tempBuf, sizeof(tempBuf), "%.0f", w.current.temperature_2m);
  tft.setTextColor(WEATHER_TEXT_COLOR, SCREEN_BG);
  tft.drawCentreString(tempBuf, 160, 118, 4);

  // Feels like
  char feelsBuf[32];
  snprintf(feelsBuf, sizeof(feelsBuf), "Feels like %.0f", w.current.apparent_temperature);
  tft.setTextColor(WEATHER_DIM_COLOR, SCREEN_BG);
  tft.drawCentreString(feelsBuf, 160, 160, 2);

  // Humidity and wind — side by side in a row
  char humBuf[24];
  snprintf(humBuf, sizeof(humBuf), "Hum: %d%%", w.current.relative_humidity_2m);
  tft.setTextColor(WEATHER_TEXT_COLOR, SCREEN_BG);
  tft.drawString(humBuf, 8, 182, 2);

  char windBuf[32];
  snprintf(windBuf, sizeof(windBuf), "Wind: %.1f mp/h", w.current.wind_speed_10m);
  tft.drawString(windBuf, 160, 182, 2);

  // Today's high / low
  if (w.daily.count > 0) {
    char hlBuf[32];
    snprintf(hlBuf, sizeof(hlBuf), "H:%.0f\xb0  L:%.0f\xb0",
             w.daily.temperature_2m_max[0],
             w.daily.temperature_2m_min[0]);
    tft.setTextColor(WEATHER_DIM_COLOR, SCREEN_BG);
    tft.drawCentreString(hlBuf, 160, 204, 2);
  }

  // Touch hint at the bottom
  tft.setTextColor(WEATHER_DIM_COLOR, SCREEN_BG);
  tft.drawCentreString("Touch to return", 160, 228, 1);
}

#endif
