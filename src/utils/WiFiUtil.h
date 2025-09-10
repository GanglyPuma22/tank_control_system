#pragma once
#include "../config/Credentials.h" // Put WIFI_SSID and WIFI_PASSWORD here (git-ignored)
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <time.h>

namespace WiFiUtil {

// NTP Server
constexpr const char *ntpServer = "pool.ntp.org";

// Pacific Time with automatic DST rules
constexpr const char *tzInfo = "PST8PDT,M3.2.0/2,M11.1.0/2";

inline void setupOTA() {
  // OTA setup
  ArduinoOTA.onStart([]() { Serial.println("OTA Update Start"); })
      .onEnd([]() { Serial.println("OTA Update End"); })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed");
      });

  ArduinoOTA.begin();
  Serial.println("OTA ready. IP address: " + WiFi.localIP().toString());
}

// ----- CONNECT + OTA + NTP SYNC -----
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

  // Setup OTA - for wireless updates
  setupOTA();

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
  ArduinoOTA.handle(); // Keep checking for OTA updates
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
