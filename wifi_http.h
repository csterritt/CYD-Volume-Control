#ifndef WIFI_HTTP_H
#define WIFI_HTTP_H

#include <WiFi.h>
#include <HTTPClient.h>

#include "common_definitions.h"

// Initialize WiFi — call in setup(). Blocks until connected or times out.
void initializeWiFi() {
  Serial.printf("WiFi connecting to %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const unsigned long timeout = 15000;
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("WiFi connection timed out");
  }
}

// Returns true when WiFi is connected
bool isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

// Send an HTTP POST to the given path on the configured web server.
// Returns true on HTTP 2xx response.
static bool postToServer(const char* path) {
  if (!isWiFiConnected()) {
    Serial.printf("WiFi not connected, skipping POST %s\n", path);
    return false;
  }

  HTTPClient http;
  char url[128];
  snprintf(url, sizeof(url), "http://%s%s", WEB_SERVER_ADDRESS, path);
  Serial.printf("POST %s\n", url);

  http.begin(url);
  http.setTimeout(3000);
  int code = http.POST("");
  Serial.printf("HTTP response: %d\n", code);
  http.end();

  return code >= 200 && code < 300;
}

void sendVolumeUp() {
  postToServer("/api/v1/volume-up");
  Serial.println("VOL_UP");
}

void sendVolumeDown() {
  postToServer("/api/v1/volume-down");
  Serial.println("VOL_DN");
}

void sendMute() {
  postToServer("/api/v1/mute");
  Serial.println("MUTE");
}

// Fetch weather JSON from GET /weather on the configured web server.
// Stores the response body into buf (up to bufLen-1 chars, null-terminated).
// Returns true on HTTP 2xx response.
bool fetchWeather(char* buf, size_t bufLen) {
  if (!isWiFiConnected()) {
    Serial.println("fetchWeather: WiFi not connected");
    return false;
  }

  HTTPClient http;
  char url[128];
  snprintf(url, sizeof(url), "http://%s/weather", WEB_SERVER_ADDRESS);
  Serial.printf("GET %s\n", url);

  http.begin(url);
  http.setTimeout(5000);
  int code = http.GET();
  Serial.printf("fetchWeather HTTP response: %d\n", code);

  if (code >= 200 && code < 300) {
    String body = http.getString();
    size_t len = body.length();
    if (len >= bufLen) len = bufLen - 1;
    memcpy(buf, body.c_str(), len);
    buf[len] = '\0';
    http.end();
    return true;
  }

  http.end();
  return false;
}

#endif
