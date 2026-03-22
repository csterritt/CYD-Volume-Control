#ifndef WEATHER_DATA_H
#define WEATHER_DATA_H

#include <JsmnStream.h>

// Maximum number of jsmn tokens needed to parse the /weather response.
// The response has roughly 80+ tokens; 150 gives comfortable headroom.
static const size_t WEATHER_TOKEN_COUNT = 150;

// All fields we care about from the /weather JSON response
struct WeatherCurrent {
  char time[32];
  float temperature_2m;
  float apparent_temperature;
  int relative_humidity_2m;
  float wind_speed_10m;
  int wind_direction_10m;
  int weather_code;
};

struct WeatherDaily {
  char time[7][16];
  float temperature_2m_max[7];
  float temperature_2m_min[7];
  int precipitation_probability_max[7];
  int weather_code[7];
  int count;
};

struct WeatherData {
  char timestamp[40];
  float latitude;
  float longitude;
  char timezone[40];
  char timezone_abbreviation[16];
  WeatherCurrent current;
  WeatherDaily daily;
  bool valid;
};

// Copy at most (maxLen-1) characters from js[tok.start..tok.end) into dest
static void tokenToStr(const char* js, const JsmnStream::jsmntok_t& tok, char* dest, size_t maxLen) {
  size_t len = (size_t)(tok.end - tok.start);
  if (len >= maxLen) len = maxLen - 1;
  strncpy(dest, js + tok.start, len);
  dest[len] = '\0';
}

static float tokenToFloat(const char* js, const JsmnStream::jsmntok_t& tok) {
  char buf[32];
  tokenToStr(js, tok, buf, sizeof(buf));
  return (float)atof(buf);
}

static int tokenToInt(const char* js, const JsmnStream::jsmntok_t& tok) {
  char buf[32];
  tokenToStr(js, tok, buf, sizeof(buf));
  return atoi(buf);
}

// Return true if the token at index i matches the given key string
static bool tokenEq(const char* js, const JsmnStream::jsmntok_t& tok, const char* key) {
  int len = tok.end - tok.start;
  return (tok.type == JsmnStream::JSMN_STRING) &&
         (len == (int)strlen(key)) &&
         (strncmp(js + tok.start, key, len) == 0);
}

// Parse the /weather JSON response into a WeatherData struct.
// js must remain valid for the lifetime of this call.
// Returns true on success.
bool parseWeatherJson(const char* js, WeatherData& out) {
  static JsmnStream::jsmntok_t tokens[WEATHER_TOKEN_COUNT];
  JsmnStream jsmn(tokens);

  int tokenCount = jsmn.parseJson(js);
  if (tokenCount < 1) {
    Serial.printf("parseWeatherJson: parse error %d\n", tokenCount);
    return false;
  }

  memset(&out, 0, sizeof(out));
  out.valid = false;

  // Walk through top-level keys
  // Token 0 is the root object, tokens 1..N are key/value pairs
  int i = 1;
  while (i < tokenCount) {
    if (tokenEq(js, tokens[i], "timestamp")) {
      if (i + 1 < tokenCount) {
        tokenToStr(js, tokens[i + 1], out.timestamp, sizeof(out.timestamp));
        i += 2;
        continue;
      }
    }

    if (tokenEq(js, tokens[i], "weather") && i + 1 < tokenCount) {
      // tokens[i+1] is the weather object; walk its children
      int weatherObjIdx = i + 1;
      int j = weatherObjIdx + 1;
      int weatherEnd = tokens[weatherObjIdx].end;

      while (j < tokenCount && tokens[j].start < weatherEnd) {
        if (tokenEq(js, tokens[j], "latitude") && j + 1 < tokenCount) {
          out.latitude = tokenToFloat(js, tokens[j + 1]);
          j += 2;
          continue;
        }

        if (tokenEq(js, tokens[j], "longitude") && j + 1 < tokenCount) {
          out.longitude = tokenToFloat(js, tokens[j + 1]);
          j += 2;
          continue;
        }

        if (tokenEq(js, tokens[j], "timezone") && j + 1 < tokenCount) {
          tokenToStr(js, tokens[j + 1], out.timezone, sizeof(out.timezone));
          j += 2;
          continue;
        }

        if (tokenEq(js, tokens[j], "timezone_abbreviation") && j + 1 < tokenCount) {
          tokenToStr(js, tokens[j + 1], out.timezone_abbreviation, sizeof(out.timezone_abbreviation));
          j += 2;
          continue;
        }

        if (tokenEq(js, tokens[j], "current") && j + 1 < tokenCount) {
          int curObjIdx = j + 1;
          int k = curObjIdx + 1;
          int curEnd = tokens[curObjIdx].end;

          while (k < tokenCount && tokens[k].start < curEnd) {
            if (tokenEq(js, tokens[k], "time") && k + 1 < tokenCount) {
              tokenToStr(js, tokens[k + 1], out.current.time, sizeof(out.current.time));
              k += 2; continue;
            }
            if (tokenEq(js, tokens[k], "temperature_2m") && k + 1 < tokenCount) {
              out.current.temperature_2m = tokenToFloat(js, tokens[k + 1]);
              k += 2; continue;
            }
            if (tokenEq(js, tokens[k], "apparent_temperature") && k + 1 < tokenCount) {
              out.current.apparent_temperature = tokenToFloat(js, tokens[k + 1]);
              k += 2; continue;
            }
            if (tokenEq(js, tokens[k], "relative_humidity_2m") && k + 1 < tokenCount) {
              out.current.relative_humidity_2m = tokenToInt(js, tokens[k + 1]);
              k += 2; continue;
            }
            if (tokenEq(js, tokens[k], "wind_speed_10m") && k + 1 < tokenCount) {
              out.current.wind_speed_10m = tokenToFloat(js, tokens[k + 1]);
              k += 2; continue;
            }
            if (tokenEq(js, tokens[k], "wind_direction_10m") && k + 1 < tokenCount) {
              out.current.wind_direction_10m = tokenToInt(js, tokens[k + 1]);
              k += 2; continue;
            }
            if (tokenEq(js, tokens[k], "weather_code") && k + 1 < tokenCount) {
              out.current.weather_code = tokenToInt(js, tokens[k + 1]);
              k += 2; continue;
            }
            k++;
          }

          j = k;
          continue;
        }

        if (tokenEq(js, tokens[j], "daily") && j + 1 < tokenCount) {
          int dailyObjIdx = j + 1;
          int k = dailyObjIdx + 1;
          int dailyEnd = tokens[dailyObjIdx].end;

          while (k < tokenCount && tokens[k].start < dailyEnd) {
            if (tokenEq(js, tokens[k], "time") && k + 1 < tokenCount) {
              int arrIdx = k + 1;
              int arrEnd = tokens[arrIdx].end;
              int m = arrIdx + 1;
              out.daily.count = 0;
              while (m < tokenCount && tokens[m].start < arrEnd && out.daily.count < 7) {
                tokenToStr(js, tokens[m], out.daily.time[out.daily.count], sizeof(out.daily.time[0]));
                out.daily.count++;
                m++;
              }
              k = m;
              continue;
            }

            if (tokenEq(js, tokens[k], "temperature_2m_max") && k + 1 < tokenCount) {
              int arrIdx = k + 1;
              int arrEnd = tokens[arrIdx].end;
              int m = arrIdx + 1;
              int n = 0;
              while (m < tokenCount && tokens[m].start < arrEnd && n < 7) {
                out.daily.temperature_2m_max[n++] = tokenToFloat(js, tokens[m++]);
              }
              k = m;
              continue;
            }

            if (tokenEq(js, tokens[k], "temperature_2m_min") && k + 1 < tokenCount) {
              int arrIdx = k + 1;
              int arrEnd = tokens[arrIdx].end;
              int m = arrIdx + 1;
              int n = 0;
              while (m < tokenCount && tokens[m].start < arrEnd && n < 7) {
                out.daily.temperature_2m_min[n++] = tokenToFloat(js, tokens[m++]);
              }
              k = m;
              continue;
            }

            if (tokenEq(js, tokens[k], "precipitation_probability_max") && k + 1 < tokenCount) {
              int arrIdx = k + 1;
              int arrEnd = tokens[arrIdx].end;
              int m = arrIdx + 1;
              int n = 0;
              while (m < tokenCount && tokens[m].start < arrEnd && n < 7) {
                out.daily.precipitation_probability_max[n++] = tokenToInt(js, tokens[m++]);
              }
              k = m;
              continue;
            }

            if (tokenEq(js, tokens[k], "weather_code") && k + 1 < tokenCount) {
              int arrIdx = k + 1;
              int arrEnd = tokens[arrIdx].end;
              int m = arrIdx + 1;
              int n = 0;
              while (m < tokenCount && tokens[m].start < arrEnd && n < 7) {
                out.daily.weather_code[n++] = tokenToInt(js, tokens[m++]);
              }
              k = m;
              continue;
            }

            k++;
          }

          j = k;
          continue;
        }

        j++;
      }

      i = j;
      continue;
    }

    i++;
  }

  out.valid = (out.timestamp[0] != '\0');
  return out.valid;
}

#endif
