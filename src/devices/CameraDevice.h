#pragma once

#include <Arduino.h>

#include "Device.h"
#include "memorysaver.h" // needed for OV2640 //THIS NEEDS TO BE SET IN ARDUCAM LIBRARY
#include <ArduCAM.h>

// Undefine ArduCAM's swap macro to prevent conflicts with std::swap
#ifdef swap
#undef swap
#endif

#include "../config/Credentials.h"
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <algorithm>

class CameraDevice : public Device {
public:
  enum class Resolution {
    VGA, // default
    QVGA,
    SVGA,
    XGA,
    FULL
  };

  // Pin order: {CS, SCK, MISO, MOSI, SDA, SCL}
  CameraDevice(const std::string &name, const uint8_t pins[6],
               Resolution res = Resolution::VGA, int8_t fps = 5,
               int8_t cameraTaskPriority = 8); // UDP priority 18

  static constexpr size_t MAX_BUFFER_SIZE =
      40 * 1024; // TODO Make this configurable

  WiFiUDP udp;

  void begin() override;
  void update() override;
  void turnOn() override;
  void turnOff() override;
  void reportState(JsonDocument &doc) override;
  void applyState(JsonVariantConst desired) override;
  void setResolution(Resolution res);
  uint8_t *getJpegBuffer() { return jpegBuffer; }
  size_t getJpegBufferLen() { return jpegBufferLen; }
  bool isCapturing() const { return capturing; }
  bool isFrameReady() const { return frameReady; }
  void setFrameReady(bool ready) { frameReady = ready; }
  bool isImageCorrupted() const { return imageCorrupted; }
  void startStreamTaskAsync();
  void handleStreamTaskAsync(void *params);

private:
  int8_t fps;
  int8_t cameraTaskPriority;
  const uint8_t csPin, sckPin, misoPin, mosiPin, sdaPin, sclPin;
  bool camera_initialized;
  Resolution resolution;
  ArduCAM camera;
  volatile bool capturing = false;
  volatile bool frameReady = false;
  volatile bool imageCorrupted = false;
  uint8_t jpegBuffer[MAX_BUFFER_SIZE];
  volatile size_t jpegBufferLen = 0;
  TaskHandle_t handleStreamTask;
  TickType_t xLastWakeTime;
  TickType_t xPeriod;
};
