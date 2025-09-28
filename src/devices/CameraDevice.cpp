#include "CameraDevice.h"

CameraDevice::CameraDevice(
    const std::string &name) //, const uint8_t macAddress[6])
    : Device(name) {
  // memcpy(this->cameraBoardMacAddress, macAddress, 6);
}

const struct_message CameraDevice::CAMERA_ON_MESSAGE = {
    "CAMERA", // message
    1         // camera_action, 1 for ON
};

const struct_message CameraDevice::CAMERA_OFF_MESSAGE = {
    "CAMERA", // message
    0         // camera_action, 0 for OFF
};

// State will be set by onDataSent callback in main.cpp -> This guarantees
// camera state represents true state of camera board.
void CameraDevice::turnOn() {
  bool result = WiFiHelper::sendData(CAMERA_BOARD_MAC_ADDRESS,
                                     (uint8_t *)&CAMERA_ON_MESSAGE);

  if (!result) {
    this->errorState = true;
    Serial.println("Error sending camera command");
  }
}
void CameraDevice::turnOff() {
  bool result = WiFiHelper::sendData(CAMERA_BOARD_MAC_ADDRESS,
                                     (uint8_t *)&CAMERA_OFF_MESSAGE);

  if (!result) {
    this->errorState = true;
    Serial.println("Error sending camera command");
  }
}

void CameraDevice::applyState(JsonVariantConst desired) {
  if (desired["state"].is<JsonVariantConst>()) {
    this->shouldBeOnState = desired["state"].as<bool>();
    if (this->shouldBeOnState) {
      turnOn();
    } else {
      turnOff();
    }
  }
}

void CameraDevice::reportState(JsonDocument &doc) {
  doc["state"] = this->isOn();
  doc["error"] = this->hasError();
}
