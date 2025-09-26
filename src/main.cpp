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

// This camera device instance is used only to initialize the camera and set
// firebase state with published reported states.
// Camera streaming is handled in camera_board_main.cpp uploaded
// to the camera board
CameraDevice camera("camera", MAIN_BOARD_MAC_ADDRESS);

struct_message cameraBoardData;
// callback that makes sure firebase state is synced after data is sent
// successfully to start/stop streaming camera board
void onDataSentToCameraBoard(const uint8_t *mac_addr,
                             esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  bool success = (status == ESP_NOW_SEND_SUCCESS);
  Serial.println(success ? "Delivery Success" : "Delivery Fail");
  if (success) {
    if (cameraBoardData.camera_action == 1) {
      camera.setState(true); // We dont want to actually turn on the camera here
    } else if (cameraBoardData.camera_action == 0) {
      camera.setState(false);
    }
  } else {
    Serial.println("Failed to send camera command");
    // TODO Have error code for firebase
  }
}

void setup() {
  Serial.begin(115200);
  // Enable for detailed debug output (for when the gremlins strike)
  // Serial.setDebugOutput(true);
  // esp_log_level_set("*", ESP_LOG_VERBOSE);
  Serial.println("Sensors and devices initializing...");
  // TODO Check emmissivity setting and tune it with input here
  mlxSensor.begin();
  aht20Sensor.begin();
  heatLamp.begin();
  roomLight.begin();
  WiFiUtil::connectAndSyncTime(true);
  WiFiUtil::setupEspNow(
      false, nullptr,
      onDataSentToCameraBoard); // This file is uploaded to the main board

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

  // Allow time to be accessed in the rest of the loop
  struct tm timeInfo;
  char timeBuffer[20];

  // Run the rest of the period tasks every 1 second
  if (now - lastDeviceLoopUpdate >= 1000) {
    lastDeviceLoopUpdate = now;
    WiFiUtil::maintain(); // Keep Wi-Fi alive

    roomLight.update();

    // Read time from NTP
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
      heatLamp.update(objectTemp);
      firebaseApp.setValue("sensors/MLX90614/reported/ambientTempF",
                           ambientTemp);
      firebaseApp.setValue("sensors/MLX90614/reported/objectTempF", objectTemp);
      firebaseApp.setValue("sensors/lastUpdateTime", timeBuffer);
    } else {
      Serial.println("Failed to read MLX sensor");
    }
  }

  // Publish states every 3 seconds - Seems stable compared to this in 1s loop
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
      firebaseApp.setValue("sensors/lastUpdateTime", timeBuffer);

    } else {
      Serial.println("Failed to read sensor");
    }
  }
}