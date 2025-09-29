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

// TODO Cleanest way to do this here would be to have a time check for last
// retry and only retry every x seconds
void CameraDevice::update() {
  if (this->shouldBeOnState != this->isOn() &&
      this->currentlySendingEspNowCommand &&
      this->currentRetryCount < MAX_ESP_NOW_RETRIES) {
    this->currentRetryCount++;
    // Desired state differs from actual state, attempt to correct
    if (this->shouldBeOnState) {
      turnOn();
    } else {
      turnOff();
    }
  }

  if (this->currentRetryCount >= MAX_ESP_NOW_RETRIES) {
    this->currentlySendingEspNowCommand = false;
    this->setErrorState(true);
  }
}

bool CameraDevice::attemptSend(const struct_message &message) {
  esp_err_t result =
      esp_now_send(CAMERA_BOARD_MAC_ADDRESS, (const uint8_t *)&message,
                   sizeof(struct_message));

  if (result == ESP_OK) {
    return true;
  }

  return false;
}

// State will be set by onDataSent callback in main.cpp -> This guarantees
// camera state represents true state of camera board.
void CameraDevice::turnOn() { attemptSend(CAMERA_ON_MESSAGE); }
void CameraDevice::turnOff() { attemptSend(CAMERA_OFF_MESSAGE); }

void CameraDevice::applyState(JsonVariantConst desired) {
  if (desired["state"].is<JsonVariantConst>()) {
    this->shouldBeOnState = desired["state"].as<bool>();
    this->currentRetryCount = 0;
    this->currentlySendingEspNowCommand = true;
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
