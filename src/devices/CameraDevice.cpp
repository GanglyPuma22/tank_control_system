#include "CameraDevice.h"

// Pin order: {CS, SCK, MISO, MOSI, SDA, SCL}
CameraDevice::CameraDevice(const std::string &name, const uint8_t pins[6],
                           Resolution res, AsyncWebSocket *ws, int8_t fps,
                           int8_t cameraTaskPriority)
    : Device(name), csPin(pins[0]), sckPin(pins[1]), misoPin(pins[2]),
      mosiPin(pins[3]), sdaPin(pins[4]), sclPin(pins[5]),
      camera_initialized(false), resolution(res), streamWebSocket(ws), fps(fps),
      cameraTaskPriority(cameraTaskPriority), camera(OV2640, pins[0]) {}

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

void CameraDevice::handleCaptureTaskAsync(void *param) {
  // Wait until capture is done
  while (!camera.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)) {
    vTaskDelay(100 / portTICK_RATE_MS);
  }

  frameReady = true;
  jpegBufferLen = camera.read_fifo_length();

  if (jpegBufferLen == 0 || jpegBufferLen > MAX_BUFFER_SIZE) {
    imageCorrupted = true;
    capturing = false;
    return;
  }

  imageCorrupted = false;
  capturing = false;
  camera.CS_LOW();
  camera.set_fifo_burst();
  SPI.transfer(jpegBuffer, jpegBufferLen);
  camera.CS_HIGH();
  Serial.println("🌟 Image capture task completed! 🌟");

  vTaskDelete(NULL);
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
      SPI.transfer(jpegBuffer, jpegBufferLen);
      // TODO Replace this with chunk sending
      streamWebSocket->binaryAll(jpegBuffer, jpegBufferLen);
      camera.CS_HIGH();
    }
    // TODO Make this framerate configurable, but for now test going as fast as
    // possible
    vTaskDelayUntil(&xLastWakeTime, xPeriod);
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

void CameraDevice::capture_async() {
  if (capturing || !camera_initialized) {
    return;
  }

  camera.flush_fifo();
  camera.clear_fifo_flag();
  camera.start_capture();
  capturing = true;
  imageCorrupted = false;
  frameReady = false;
  jpegBufferLen = 0;

  // Start FreeRTOS task to handle capture asynchronously
  // So we are not blocking main loop waiting for image to be done
  Serial.println("Starting async image capture task 🦞");
  xTaskCreate(
      [](void *params) {
        static_cast<CameraDevice *>(params)->handleCaptureTaskAsync(params);
      },
      "CameraCaptureTask", 4096, this, 1, &handleCaptureTask);
}

void CameraDevice::capture() { // fromerly used for the WebServerUtil&server
  if (!camera_initialized) {
    return;
  }

  camera.flush_fifo();
  camera.clear_fifo_flag();
  camera.start_capture();
  while (!camera.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK))
    delay(1);

  frameReady = true;
  jpegBufferLen = camera.read_fifo_length();

  if (jpegBufferLen == 0 || jpegBufferLen > MAX_BUFFER_SIZE) {
    imageCorrupted = true;
    capturing = false;
    return;
  }

  Serial.println("Done with syncronous capture 🌟 ");
  imageCorrupted = false;
  capturing = false;
  camera.CS_LOW();
  camera.set_fifo_burst();
  SPI.transfer(jpegBuffer, jpegBufferLen);
  camera.CS_HIGH();

  // server.sendHeader("Content-Type", "image/jpeg");
  // server.sendHeader("Content-Length", String(len));
  // server.send(200);

  // WiFiClient client = server.client();
  // const size_t chunkSize = 1024; // 1 KB chunks
  // size_t sent = 0;
  // while (sent < len) {
  //   size_t toSend = min(chunkSize, len - sent);
  //   size_t written = client.write(jpegBuffer + sent, toSend);
  //   if (written == 0) {
  //     Serial.println("Client disconnected early");
  //     break;
  //   }
  //   sent += written;
  // }
}

// void CameraDevice::stream(WebServer &server, const char *mjpegBoundary,
//                           bool streaming) {
//   Serial.println("Starting MJPEG stream");
//   if (!camera_initialized) {
//     server.send(503, "text/plain", "Camera is off");
//     return;
//   }

//   WiFiClient client = server.client();
//   if (!client)
//     return;

//   streaming = true;
//   client.print("HTTP/1.1 200 OK\r\n");
//   client.print("Content-Type: multipart/x-mixed-replace; boundary=");
//   client.print(mjpegBoundary);
//   client.print("\r\n\r\n");

//   while (client.connected()) {
//     camera.flush_fifo();
//     camera.clear_fifo_flag();
//     camera.start_capture();
//     while (!camera.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK))
//       delay(1);

//     size_t len = camera.read_fifo_length();
//     if (len == 0)
//       continue;

//     camera.CS_LOW();
//     camera.set_fifo_burst();

//     client.print("--");
//     client.print(mjpegBoundary);
//     client.print("\r\n");
//     client.print("Content-Type: image/jpeg\r\n");
//     client.print("Content-Length: ");
//     client.print(len);
//     client.print("\r\n\r\n");

//     const size_t CHUNK_SIZE = 1024;
//     uint8_t buf[CHUNK_SIZE];
//     size_t remaining = len;
//     while (remaining) {
//       size_t toRead = min(CHUNK_SIZE, remaining);
//       SPI.transferBytes(buf, buf, toRead);
//       client.write(buf, toRead);
//       remaining -= toRead;
//       delay(1); // give TCP stack a breather
//     }

//     camera.CS_HIGH();
//     client.print("\r\n");

//     // Small delay to prevent watchdog resets
//     delay(50);
//   }

//   streaming = false;
// }

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