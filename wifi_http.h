/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
 
#ifndef WIFI_HTTP_H
#define WIFI_HTTP_H

#include <WiFi.h>
#include <HTTPClient.h>

#include "common_definitions.h"

// Cached resolved IP address for the web server
static IPAddress resolvedServerIP;
static bool isIPResolved = false;

// WiFi reconnection tracking
static unsigned long lastReconnectAttempt = 0;
const unsigned long WIFI_RECONNECT_INTERVAL = 30000;  // 30 seconds

// Resolve WEB_SERVER_ADDRESS to an IP address if it's a hostname.
// Returns the IP address (either resolved or from cache).
static IPAddress getServerIP() {
  if (isIPResolved) {
    return resolvedServerIP;
  }

  // Try to parse as IP address first
  if (resolvedServerIP.fromString(WEB_SERVER_ADDRESS)) {
    isIPResolved = true;
    Serial.printf("Using IP address: %s\n", WEB_SERVER_ADDRESS);
    return resolvedServerIP;
  }

  // If not an IP, perform DNS lookup
  Serial.printf("Resolving hostname: %s\n", WEB_SERVER_ADDRESS);
  if (WiFi.hostByName(WEB_SERVER_ADDRESS, resolvedServerIP)) {
    isIPResolved = true;
    Serial.printf("Resolved to IP: %s\n", resolvedServerIP.toString().c_str());
  } else {
    Serial.println("DNS lookup failed");
  }

  return resolvedServerIP;
}

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

// Attempt to reconnect WiFi if disconnected. Call from loop().
// Tries every WIFI_RECONNECT_INTERVAL (30 s) until it succeeds.
void maintainWiFiConnection() {
  if (isWiFiConnected()) {
    return;
  }

  unsigned long now = millis();
  if (now - lastReconnectAttempt < WIFI_RECONNECT_INTERVAL) {
    return;
  }

  lastReconnectAttempt = now;
  Serial.println("WiFi disconnected — attempting reconnect...");
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Wait up to 10 seconds for this attempt
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (isWiFiConnected()) {
    Serial.printf("WiFi reconnected, IP: %s\n", WiFi.localIP().toString().c_str());
    // Clear DNS cache — server IP may have changed
    isIPResolved = false;
    resolvedServerIP = INADDR_NONE;
  } else {
    Serial.println("WiFi reconnect failed — will retry in 30 seconds");
  }
}

// Send an HTTP POST to the given path on the configured web server.
// Returns true on HTTP 2xx response.
static bool postToServer(const char* path) {
  if (!isWiFiConnected()) {
    maintainWiFiConnection();
    if (!isWiFiConnected()) {
      Serial.printf("WiFi not connected, skipping POST %s\n", path);
      return false;
    }
  }

  IPAddress serverIP = getServerIP();
  if (serverIP == INADDR_NONE) {
    Serial.println("Cannot resolve server address");
    return false;
  }

  HTTPClient http;
  char url[128];
  snprintf(url, sizeof(url), "http://%s:%s%s", serverIP.toString().c_str(), WEB_SERVER_PORT, path);
  // Serial.printf("POST %s\n", url);

  http.begin(url);
  http.setTimeout(3000);
  int code = http.POST("");
  // Serial.printf("HTTP response: %d\n", code);
  http.end();

  return code >= 200 && code < 300;
}

void sendVolumeUp() {
  postToServer("/api/v1/volume-up");
  // Serial.println("VOL_UP");
}

void sendVolumeDown() {
  postToServer("/api/v1/volume-down");
  // Serial.println("VOL_DN");
}

void sendMute() {
  postToServer("/api/v1/mute");
  // Serial.println("MUTE");
}

// Fetch weather JSON from GET /weather on the configured web server.
// Stores the response body into buf (up to bufLen-1 chars, null-terminated).
// Returns true on HTTP 2xx response.
bool fetchWeather(char* buf, size_t bufLen) {
  if (!isWiFiConnected()) {
    maintainWiFiConnection();
    if (!isWiFiConnected()) {
      Serial.println("fetchWeather: WiFi not connected");
      return false;
    }
  }

  IPAddress serverIP = getServerIP();
  if (serverIP == INADDR_NONE) {
    Serial.println("Cannot resolve server address");
    return false;
  }

  HTTPClient http;
  char url[128];
  snprintf(url, sizeof(url), "http://%s:%s/weather", serverIP.toString().c_str(), WEB_SERVER_PORT);
  // Serial.printf("GET %s\n", url);

  http.begin(url);
  http.setTimeout(5000);
  int code = http.GET();
  // Serial.printf("fetchWeather HTTP response: %d\n", code);

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
