#pragma once
#include "Device.h"
#include <Arduino.h>

class HeatLamp : public Device {
public:
  HeatLamp(const std::string &name, uint8_t pin, float onTempF, float offTempF)
      : Device(name), pin(pin), heatLampOnTempF(onTempF),
        heatLampOffTempF(offTempF) {}

  void begin() override {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH); // OFF initially
    this->setState(false);
  }

  void update() override {
    // No-op, use update(temperatureF) for temp-based control
  }

  void update(float temperatureF) {
    if ((temperatureF < heatLampOnTempF) && !this->isOn())
      turnOn();
    else if (temperatureF >= heatLampOffTempF && this->isOn())
      turnOff();
  }

  void turnOn() override {
    digitalWrite(pin, LOW); // Turn on the relay
    this->setState(true);
    Serial.println("Heat lamp ON");
  }

  void turnOff() override {
    digitalWrite(pin, HIGH); // Turn off the relay
    this->setState(false);
    Serial.println("Heat lamp OFF");
  }

  void setHeatLampTemp(float onTempF, float offTempF) {
    heatLampOnTempF = onTempF;
    heatLampOffTempF = offTempF;
  }

  void applyState(JsonVariantConst desired) override {
    if (desired["on"].is<JsonVariantConst>()) {
      bool shouldBeOn = desired["on"].as<bool>();
      if (shouldBeOn) {
        turnOn();
      } else {
        turnOff();
      }
    }

    if (desired["heatLampOnTempF"].is<JsonVariantConst>()) {
      float onTempF = desired["heatLampOnTempF"].as<float>();
      float offTempF = desired["heatLampOffTempF"].as<float>();
      setHeatLampTemp(onTempF, offTempF);
      Serial.printf(
          "Heat lamp target temps set to: onTemp: %.1f°F offTemp: %.1f°F\n",
          onTempF, offTempF);
    }
  }

  void reportState(JsonDocument &doc) override {
    doc["state"] = this->isOn();
    doc["heatLampOnTempF"] = heatLampOnTempF;
    doc["heatLampOffTempF"] = heatLampOffTempF;
  }

private:
  uint8_t pin;
  float heatLampOnTempF;
  float heatLampOffTempF;
};
