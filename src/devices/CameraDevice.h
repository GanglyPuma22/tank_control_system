#pragma once

#include <Arduino.h>

#include "Device.h"
#include "memorysaver.h" // needed for OV2640 //THIS NEEDS TO BE SET IN ARDUCAM LIBRARY
#include <ArduCAM.h>

// Undefine ArduCAM's swap macro to prevent conflicts with std::swap
#ifdef swap
#undef swap
#endif

#include <SPI.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>

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
               Resolution res = Resolution::VGA);
  ~CameraDevice();

  void begin() override;
  void update() override;
  void turnOn() override;
  void turnOff() override;
  bool isOn() const override;
  void reportState(JsonDocument &doc) override;
  void applyState(JsonVariantConst desired) override;
  void capture(WebServer &server);
  uint8_t *capture_async();
  size_t get_fifo_length();
  void stream(WebServer &server, const char *mjpegBoundary, bool streaming);
  void setResolution(Resolution res);

private:
  uint8_t csPin, sckPin, misoPin, mosiPin, sdaPin, sclPin;
  bool state;
  Resolution resolution;
  ArduCAM camera;

  uint8_t *jpegBuffer; // fixed heap buffer
};
