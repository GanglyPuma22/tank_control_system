#include "devices/CameraDevice.h"
#include "esp_log.h"
#include "utils/firebase/FirebaseWrapper.h"
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiClientSecure.h>
#include <devices/HeatLamp.h>
#include <devices/Light.h>
#include <sensors/DHT11Sensor.h>
#include <utils/TimeOfDay.h>
#include <utils/WiFiUtil.h>

// Async web server setup for serving camere snapshots / MJPEG stream
AsyncWebServer server(80);

// Firebase setup using FirebaseClient class wrapper
FirebaseWrapper firebase_app(FIREBASE_WEB_API_KEY, FIREBASE_USER_EMAIL,
                             FIREBASE_USER_PASSWORD, FIREBASE_DATABASE_URL);

//**************
// This implentation uses the M5Stamp C3 board with the Arduino framework.
// Certain pins selections assume defaults for the M5Stamp C3 board.
// If you are using a different board, please adjust the pin numbers
// accordingly.
//***** */
#define DHTTYPE DHT11

// Pin definitions
constexpr uint8_t DHT_PIN = 18;
constexpr uint8_t HEAT_LAMP_PIN = 0; // Pin for heat lamp relay
constexpr uint8_t LIGHT_PIN = 1;     // Pin for light relay

// Temp thresholds
constexpr float HEAT_LAMP_ON_TEMP_F =
    80.0f; // should be 66 but higher for testing relay
constexpr float HEAT_LAMP_OFF_TEMP_F = 100.0f;

DHTSensor dht11Sensor(DHT_PIN, DHTTYPE);
HeatLamp heatLamp("heatLamp", HEAT_LAMP_PIN, HEAT_LAMP_ON_TEMP_F,
                  HEAT_LAMP_OFF_TEMP_F);
Light roomLight("lights", LIGHT_PIN, TimeOfDay(0, 0),
                TimeOfDay(23, 59)); // Lights on from 7:30AM to 8PM

// Camera definition
uint8_t camPins[6] = {5, 4, 7, 6, 8, 9}; // CS, SCK, MISO, MOSI, SDA, SCL
CameraDevice camera("camera", camPins, CameraDevice::Resolution::QVGA);
unsigned long intervalMs = 30000; // 1-second refresh

void setup() {
  Serial.begin(115200);
  // Enable for detailed debug output (for when the gremlins strike)
  // Serial.setDebugOutput(true);
  // esp_log_level_set("*", ESP_LOG_VERBOSE);
  Serial.println("Sensors and devices initializing...");
  dht11Sensor.begin();
  heatLamp.begin();
  roomLight.begin();
  camera.begin();

  WiFiUtil::connectAndSyncTime();

  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hello from ESP32-C3");
  });

  server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request) {
    uint8_t *jpegBuffer = camera.capture_async();
    size_t jpegBufferLen = camera.get_fifo_length();
    Serial.printf("/stream jpegBuffer length: %u bytes\n", jpegBufferLen);

    if (!jpegBuffer || jpegBufferLen == 0) {
      request->send(500, "text/plain", "Camera capture failed");
      return;
    }

    request->send(200, "image/jpeg", jpegBuffer, jpegBufferLen);
  });

  server.begin();

  // Start firebase app with a stream path to listen for commands
  firebase_app.begin("/devices");

  delay(1000); // Allow time for devices to initialize
  Serial.println("Initialization complete.!");
}

void loop() {
  firebase_app.loop(); // Process Firebase app tasks

  unsigned long now = millis();

  // Run the rest of the period tasks every 1 second
  if (now % 1000 == 0) {
    WiFiUtil::maintain(); // Keep Wi-Fi alive

    // Read time from NTP
    struct tm time_info;
    char time_buffer[20];
    if (WiFiUtil::getLocalTimeWithDST(time_info)) {
      strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S",
               &time_info);
      Serial.print("Local time: ");
      Serial.println(time_buffer);
    }

    // float temperatureF, humidity;
    auto reading = dht11Sensor.readData();
    if (reading) {
      auto [temperatureF, humidity] = *reading;
      Serial.printf("Temp: %.2f°C, humidityidity: %.2f%%\n", temperatureF,
                    humidity);
      heatLamp.update(temperatureF);
      firebase_app.setValue("/temperature", temperatureF);
      firebase_app.setValue("/humidity", humidity);
    } else {
      Serial.println("Failed to read sensor");
    }
    roomLight.update();

    firebase_app.setValue("/time", time_buffer);

    // firebase_app.publishReportedStates();
  }
}