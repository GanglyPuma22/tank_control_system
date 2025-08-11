#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <devices/HeatLamp.h>
#include <devices/Light.h>
#include <sensors/DHT11Sensor.h>

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
Light roomLight(LIGHT_PIN, 7, 19); // Lights on from 7AM to 7PM

void setup() {
  Serial.begin(115200);

  Serial.println("Sensors and devices initializing...");
  dht11Sensor.begin();
  heatLamp.begin();
  roomLight.begin();
  delay(2000); // Allow time for devices to initialize
  Serial.println("Initialization complete.");
}

void loop() {

  auto reading = dht11Sensor.readData();
  if (reading) {
    auto [temperatureF, humidity] = *reading;
    Serial.printf("Temp: %.2f°C, humidityidity: %.2f%%\n", temperatureF,
                  humidity);
    heatLamp.update(temperatureF);
    //  Serial.printf("Heat Lamp State: %s\n", heatLamp.isOn() ? "ON" : "OFF");
  } else {
    Serial.println("Failed to read sensor");
  }
  roomLight.update();
  delay(1000); // Update every second
}