#include "CameraDevice.h"

CameraDevice::CameraDevice(const std::string &name, const uint8_t macAddress[6])
    : Device(name) {
  memcpy(this->cameraBoardMacAddress, macAddress, 6);
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
  esp_err_t result =
      esp_now_send(this->cameraBoardMacAddress, (uint8_t *)&CAMERA_ON_MESSAGE,
                   sizeof(CAMERA_ON_MESSAGE));

  if (result != ESP_OK) {
    // TODO Document error state for publishing to firebase
    Serial.println("Error sending camera command");
  }
}
void CameraDevice::turnOff() {
  esp_err_t result =
      esp_now_send(this->cameraBoardMacAddress, (uint8_t *)&CAMERA_OFF_MESSAGE,
                   sizeof(CAMERA_OFF_MESSAGE));

  if (result != ESP_OK) {
    // TODO Document error state for publishing to firebase
    Serial.println("Error sending camera command");
  }
}

void CameraDevice::applyState(JsonVariantConst desired) {
  if (desired["state"].is<JsonVariantConst>()) {
    bool shouldBeOn = desired["state"].as<bool>();
    if (shouldBeOn) {
      turnOn();
    } else {
      turnOff();
    }
  }
}

void CameraDevice::reportState(JsonDocument &doc) {
  doc["state"] = this->isOn();
}
