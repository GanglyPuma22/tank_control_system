#include "CameraDevice.h"

CameraDevice::CameraDevice(
    const std::string &name) //, const uint8_t macAddress[6])
    : Device(name) {
  // memcpy(this->cameraBoardMacAddress, macAddress, 6);
}

void CameraDevice::setCameraMessage(String message, int camera_action,
                                    int fps) {

  strncpy(this->cameraMessage.message, message.c_str(),
          sizeof(this->cameraMessage.message) - 1);
  this->cameraMessage.message[sizeof(this->cameraMessage.message) - 1] =
      '\0'; // Ensure null-termination
  this->cameraMessage.camera_action = camera_action;
  this->cameraMessage.fps = fps;
}

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

bool CameraDevice::attemptSend(const camera_message &message) {
  esp_err_t result =
      esp_now_send(CAMERA_BOARD_MAC_ADDRESS, (const uint8_t *)&message,
                   sizeof(camera_message));

  if (result == ESP_OK) {
    return true;
  }

  return false;
}

// State will be set by onDataSent callback in main.cpp -> This guarantees
// camera state represents true state of camera board.
void CameraDevice::turnOn() {
  setCameraMessage("Camera On", 1, this->fps);
  attemptSend(this->cameraMessage);
}
void CameraDevice::turnOff() {
  setCameraMessage("Camera Off", 0, this->fps);
  attemptSend(this->cameraMessage);
}

void CameraDevice::applyState(JsonVariantConst desired) {
  // FPS needs to be set first since turnOn and turnOff use it
  if (desired["fps"].is<JsonVariantConst>()) {
    int newFps = desired["fps"].as<int>();
    if (newFps > 0 && newFps <= 30) {
      this->setFps(newFps);
      setCameraMessage("Camera FPS", 2, this->fps);
      attemptSend(this->cameraMessage);
    } else {
      Serial.println("Invalid FPS value received, must be between 1 and 30");
    }
  }
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
  doc["fps"] = this->getFps();
}

void CameraDevice::logState(FirebaseMapValue &map) {
  map.add("state", FirebaseBooleanValue(this->isOn()))
      .add("error", FirebaseBooleanValue(this->hasError()))
      .add("fps", FirebaseIntegerValue(this->getFps()));
}