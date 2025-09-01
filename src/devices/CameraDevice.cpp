#include "CameraDevice.h"

// Pin order: {CS, SCK, MISO, MOSI, SDA, SCL}
CameraDevice::CameraDevice(const std::string &name, const uint8_t pins[6],
                           Resolution res, AsyncWebSocket *ws, int8_t fps,
                           int8_t cameraTaskPriority)
    : Device(name), csPin(pins[0]), sckPin(pins[1]), misoPin(pins[2]),
      mosiPin(pins[3]), sdaPin(pins[4]), sclPin(pins[5]),
      camera_initialized(false), resolution(res), streamWebSocket(ws), fps(fps),
      cameraTaskPriority(cameraTaskPriority), camera(OV2640, pins[0]) {
  xPeriod = pdMS_TO_TICKS((1000 / (fps)));
}

void CameraDevice::begin() {
  Serial.println("Camera begin: setting pinMode");
  pinMode(csPin, OUTPUT);

  Wire.begin(sdaPin, sclPin);

  delay(10);

  SPI.begin(sckPin, misoPin, mosiPin, csPin);
  // Reset the CPLD
  camera.write_reg(0x07, 0x80);
  delay(100);
  camera.write_reg(0x07, 0x00);
  delay(100);

  camera.CS_HIGH();
  camera.set_format(JPEG);

  // TODO Add more of the checks from arducam example for camera safety
  Wire.beginTransmission(0x30); // OV2640 default
  if (Wire.endTransmission() != 0) {
    Serial.println("ERROR: Camera I2C not found. Check SDA/SCL wiring.");
    return;
  }

  camera.InitCAM();

  camera.clear_fifo_flag();
  setResolution(resolution);

  Serial.println("📸 Camera initialized 📸");
  camera_initialized = true;
}

void CameraDevice::update() {
  // No periodic updates needed
}

void CameraDevice::turnOn() { camera_initialized = true; }
void CameraDevice::turnOff() { camera_initialized = false; }
bool CameraDevice::isOn() const { return camera_initialized; }

void CameraDevice::applyState(JsonVariantConst desired) {
  if (desired["on"].is<JsonVariantConst>()) {
    turnOn();
  } else if (desired["off"].is<JsonVariantConst>()) {
    turnOff();
  }
}

void CameraDevice::reportState(JsonDocument &doc) {
  doc["camera_initialized"] = camera_initialized;
}

void CameraDevice::handleStreamTaskAsync(void *param) {
  // Set last wake time for accurate periodic delay
  xLastWakeTime = xTaskGetTickCount();
  while (true && streamWebSocket->count() >
                     0) { // Make sure to only stream if clients are connected

    if (!capturing) {
      camera.flush_fifo();
      camera.clear_fifo_flag();
      camera.start_capture();
      capturing = true;
      imageCorrupted = false;
      frameReady = false;
      jpegBufferLen = 0;
    }

    if (camera.get_bit(
            ARDUCHIP_TRIG,
            CAP_DONE_MASK)) { // TODO consider moving this to isFrameReady()
                              // function and removing frameReady bool

      frameReady = true;
      jpegBufferLen = camera.read_fifo_length();

      if (jpegBufferLen == 0 || jpegBufferLen > MAX_BUFFER_SIZE) {
        imageCorrupted = true;
        capturing = false;
        Serial.println("ERROR: Image corrupted during stream");
        continue;
      }
      imageCorrupted = false;
      capturing = false;
      camera.CS_LOW();
      camera.set_fifo_burst();
      const size_t CHUNK_SIZE = 1024;
      size_t bufferLen = jpegBufferLen;
      while (bufferLen > 0) {
        size_t toRead = min(CHUNK_SIZE, bufferLen);
        SPI.transfer(jpegBuffer, toRead);
        udp.beginPacket(GANGLYPUMA22_LAPTOP_IP, GANGLYPUMA22_LAPTOP_PORT);
        udp.write(jpegBuffer, toRead);
        udp.endPacket();
        bufferLen -= toRead;
      }

      camera.CS_HIGH();

      // Delay inside the capture section so that it only delays once for the
      // capture + send timing. If in main while loop it delays multiple times
      BaseType_t wasDelayed = xTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
  }
  vTaskDelete(NULL);
}

void CameraDevice::startStreamTaskAsync() {
  if (!camera_initialized) {
    Serial.println("ERROR: Camera not initialized - cannot start stream task");
    return;
  }
  Serial.println("Starting async stream task 🎥");
  // Use priority 16 for camera task - max priority is configMAX_PRIORITIES, 18
  // and above is usually system critical tasks
  xTaskCreate(
      [](void *params) {
        static_cast<CameraDevice *>(params)->handleStreamTaskAsync(params);
      },
      "CameraCaptureTask", 4096, this, cameraTaskPriority, &handleStreamTask);
}

void CameraDevice::setResolution(Resolution res) {
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