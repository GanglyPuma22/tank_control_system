#include <Arduino.h>

#include "esp_log.h"
#include "utils/WebServerUtil.h" // The server code
#include <Adafruit_Sensor.h>
#include <devices/HeatLamp.h>
#include <devices/Light.h>
#include <sensors/DHT11Sensor.h>
#include <utils/TimeOfDay.h>
#include <utils/WiFiUtil.h>

#define DHTTYPE DHT11
//**************
// This implentation uses the M5Stamp C3 board with the Arduino framework.
// Certain pins selections assume defaults for the M5Stamp C3 board.
// If you are using a different board, please adjust the pin numbers
// accordingly.
//***** */
// Pin definitions
constexpr uint8_t DHT_PIN = 18;
constexpr uint8_t HEAT_LAMP_PIN = 0; // Pin for heat lamp relay
constexpr uint8_t LIGHT_PIN = 1;     // Pin for light relay

// Temp thresholds
constexpr float HEAT_LAMP_ON_TEMP_F =
    80.0f; // should be 66 but higher for testing relay
constexpr float HEAT_LAMP_OFF_TEMP_F = 100.0f;

DHTSensor dht11Sensor(DHT_PIN, DHTTYPE);
HeatLamp heatLamp("Heat Lamp", HEAT_LAMP_PIN, HEAT_LAMP_ON_TEMP_F,
                  HEAT_LAMP_OFF_TEMP_F);
Light roomLight("Daytime Lights", LIGHT_PIN, TimeOfDay(0, 0),
                TimeOfDay(23, 59)); // Lights on from 7:30AM to 8PM

// Camera definition
uint8_t camPins[6] = {5, 4, 7, 6, 8, 9}; // CS, SCK, MISO, MOSI, SDA, SCL
CameraDevice camera(camPins, CameraDevice::Resolution::QVGA);
unsigned long intervalMs = 30000; // 1-second refresh

// Recommended Approach: Firebase Storage + Realtime Database: The best practice
// for handling images with Firebase is to: Upload the image to Firebase
// Storage: Firebase Storage is built for storing large binary files like
// images, videos, and audio. It offers scalability, reliability, and security
// features. Store the image URL in the Realtime Database: Once the image is
// uploaded to Firebase Storage, you'll receive a downloadable URL. Store this
// URL in your Realtime Database, along with other relevant metadata (timestamp,
// tank ID, etc.). Use the URL for display: Your Firebase web page can then
// retrieve the URL from the Realtime Database and display the image directly
// from Firebase Storage.

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  Serial.println("Sensors and devices initializing...");
  dht11Sensor.begin();
  heatLamp.begin();
  roomLight.begin();
  camera.begin();

  WiFiUtil::connectAndSyncTime();

  WebServerUtil::begin(&camera, intervalMs); // Start web server

  delay(2000); // Allow time for devices to initialize
  Serial.println("Initialization complete.");
}

void loop() {
  unsigned long now = millis();

  // // Read time from NTP
  // struct tm timeinfo;
  // if (WiFiUtil::getLocalTimeWithDST(timeinfo)) {
  //   char buffer[20];
  //   strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &timeinfo);
  //   Serial.print("Local time: ");
  //   Serial.println(buffer);
  // }

  //  Handle web server requests as often as possible
  WebServerUtil::handleClient();

  // Run the rest of the period tasks every 1 second
  if (now % 1000 == 0) {
    WiFiUtil::maintain(); // Keep Wi-Fi alive

    auto reading = dht11Sensor.readData();
    if (reading) {
      auto [temperatureF, humidity] = *reading;
      Serial.printf("Temp: %.2f°C, humidityidity: %.2f%%\n", temperatureF,
                    humidity);
      heatLamp.update(temperatureF);
    } else {
      Serial.println("Failed to read sensor");
    }
    roomLight.update();
  }
}