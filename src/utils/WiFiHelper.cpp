#include "WiFiHelper.h"

// Initialize static members
struct_message WiFiHelper::myData;
unsigned long WiFiHelper::lastSendAttempt = 0;
int WiFiHelper::currentRetryCount = 0;
bool WiFiHelper::sendPending = false;
const uint8_t *WiFiHelper::pendingMac = nullptr;
const uint8_t *WiFiHelper::pendingData = nullptr;

WiFiHelper::WiFiHelper() {}

bool WiFiHelper::attemptSend(const uint8_t *mac, const uint8_t *data) {
  esp_err_t result = esp_now_send(mac, data, sizeof(struct_message));
  if (result == ESP_OK) {
    resetSendState();
    return true;
  }
  Serial.print("Send attempt failed with error: ");
  Serial.println(result);
  return false;
}

void WiFiHelper::resetSendState() {
  sendPending = false;
  currentRetryCount = 0;
  pendingMac = nullptr;
  pendingData = nullptr;
}

bool WiFiHelper::sendData(const uint8_t *mac, const uint8_t *data) {
  unsigned long currentTime = millis();

  // First attempt or retry after timeout
  if (!sendPending) {
    Serial.println("Attempting to send data... First attempt.");
    if (attemptSend(mac, data)) {
      return true;
    }
    // Setup for retry
    sendPending = true;
    currentRetryCount = 0;
    lastSendAttempt = currentTime;
    pendingMac = mac;
    pendingData = data;
    return false;
  }

  // Handle ongoing retry sequence
  if (sendPending &&
      (currentTime - lastSendAttempt >= ESP_NOW_RETRY_DELAY_MS)) {
    if (currentRetryCount >= MAX_ESP_NOW_RETRIES) {
      Serial.println("All retries failed");
      resetSendState();
      return false;
    }

    currentRetryCount++;
    Serial.print("Retry attempt ");
    Serial.println(currentRetryCount);

    lastSendAttempt = currentTime;
    return attemptSend(pendingMac, pendingData);
  }

  return false;
}

void WiFiHelper::defaultOnDataSent(const uint8_t *mac_addr,
                                   esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success"
                                                : "Delivery Fail");
}

void WiFiHelper::defaultOnDataRecv(const uint8_t *mac_addr, const uint8_t *data,
                                   int data_len) {
  memcpy(&myData, data, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(data_len);
}

void WiFiHelper::setupOTA() {
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

void WiFiHelper::setupEspNow(bool isCameraBoard, RecvCallback recvCb,
                             SendCallback sendCb) {
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

  // Set the WiFi channel to match on both boards for reliable ESP-NOW
  // communication. You can use WiFi.channel() after connecting to WiFi, or
  // hardcode a channel (1-13). Example: peerInfo.channel = WiFi.channel(); //
  peerInfo.channel = WiFi.channel();
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  Serial.println("ESP-NOW initialized");
}

void WiFiHelper::connectAndSyncTime(bool shouldSetupOTA) {
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint8_t attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(WIFI_RETRY_DELAY_MS);
    Serial.print(".");
    if (++attempt > MAX_WIFI_RETRIES) {
      Serial.println("\nWiFi connection failed!");
      return;
    }
  }
  delay(1000); // Wait a moment for connection to stabilize
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());

  if (shouldSetupOTA) {
    setupOTA();
  }

  configTzTime(tzInfo, ntpServer);

  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Waiting for NTP time sync...");
    delay(500);
  }

  Serial.print("Time synced: ");
  Serial.println(asctime(&timeinfo));
}

void WiFiHelper::maintain() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost, reconnecting...");
    WiFi.disconnect();
    WiFi.reconnect();
  }
  ArduinoOTA.handle();
}

bool WiFiHelper::getLocalTimeWithDST(struct tm &timeinfo) {
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return false;
  }
  return true;
}
