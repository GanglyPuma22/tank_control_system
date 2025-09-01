#include "devices/CameraDevice.h"
#include "esp_log.h"
#include "utils/firebase/FirebaseWrapper.h"
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiClientSecure.h>
#include <devices/HeatLamp.h>
#include <devices/Light.h>
#include <sensors/DHT11Sensor.h>
#include <utils/TimeOfDay.h>
#include <utils/WiFiUtil.h>

// Async web server setup for serving camere snapshots / MJPEG stream
AsyncWebServer server(80);
// create an easy-to-use handler
static AsyncWebSocketMessageHandler wsHandler;
// add it to the websocket server
static AsyncWebSocket streamWebSocket("/stream", wsHandler.eventHandler());
TaskHandle_t handleCaptureTask;

// Firebase setup using FirebaseClient class wrapper
FirebaseWrapper firebaseApp(FIREBASE_WEB_API_KEY, FIREBASE_USER_EMAIL,
                            FIREBASE_USER_PASSWORD, FIREBASE_DATABASE_URL);

//**************
// This implentation uses the M5Stamp C3 board with the Arduino framework.
// Certain pins selections assume defaults for the M5Stamp C3 board.
// If you are using a different board, please adjust the pin numbers
// accordingly.
//***** */
#define DHTTYPE DHT11
constexpr size_t MAXIMUM_NUMBER_OF_CLIENTS = 1; // Max number of WS clients

// Pin definitions
constexpr uint8_t DHT_PIN = 18;
constexpr uint8_t HEAT_LAMP_PIN = 0; // Pin for heat lamp relay
constexpr uint8_t LIGHT_PIN = 1;     // Pin for light relay

// Temp thresholds
constexpr float HEAT_LAMP_ON_TEMP_F =
    80.0f; // should be 66 but higher for testing relay
constexpr float HEAT_LAMP_OFF_TEMP_F = 100.0f;

DHTSensor dht11Sensor(DHT_PIN, DHTTYPE);
HeatLamp heatLamp("heatLamp", HEAT_LAMP_PIN, HEAT_LAMP_ON_TEMP_F,
                  HEAT_LAMP_OFF_TEMP_F);
Light roomLight("lights", LIGHT_PIN, TimeOfDay(0, 0),
                TimeOfDay(23, 59)); // Lights on from 7:30AM to 8PM

// IMPORTANT FINDS
// With TCP binarySendAll using entire buffer I cant get better than 5 fps seems
// With TCP chunking there are some issues getting the queue to not fill
// TRY GETTING UDP METRICS NEXT -> How fast can we take images
// Try camera metrics too, how fast can i take images for realzies
// With UDP I can also only squeeze out 5fps, this might be the camera
// limitation at the moment. But now its stable and can stream low latency
// TODO Try to see if we get a few more fps by using a seperate queue to send
// images vs take to maximize camera throughput. Camera definition Desired rate
// of image frames sending to websocket
constexpr int8_t framesPerSecond = 5;
// Task priority for camera capture task
constexpr int8_t cameraTaskPriority = 8;
uint8_t camPins[6] = {5, 4, 7, 6, 8, 9}; // CS, SCK, MISO, MOSI, SDA, SCL
CameraDevice camera("camera", camPins, CameraDevice::Resolution::QVGA,
                    &streamWebSocket, framesPerSecond, cameraTaskPriority);

void setup() {
  Serial.begin(115200);
  // Enable for detailed debug output (for when the gremlins strike)
  // Serial.setDebugOutput(true);
  // esp_log_level_set("*", ESP_LOG_VERBOSE);
  Serial.println("Sensors and devices initializing...");
  dht11Sensor.begin();
  heatLamp.begin();
  roomLight.begin();
  camera.begin();

  WiFiUtil::connectAndSyncTime();

  // TODO Move setup of AsyncWebServer to helper class
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hello from ESP32-C3");
  });

  wsHandler.onConnect([](AsyncWebSocket *server, AsyncWebSocketClient *client) {
    Serial.printf("Client %" PRIu32 " connected\n", client->id());
    server->textAll("New client: " + String(client->id()));
    // Start streaming camera data to configured IP and Port in Credentials.h
    // Stream as soon as the one-at-a-time allowed client connects
    camera.startStreamTaskAsync();
  });

  wsHandler.onDisconnect([](AsyncWebSocket *server, uint32_t clientId) {
    Serial.printf("Client %" PRIu32 " disconnected\n", clientId);
    server->textAll("Client " + String(clientId) + " disconnected");
  });
  wsHandler.onError([](AsyncWebSocket *server, AsyncWebSocketClient *client,
                       uint16_t errorCode, const char *reason, size_t len) {
    Serial.printf("Client %" PRIu32 " error: %" PRIu16 ": %s\n", client->id(),
                  errorCode, reason);
  });

  wsHandler.onMessage([](AsyncWebSocket *server, AsyncWebSocketClient *client,
                         const uint8_t *data, size_t len) {
    Serial.printf("Client %" PRIu32 " data: %s\n", client->id(),
                  (const char *)data);
  });

  // allow only one connection at a time
  server.addHandler(&streamWebSocket)
      .addMiddleware([](AsyncWebServerRequest *request, ArMiddlewareNext next) {
        if (streamWebSocket.count() >
            MAXIMUM_NUMBER_OF_CLIENTS - 1) { //-1 because streamWebSocket.counts
                                             // works like array indexing
          request->send(503, "text/plain", "Server is busy");
        } else {
          // process next middleware and at the end the handler
          next();
        }
      });

  server.addHandler(&streamWebSocket);
  server.begin();

  // Start firebase app with a stream path to listen for commands
  firebaseApp.begin("/devices");

  delay(1000); // Allow time for devices to initialize
  Serial.println("Initialization complete.!");
}

void loop() {
  unsigned long now = millis();

  firebaseApp.loop(); // Process Firebase app tasks

  // Run the rest of the period tasks every 1 second
  if (now % 1000 == 0) {
    WiFiUtil::maintain(); // Keep Wi-Fi alive

    // Read time from NTP
    struct tm time_info;
    char time_buffer[20];
    if (WiFiUtil::getLocalTimeWithDST(time_info)) {
      strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S",
               &time_info);
    }
    firebaseApp.setValue("/time", time_buffer);

    auto reading = dht11Sensor.readData();
    if (reading) {
      auto [temperatureF, humidity] = *reading;
      // Serial.printf("Temp: %.2f°C, humidityidity: %.2f%%\n", temperatureF,
      //               humidity);
      heatLamp.update(temperatureF);
      firebaseApp.setValue("/temperature", temperatureF);
      firebaseApp.setValue("/humidity", humidity);
    } else {
      Serial.println("Failed to read sensor");
    }
    roomLight.update();

    firebaseApp.publishReportedStates();

    streamWebSocket.cleanupClients();
  }
}