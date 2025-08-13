#include "utils/WebServerUtil.h" // The server code
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <devices/HeatLamp.h>
#include <devices/Light.h>
#include <sensors/DHT11Sensor.h>
#include <utils/TimeOfDay.h>
#include <utils/WiFiUtil.h>

#define DHTTYPE DHT11

// Pin definitions
constexpr uint8_t DHT_PIN = 4;
constexpr uint8_t HEAT_LAMP_PIN = 7; // Pin for heat lamp relay
constexpr uint8_t LIGHT_PIN = 6;

// Temp thresholds
constexpr float HEAT_LAMP_ON_TEMP_F = 66.0f;
constexpr float HEAT_LAMP_OFF_TEMP_F = 100.0f;

DHTSensor dht11Sensor(DHT_PIN, DHTTYPE);
HeatLamp heatLamp(HEAT_LAMP_PIN, HEAT_LAMP_ON_TEMP_F, HEAT_LAMP_OFF_TEMP_F);
Light roomLight(LIGHT_PIN, TimeOfDay(7, 30),
                TimeOfDay(20, 0)); // Lights on from 7:30AM to 8PM

void setup() {
  Serial.begin(115200);
  Serial.println("Sensors and devices initializing...");
  dht11Sensor.begin();
  heatLamp.begin();
  roomLight.begin();

  WiFiUtil::connectAndSyncTime();
  // Handle all web server routes
  WebServerUtil::addRoute("/light/on", [&]() { roomLight.turnOn(); });
  WebServerUtil::addRoute("/light/off", [&]() { roomLight.turnOff(); });
  WebServerUtil::addRoute("/heatlamp/on", [&]() { heatLamp.turnOn(); });
  WebServerUtil::addRoute("/heatlamp/off", [&]() { heatLamp.turnOff(); });
  WebServerUtil::begin(); // Start web server

  delay(2000); // Allow time for devices to initialize
  Serial.println("Initialization complete.");
}

void loop() {
  unsigned long now = millis();

  // // Read time from NTP
  // struct tm timeinfo;
  // if (WiFiUtil::getLocalTimeWithDST(timeinfo)) {
  //   char buffer[20];
  //   strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &timeinfo);
  //   Serial.print("Local time: ");
  //   Serial.println(buffer);
  // }

  // Handle web server requests as often as possible
  WebServerUtil::handleClient();

  // Run the rest of the period tasks every 1 second
  if (now % 1000 == 0) {
    WiFiUtil::maintain(); // Keep Wi-Fi alive

    auto reading = dht11Sensor.readData();
    if (reading) {
      auto [temperatureF, humidity] = *reading;
      Serial.printf("Temp: %.2f°C, humidityidity: %.2f%%\n", temperatureF,
                    humidity);
      heatLamp.update(temperatureF);
      //  Serial.printf("Heat Lamp State: %s\n", heatLamp.isOn() ? "ON" :
      //  "OFF");
    } else {
      Serial.println("Failed to read sensor");
    }
    roomLight.update();
  }
}