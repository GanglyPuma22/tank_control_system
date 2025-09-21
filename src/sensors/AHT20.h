#pragma once
#include "Sensor.h"
#include <Adafruit_AHTX0.h>

class AHT20 : public Sensor {
public:
  AHT20() {}

  void begin() {
    if (!aht.begin()) {
      Serial.println("Could not find AHT20? Check wiring");
    } else {

      Serial.println("AHT20 found");
    }
  }

  std::optional<std::tuple<float, float>> readData() override {
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity,
                 &temp); // populate temp and humidity objects with fresh data
    if (isnan(temp.temperature) || isnan(humidity.relative_humidity)) {
      return std::nullopt; // Sensor read failed
    }
    // Default return is in Celsius, convert to Fahrenheit
    float temperature_f = temp.temperature * 9.0 / 5.0 + 32.0;
    return std::make_tuple(temperature_f, humidity.relative_humidity);
  }

private:
  Adafruit_AHTX0 aht;
};