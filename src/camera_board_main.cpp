#include "devices/CameraDevice.h"
#include "esp_log.h"
#include <Arduino.h>
#include <config/Credentials.h>
#include <esp_now.h>
#include <utils/WiFiUtil.h>

//**************
// This implentation uses the M5Stamp C3 board with the Arduino framework.
// Certain pins selections assume defaults for the M5Stamp C3 board.
// If you are using a different board, please adjust the pin numbers
// accordingly.
//***** */
#define SDA_PIN 8
#define SCL_PIN 9

// Camera frame rate for streaming
constexpr int8_t framesPerSecond = 5;
// Task priority for camera capture task
constexpr int8_t cameraTaskPriority = 3;
uint8_t camPins[6] = {5, 4,       7,
                      6, SDA_PIN, SCL_PIN}; // CS, SCK, MISO, MOSI, SDA, SCL
CameraDevice camera("camera", camPins, CameraDevice::Resolution::QVGA,
                    framesPerSecond, cameraTaskPriority);

// callback function that will be executed when data is received from main board
struct_message mainBoardData;
void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&mainBoardData, incomingData, sizeof(mainBoardData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Message: ");
  Serial.println(mainBoardData.message);
  Serial.print("Camera Action: ");
  Serial.println(mainBoardData.camera_action);

  if (mainBoardData.camera_action == 1) {
    // Turn on camera
    camera.turnOn();
  } else if (mainBoardData.camera_action == 0) {
    // Turn off camera
    camera.turnOff();
  }
}

void setup() {
  Serial.begin(115200);
  // Enable for detailed debug output (for when the gremlins strike)
  // Serial.setDebugOutput(true);
  // esp_log_level_set("*", ESP_LOG_VERBOSE);
  camera.begin();
  // True to setup OTA updates, false to skip
  WiFiUtil::connectAndSyncTime(true);
  WiFiUtil::setupEspNow(true, onDataRecv);
  delay(1000); // Allow time for devices to initialize
  Serial.println("Initialization complete.!");
}

void loop() {

  // Handle OTA updates
  ArduinoOTA.handle();
  // Maintain WiFi connection
  WiFiUtil::maintain();
}