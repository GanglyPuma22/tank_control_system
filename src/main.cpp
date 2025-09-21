#include "devices/CameraDevice.h"
#include "esp_log.h"
#include "utils/firebase/FirebaseWrapper.h"
#include <Arduino.h>
#include <devices/HeatLamp.h>
#include <devices/Light.h>
#include <sensors/AHT20.h>
#include <sensors/MLX90614.h>
#include <utils/TimeOfDay.h>
#include <utils/WiFiUtil.h>

// Firebase setup using FirebaseClient class wrapper
FirebaseWrapper firebaseApp(FIREBASE_WEB_API_KEY, FIREBASE_USER_EMAIL,
                            FIREBASE_USER_PASSWORD, FIREBASE_DATABASE_URL);

//**************
// This implentation uses the M5Stamp C3 board with the Arduino framework.
// Certain pins selections assume defaults for the M5Stamp C3 board.
// If you are using a different board, please adjust the pin numbers
// accordingly.
//***** */
#define SDA_PIN 8
#define SCL_PIN 9

// Pin definitions
constexpr uint8_t HEAT_LAMP_PIN = 0; // Pin for heat lamp relay
constexpr uint8_t LIGHT_PIN = 1;     // Pin for light relay

// Temp thresholds
constexpr float HEAT_LAMP_ON_ABOVE_TEMP_F =
    80.0f; // should be 66 but higher for testing relay
constexpr float HEAT_LAMP_OFF_ABOVE_TEMP_F = 100.0f;

MLX90614 mlxSensor; // Uses default I2C pins 8 (SDA) and 9 (SCL)
AHT20 aht20Sensor;  // Uses default I2C pins 8 (SDA) and 9 (SCL)
HeatLamp heatLamp("heatLamp", HEAT_LAMP_PIN, HEAT_LAMP_ON_ABOVE_TEMP_F,
                  HEAT_LAMP_OFF_ABOVE_TEMP_F);
Light roomLight("lights", LIGHT_PIN, TimeOfDay(0, 0),
                TimeOfDay(23, 59)); // Lights on from 7:30AM to 8PM

// TODO Try to see if we get a few more fps by using a seperate queue to send
// images vs take to maximize camera throughput.

// Camera frame rate for streaming
constexpr int8_t framesPerSecond = 1;
// Task priority for camera capture task
constexpr int8_t cameraTaskPriority = 3;
uint8_t camPins[6] = {5, 4,       7,
                      6, SDA_PIN, SCL_PIN}; // CS, SCK, MISO, MOSI, SDA, SCL
CameraDevice camera("camera", camPins, CameraDevice::Resolution::QVGA,
                    framesPerSecond, cameraTaskPriority);

void setup() {
  Serial.begin(115200);
  // Enable for detailed debug output (for when the gremlins strike)
  Serial.setDebugOutput(true);
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  Serial.println("Sensors and devices initializing...");
  mlxSensor.begin();
  aht20Sensor.begin();
  heatLamp.begin();
  roomLight.begin();
  camera.begin();
  WiFiUtil::connectAndSyncTime();

  // Start firebase app with a stream path to listen for commands
  firebaseApp.begin("/devices");
  delay(1000); // Allow time for devices to initialize
  Serial.println("Initialization complete.!");
}

unsigned long lastDeviceLoopUpdate = 0;
unsigned long lastPublishedStateUpdate = 0;
unsigned long lastSensorUpdate = 0;
unsigned long lastSlowUpdate = 0;

void loop() {
  unsigned long now = millis();

  firebaseApp.loop(); // Process Firebase app tasks

  // Run the rest of the period tasks every 1 second
  if (now - lastDeviceLoopUpdate >= 1000) {
    lastDeviceLoopUpdate = now;
    WiFiUtil::maintain(); // Keep Wi-Fi alive

    roomLight.update();

    // Read time from NTP
    struct tm timeInfo;
    char timeBuffer[20];
    if (WiFiUtil::getLocalTimeWithDST(timeInfo)) {
      strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeInfo);
    }
    firebaseApp.setValue("/status/time", timeBuffer);
  }

  if (now - lastSensorUpdate >= 2000) {
    lastSensorUpdate = now;
    // Read sensors and update heat lamp state
    auto mlxReading = mlxSensor.readData();
    if (mlxReading) {
      auto [objectTemp, ambientTemp] = *mlxReading;
      // heatLamp.update(objectTemp);
      firebaseApp.setValue("sensors/MLX90614/reported/ambientTempF",
                           ambientTemp);
      firebaseApp.setValue("sensors/MLX90614/reported/objectTempF", objectTemp);
    } else {
      Serial.println("Failed to read MLX sensor");
    }
  }

  // Publish states every 3 seconds - Seems stable compared to this in 1s
  // loop
  if (now - lastPublishedStateUpdate >= 3000) {
    firebaseApp.publishReportedStates();
    lastPublishedStateUpdate = now;
  }

  if (now - lastSlowUpdate >= 5000) {
    lastSlowUpdate = now;
    auto reading = aht20Sensor.readData();
    if (reading) {
      auto [temperatureF, humidity] = *reading;
      firebaseApp.setValue("sensors/AHT20/reported/temperature", temperatureF);
      firebaseApp.setValue("sensors/AHT20/reported/humidity", humidity);
    } else {
      Serial.println("Failed to read sensor");
    }
  }
}