#pragma once
#include "Sensor.h"
#include <DHT.h>

class DHTSensor : public Sensor {
public:
  DHTSensor(uint8_t pin, uint8_t type) : dht(pin, type) {}

  void begin() {
    dht.begin();
    this->initializeSuccessful();
    Serial.println("DHT Sensor initialized");
  }

  std::optional<std::tuple<float, float>> readData() override {
    if (!isInitialized()) {
      Serial.println("DHT Sensor not initialized");
      return std::nullopt;
    }
    float temp = dht.readTemperature(true); // true for Fahrenheit
    float hum = dht.readHumidity();
    if (isnan(temp) || isnan(hum)) {
      Serial.println("Failed to read DHT sensor");
      return std::nullopt; // Sensor read failed
    }
    return std::make_tuple(temp, hum);
  }

private:
  DHT dht;
};
