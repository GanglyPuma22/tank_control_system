#pragma once
#include "../config/Credentials.h"
#include <Arduino.h> // For uint8_t and other Arduino types
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <esp_now.h>

// Structure example to send data
// Must match the receiver structure
struct struct_message {
  char message[32];
  int camera_action;
};

// Global variables
extern struct_message myData;

namespace WiFiUtil {
extern esp_now_peer_info_t peerInfo;

// ESP-NOW callback functions
void defaultOnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void defaultOnDataRecv(const uint8_t *mac_addr, const uint8_t *data,
                       int data_len);
} // namespace WiFiUtil
constexpr const char *ntpServer = "pool.ntp.org";

// Pacific Time with automatic DST rules
constexpr const char *tzInfo = "PST8PDT,M3.2.0/2,M11.1.0/2";

// Define callback function types
using SendCallback = void (*)(const uint8_t *, esp_now_send_status_t);
using RecvCallback = void (*)(const uint8_t *, const uint8_t *, int);
esp_now_peer_info_t peerInfo;

// callback when data is sent
void defaultOnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success"
                                                : "Delivery Fail");
}

// callback function that will be executed when data is received
void defaultOnDataRecv(const uint8_t *mac, const uint8_t *incomingData,
                       int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Message: ");
  Serial.println(myData.message);
  Serial.print("Camera Action: ");
  Serial.println(myData.camera_action);
}

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

inline void setupEspNow(bool isCameraBoard = false,
                        RecvCallback recvCb = nullptr,
                        SendCallback sendCb = nullptr) {
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  if (isCameraBoard) {
    esp_now_register_recv_cb(recvCb ? recvCb : defaultOnDataRecv);
    memcpy(peerInfo.peer_addr, MAIN_BOARD_MAC_ADDRESS, 6);
  } else {
    esp_now_register_send_cb(sendCb ? sendCb : defaultOnDataSent);
    memcpy(peerInfo.peer_addr, CAMERA_BOARD_MAC_ADDRESS, 6);
  }

  peerInfo.channel = 0; // pick a channel
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

// ----- CONNECT + OTA + NTP SYNC -----
inline void connectAndSyncTime(bool shouldSetupOTA = false) {
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
  if (shouldSetupOTA) {
    setupOTA();
  }

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
