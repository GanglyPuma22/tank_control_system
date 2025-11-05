#include "devices/CameraDevice.h"
#include "esp_log.h"
#include "utils/firebase/FirebaseWrapper.h"
#include <Arduino.h>
#include <devices/HeatLamp.h>
#include <devices/Light.h>
#include <sensors/AHT20.h>
#include <sensors/MLX90614.h>
#include <utils/TimeOfDay.h>
#include <utils/WiFiHelper.h>

//**************
// This implentation uses the M5Stamp C3 board with the Arduino framework.
// Certain pins selections assume defaults for the M5Stamp C3 board.
// If you are using a different board, please adjust the pin numbers
// accordingly.
//***** */

// Credentials are coming from include in WifiUtil.h
// Firebase setup using FirebaseClient class wrapper
FirebaseWrapper firebaseApp(FIREBASE_WEB_API_KEY, FIREBASE_USER_EMAIL,
                            FIREBASE_USER_PASSWORD, FIREBASE_DATABASE_URL);
// Base path for all data in Firebase
const String BASE_PATH = "/tanks/yashatankid";

// Pin definitions
constexpr uint8_t HEAT_LAMP_PIN = 0; // Pin for heat lamp relay
constexpr uint8_t LIGHT_PIN = 1;     // Pin for light relay

// Temp thresholds
constexpr float HEAT_LAMP_ON_ABOVE_TEMP_F =
    80.0f; // should be 66 but higher for testing relay
constexpr float HEAT_LAMP_OFF_ABOVE_TEMP_F = 100.0f;
// Device instances
HeatLamp heatLamp("heatLamp", HEAT_LAMP_PIN, HEAT_LAMP_ON_ABOVE_TEMP_F,
                  HEAT_LAMP_OFF_ABOVE_TEMP_F);
Light roomLight("lights", LIGHT_PIN, TimeOfDay(0, 0),
                TimeOfDay(23, 59)); // Lights on from 7:30AM to 8PM

// Sensor instances
// Adjusted for grout surface of tanks
constexpr double MLX90614_EMISSIVITY = 0.94;
// They use default I2C pins 8 (SDA) and 9 (SCL)
MLX90614 mlxSensor(MLX90614_EMISSIVITY);
AHT20 aht20Sensor;

// This camera device instance is used for firebase state management
// Camera streaming is handled in camera_board_main.cpp running on the
// camera-board
CameraDevice camera("camera");

// callback that makes sure firebase state is synced after data is sent
// successfully to start/stop streaming camera board. This runs after esp-now
// data is processed by camera-board
void onDataSentToCameraBoard(const uint8_t *mac_addr,
                             esp_now_send_status_t status) {
  // Serial.print("\r\nLast Packet Send Status:\t");
  bool success = (status == ESP_NOW_SEND_SUCCESS);
  // Serial.println(success ? "Delivery Success" : "Delivery Fail");

  if (success) {
    camera.setErrorState(false);
    // Serial.println("Setting camera state");
    // We can now trust the command was received, so use desired state
    if (camera.shouldBeOn()) {
      camera.setState(true);
    } else {
      camera.setState(false);
    }
  } else {
    // Serial.println("Failed to send camera command");
    // TODO Have error code for firebase
  }
}

WiFiHelper wifi;

void setup() {
  Serial.begin(115200);
  // Enable for detailed debug output (for when the gremlins strike)
  // Serial.setDebugOutput(true);
  // esp_log_level_set("*", ESP_LOG_VERBOSE);
  // Serial.println("Sensors and devices initializing...");
  // TODO Check emmissivity setting and tune it with input here
  mlxSensor.begin();
  aht20Sensor.begin();
  heatLamp.begin();
  roomLight.begin();
  // wifi.setFirebaseWrapper(&firebaseApp); // Set the FirebaseWrapper
  wifi.connectAndSyncTime(true, true);
  wifi.setupEspNow(
      false, nullptr,
      onDataSentToCameraBoard); // This file is uploaded to the main board

  // Start firebase app with a stream path to listen for commands
  firebaseApp.begin((BASE_PATH + String("/devices")).c_str());
  delay(1000); // Allow time for devices to initialize
  // Serial.println("Initialization complete.!");
}

unsigned long lastDeviceLoopUpdate = 0;
unsigned long lastPublishedStateUpdate = 0;
unsigned long lastSensorUpdate = 0;
unsigned long lastSensorLogUpdate = 0;

void loop() {
  unsigned long now = millis();
  wifi.maintain();    // Keep Wi-Fi alive and handle OTA updates
  firebaseApp.loop(); // Process Firebase app tasks
  // Process camera state changes if any -> Done as fast as possible for esp-now
  camera.update();

  // Allow time to be accessed in the rest of the loop
  struct tm timeInfo;
  char timeBuffer[20];

  // Run the rest of the periodic tasks every 1 second
  if (now - lastDeviceLoopUpdate >= 1000) {
    lastDeviceLoopUpdate = now;
    roomLight.update();

    // Read time from NTP
    struct tm timeInfo;
    char timeBuffer[20];
    if (wifi.getLocalTimeWithDST(timeInfo)) {
      strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeInfo);
    }
    firebaseApp.setValue((BASE_PATH + String("/status/time")).c_str(),
                         timeBuffer);
  }

  // Publish states every 3 seconds - Seems stable compared to this in 1s loop
  if (now - lastPublishedStateUpdate >= 3000) {
    firebaseApp.publishReportedStates();
    lastPublishedStateUpdate = now;
  }

  if (now - lastSensorUpdate >= 5000) {
    lastSensorUpdate = now;
    // Compute a fresh timestamp string for lastUpdateTime
    struct tm sensorTimeInfo;
    char sensorTimeBuffer[20] = {0};
    auto aht20Reading = aht20Sensor.readData();
    auto mlxReading = mlxSensor.readData();

    if (aht20Reading) {
      auto [temperatureF, humidity] = *aht20Reading;
      // update heat lamp state
      heatLamp.update(temperatureF);

      firebaseApp.setValue(
          (BASE_PATH + String("/sensors/AHT20/reported/temperature")).c_str(),
          temperatureF);
      firebaseApp.setValue(
          (BASE_PATH + String("/sensors/AHT20/reported/humidity")).c_str(),
          humidity);
    }
    if (mlxReading) {
      auto [objectTemp, ambientTemp] = *mlxReading;
      firebaseApp.setValue(
          (BASE_PATH + String("/sensors/MLX90614/reported/ambientTempF"))
              .c_str(),
          ambientTemp);
      firebaseApp.setValue(
          (BASE_PATH + String("/sensors/MLX90614/reported/objectTempF"))
              .c_str(),
          objectTemp);
    }

    // Log sensor data every minute
    if (now - lastSensorLogUpdate >= 60000) {
      firebaseApp.logSensorEvent("MLX90614", mlxReading, "objectTempF",
                                 "ambientTempF");
      firebaseApp.logSensorEvent("AHT20", aht20Reading, "temperatureF",
                                 "humidity");
      lastSensorLogUpdate = now;
    }
  }
}