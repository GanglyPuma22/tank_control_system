#pragma once
#include "../utils/MessageTypes.h"
#include "Device.h"
#define sensor_t camera_sensor_t
#include <esp_camera.h>
#undef sensor_t
#include <esp_now.h>

// This class encompases the management of the camera device from a state and
// communication perspective The actual camera board will run a separate
// firmware that receives commands from this class to start and stop streaming,
// and sends frames back via ESP-NOW and see camera_board_main.cpp

class CameraDevice : public Device {
public:
  CameraDevice(const std::string &name, const uint8_t macAddress[6]);
  void begin() override{};
  void update() override{};
  // Using override calls to start and stop streams
  void turnOn() override;
  void turnOff() override;
  void reportState(JsonDocument &doc) override;
  void applyState(JsonVariantConst desired) override;

private:
  uint8_t cameraBoardMacAddress[6];
  // Define constant messages for on/off commands
  static const struct_message CAMERA_ON_MESSAGE;
  static const struct_message CAMERA_OFF_MESSAGE;
};
