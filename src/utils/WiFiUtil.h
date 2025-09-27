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

class WiFiUtil {
public:
  WiFiUtil();
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
  static constexpr const char *ntpServer = "pool.ntp.org";
  static constexpr const char *tzInfo = "PST8PDT,M3.2.0/2,M11.1.0/2";
};
