#pragma once
#include <Arduino.h>

#include "Device.h"
#include "memorysaver.h" // needed for OV2640
#include <ArduCAM.h>
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
  CameraDevice(const uint8_t pins[6], Resolution res = Resolution::VGA)
      : csPin(pins[0]), sckPin(pins[1]), misoPin(pins[2]), mosiPin(pins[3]),
        sdaPin(pins[4]), sclPin(pins[5]), state(false), resolution(res),
        camera(OV2640, pins[0]), jpegBuffer(nullptr) {}

  ~CameraDevice() {
    if (jpegBuffer) {
      delete[] jpegBuffer;
      jpegBuffer = nullptr;
    }
  }

  void begin() override {
    Serial.println("Camera begin: setting pinMode");
    pinMode(csPin, OUTPUT);

    Serial.printf("I2C begin on SDA=%d, SCL=%d...\n", sdaPin, sclPin);
    Serial.flush();
    delay(10);

    Wire.begin(sdaPin, sclPin);

    Serial.printf("SPI begin on SCK=%d, MISO=%d, MOSI=%d, CS=%d...\n", sckPin,
                  misoPin, mosiPin, csPin);
    Serial.flush();
    delay(10);

    SPI.begin(sckPin, misoPin, mosiPin, csPin);

    camera.CS_HIGH();
    camera.set_format(JPEG);

    Serial.println("Checking I2C device at camera address...");
    Wire.beginTransmission(0x30); // OV2640 default
    if (Wire.endTransmission() != 0) {
      Serial.println("ERROR: Camera I2C not found. Check SDA/SCL wiring.");
      return;
    }

    Serial.println("InitCAM...");
    camera.InitCAM();
    Serial.println("InitCAM done");

    camera.clear_fifo_flag();
    setResolution(resolution);

    Serial.println("Camera initialized OK");
    state = true;
  }

  void update() override {
    // No periodic updates needed
  }

  void turnOn() override { state = true; }
  void turnOff() override { state = false; }
  bool isOn() const override { return state; }

  void capture(WebServer &server) {
    if (!state) {
      server.send(503, "text/plain", "Camera is off");
      return;
    }

    camera.flush_fifo();
    camera.clear_fifo_flag();
    camera.start_capture();
    while (!camera.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK))
      delay(1);

    size_t len = camera.read_fifo_length();
    if (len == 0) {
      server.send(500, "text/plain", "Capture failed");
      return;
    }

    constexpr size_t MAX_JPEG_BUFFER = 32768;
    if (len > MAX_JPEG_BUFFER) {
      server.send(500, "text/plain", "Image too large for buffer");
      return;
    }

    camera.CS_LOW();
    camera.set_fifo_burst();
    SPI.transferBytes(jpegBuffer, jpegBuffer, len);
    camera.CS_HIGH();

    server.sendHeader("Content-Type", "image/jpeg");
    server.sendHeader("Content-Length", String(len));
    server.send(200);

    WiFiClient client = server.client();
    const size_t chunkSize = 1024; // 1 KB chunks
    size_t sent = 0;
    while (sent < len) {
      size_t toSend = min(chunkSize, len - sent);
      size_t written = client.write(jpegBuffer + sent, toSend);
      if (written == 0) {
        Serial.println("Client disconnected early");
        break;
      }
      sent += written;
    }
  }

  void stream(WebServer &server, const char *mjpegBoundary, bool streaming) {
    if (!state) {
      server.send(503, "text/plain", "Camera is off");
      return;
    }

    WiFiClient client = server.client();
    if (!client)
      return;

    streaming = true;
    client.print("HTTP/1.1 200 OK\r\n");
    client.print("Content-Type: multipart/x-mixed-replace; boundary=");
    client.print(mjpegBoundary);
    client.print("\r\n\r\n");

    while (client.connected()) {
      camera.flush_fifo();
      camera.clear_fifo_flag();
      camera.start_capture();
      while (!camera.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK))
        delay(1);

      size_t len = camera.read_fifo_length();
      if (len == 0)
        continue;

      camera.CS_LOW();
      camera.set_fifo_burst();

      client.print("--");
      client.print(mjpegBoundary);
      client.print("\r\n");
      client.print("Content-Type: image/jpeg\r\n");
      client.print("Content-Length: ");
      client.print(len);
      client.print("\r\n\r\n");

      const size_t CHUNK_SIZE = 1024;
      uint8_t buf[CHUNK_SIZE];
      size_t remaining = len;
      while (remaining) {
        size_t toRead = min(CHUNK_SIZE, remaining);
        SPI.transferBytes(buf, buf, toRead);
        client.write(buf, toRead);
        remaining -= toRead;
        delay(1); // give TCP stack a breather
      }

      camera.CS_HIGH();
      client.print("\r\n");

      // Small delay to prevent watchdog resets
      delay(50);
    }

    streaming = false;
  }

  void setResolution(Resolution res) {
    resolution = res;
    switch (res) {
    case Resolution::QVGA:
      camera.OV2640_set_JPEG_size(OV2640_320x240);
      break;
    case Resolution::VGA:
      camera.OV2640_set_JPEG_size(OV2640_640x480);
      break;
    case Resolution::SVGA:
      camera.OV2640_set_JPEG_size(OV2640_800x600);
      break;
    case Resolution::XGA:
      camera.OV2640_set_JPEG_size(OV2640_1024x768);
      break;
    case Resolution::FULL:
      camera.OV2640_set_JPEG_size(OV2640_1600x1200);
      break;
    }
  }

private:
  uint8_t csPin, sckPin, misoPin, mosiPin, sdaPin, sclPin;
  bool state;
  Resolution resolution;
  ArduCAM camera;

  uint8_t *jpegBuffer; // fixed heap buffer
};
