#include "esp_camera.h"
#include "esp_log.h"
#include <Arduino.h>
#include <WiFiUdp.h>
#include <esp_now.h>
#include <utils/WiFiHelper.h>

//**************
// This implentation uses the esp32-cam dev board with the espressif +
// Arduino framework. Certain pins selections assume defaults for the esp32-cam
// dev board. If you are using a different board, please adjust the pin numbers
// and use of camera accordingly.
//***** */
// TODO Make sure we can OTA update before building box
int target_fps = 5; // Adjust this value to change FPS
unsigned long frame_interval_ms = 1000 / target_fps;
const size_t CHUNK_SIZE = 1024; // Size of each UDP packet chunk

WiFiHelper wifi;
WiFiUDP udp;
unsigned long lastFrameTime = 0; // Add this variable to track timing
bool shouldBeStreaming = false;

// callback function that will be executed when data is received from main board
camera_message mainBoardData;
void cameraBoardOnDataRecv(const uint8_t *mac, const uint8_t *incomingData,
                           int len) {
  memcpy(&mainBoardData, incomingData, sizeof(mainBoardData));
  // Serial.print("Bytes received: ");
  // Serial.println(len);
  // Serial.print("Message: ");
  // Serial.println(mainBoardData.message);
  // Serial.print("Camera Action: ");
  // Serial.println(mainBoardData.camera_action);
  // Serial.print("FPS: ");
  // Serial.println(mainBoardData.fps);

  if (mainBoardData.camera_action == 1) {
    // Turn on camera
    shouldBeStreaming = true;
  } else if (mainBoardData.camera_action == 0) {
    // Turn off camera
    shouldBeStreaming = false;
  }
  if (mainBoardData.fps > 0 && mainBoardData.fps <= 30) {
    target_fps = mainBoardData.fps;
    frame_interval_ms = 1000 / target_fps;
  }
  // TODO process message for setting changes
}

void captureAndTransmitFrame() {
  // capture a frame
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    // Serial.println("Frame buffer could not be acquired");
  }

  size_t remaining = fb->len;
  while (remaining) {
    size_t toRead = min(CHUNK_SIZE, remaining);
    udp.beginPacket(VIDEO_WEB_SERVER_IP, VIDEO_WEB_SERVER_PORT);
    udp.write(fb->buf + (fb->len - remaining), toRead);
    udp.endPacket();
    remaining -= toRead;
    delay(1); // give TCP stack a breather
  }
  // return the frame buffer back to be reused
  esp_camera_fb_return(fb);
}
void setup() {
  Serial.begin(115200);
  // Enable for detailed debug output (for when the gremlins strike)
  // Serial.setDebugOutput(true);
  // esp_log_level_set("*", ESP_LOG_VERBOSE);
  // True to setup OTA updates, false to skip time syncing
  wifi.connectAndSyncTime(true, false);
  wifi.setupEspNow(true, cameraBoardOnDataRecv);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sccb_sda = 26; // Changed from pin_sscb_sda
  config.pin_sccb_scl = 27; // Changed from pin_sscb_scl
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 15;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.fb_count = 2;

  if (esp_camera_init(&config) != ESP_OK) {
    // Serial.println("Camera init failed");
    return;
  }
  // Serial.printf("PSRAM size: %d bytes\n", ESP.getPsramSize());

  // Serial.println("Camera initialized successfully");
  delay(1000); // Allow time for devices to initialize
  // Serial.println("Initialization complete.!");
}

int fpsCounter = 0;
unsigned long fpsTimer = 0;
void loop() {
  // Maintain WiFi connection and handles OTA updates
  wifi.maintain();

  if (!shouldBeStreaming) {
    return; // Not streaming, skip the rest of the loop
  }

  // Check if enough time has passed since last frame
  unsigned long currentTime = millis();
  if (currentTime - lastFrameTime < frame_interval_ms) {
    return; // Not time for next frame yet
  }

  captureAndTransmitFrame();
  lastFrameTime = currentTime;

  // Update FPS counter
  fpsCounter++;
  if (currentTime - fpsTimer >= 1000) {
    // Serial.print("FPS: ");
    // Serial.println(fpsCounter);
    fpsCounter = 0;
    fpsTimer = currentTime;
  }

  delay(10); // Small delay to avoid overwhelming the loop
}