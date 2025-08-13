#pragma once
#include "../config/WiFiCredentials.h" // Put WIFI_SSID and WIFI_PASSWORD here (git-ignored)
#include <WiFi.h>
#include <time.h>

namespace WiFiUtil {

// NTP Server
constexpr const char *ntpServer = "pool.ntp.org";

// Pacific Time with automatic DST rules
constexpr const char *tzInfo = "PST8PDT,M3.2.0/2,M11.1.0/2";

// ----- CONNECT + NTP SYNC -----
inline void connectAndSyncTime() {
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint8_t attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++attempt > 60) { // ~30 seconds timeout
      Serial.println("\nWiFi connection failed!");
      return;
    }
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Set timezone and start NTP sync
  configTzTime(tzInfo, ntpServer);

  // Wait for time sync
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Waiting for NTP time sync...");
    delay(500);
  }

  Serial.print("Time synced: ");
  Serial.println(asctime(&timeinfo));
}

// ----- AUTO-RECONNECT -----
inline void maintain() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost, reconnecting...");
    WiFi.disconnect();
    WiFi.reconnect();
  }
}

// ----- GET LOCAL TIME -----
inline bool getLocalTimeWithDST(struct tm &timeinfo) {
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return false;
  }
  return true; // tm_isdst will be correct automatically
}

} // namespace WiFiUtil
