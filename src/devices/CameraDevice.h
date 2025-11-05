#pragma once
#include "../config/Credentials.h"
#include "../utils/MessageTypes.h"
#include "Device.h"
#include <esp_now.h>

// Conflicting sensor_t definition between esp_camera and arduinojson
#define sensor_t camera_sensor_t
#include <esp_camera.h>
#undef sensor_t

// This class encompases the management of the camera device from a state and
// communication perspective The actual camera board will run a separate
// firmware that receives commands from this class to start and stop streaming,
// and sends frames back via ESP-NOW and see camera_board_main.cpp

class CameraDevice : public Device {
public:
  CameraDevice(const std::string &name); //, const uint8_t macAddress[6]);
  void begin() override{};
  void update() override;
  // Using override calls to start and stop streams
  void turnOn() override;
  void turnOff() override;
  void reportState(JsonDocument &doc) override;
  void applyState(JsonVariantConst desired) override;
  void logState(FirebaseMapValue &map) override;
  bool shouldBeOn() { return shouldBeOnState; }
  void setErrorState(bool state) { errorState = state; }
  bool hasError() { return errorState; }
  int getFps() { return fps; }
  void setFps(int newFps) { fps = newFps; }

  void setCameraMessage(String message, int camera_action, int fps);

private:
  // Used to track desired state from Firebase database
  bool shouldBeOnState = false;
  // Used to track if there was an error sending command to camera board
  bool errorState = false;
  int fps = 5;
  // Initialize memory to send all commands for camera control
  camera_message cameraMessage;

  // ESP-NOW retry variables
  bool currentlySendingEspNowCommand = false;
  int currentRetryCount = 0;
  static constexpr int MAX_ESP_NOW_RETRIES = 5;

  bool attemptSend(const camera_message &message);
};
