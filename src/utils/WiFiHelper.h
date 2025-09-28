#pragma once
#include "../config/Credentials.h"
#include "MessageTypes.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <esp_now.h>

// Define callback function types
using SendCallback = void (*)(const uint8_t *, esp_now_send_status_t);
using RecvCallback = void (*)(const uint8_t *, const uint8_t *, int);

// Class to manage WiFi, ESP-NOW, OTA, and NTP time sync
class WiFiHelper {
public:
  WiFiHelper();
  static bool attemptSend(const uint8_t *mac, const uint8_t *data);
  static void resetSendState();
  static bool sendData(const uint8_t *mac, const uint8_t *data);

  static void defaultOnDataSent(const uint8_t *mac_addr,
                                esp_now_send_status_t status);
  static void defaultOnDataRecv(const uint8_t *mac_addr, const uint8_t *data,
                                int data_len);
  void setupOTA();
  void setupEspNow(bool isCameraBoard = false, RecvCallback recvCb = nullptr,
                   SendCallback sendCb = nullptr);
  void connectAndSyncTime(bool shouldSetupOTA = false);
  void maintain();
  bool getLocalTimeWithDST(struct tm &timeinfo);

  static struct_message myData;
  esp_now_peer_info_t peerInfo;

private:
  // State tracking for esp-now send retries
  static unsigned long lastSendAttempt;
  static int currentRetryCount;
  static bool sendPending;
  static const uint8_t *pendingMac;
  static const uint8_t *pendingData;

  static constexpr int ESP_NOW_RETRY_DELAY_MS = 300;
  static constexpr int MAX_ESP_NOW_RETRIES = 10;
  static constexpr int WIFI_RETRY_DELAY_MS = 250;
  static constexpr int MAX_WIFI_RETRIES = 30;
  static constexpr const char *ntpServer = "pool.ntp.org";
  static constexpr const char *tzInfo = "PST8PDT,M3.2.0/2,M11.1.0/2";
};
