#pragma once
#include "Device.h"
#include <Arduino.h>

class HeatLamp : public Device {
public:
  HeatLamp(const std::string &name, uint8_t pin, float onTempF, float offTempF)
      : Device(name), pin(pin), heatLampOnTempF(onTempF),
        heatLampOffTempF(offTempF), state(false) {}

  void begin() override {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH); // OFF initially
    state = false;
  }

  void update() override {
    // No-op, use update(temperatureF) for temp-based control
  }

  void update(float temperatureF) {
    if ((temperatureF < heatLampOnTempF) && !state)
      turnOn();
    else if (temperatureF >= heatLampOffTempF && state)
      turnOff();
  }

  void turnOn() override {
    digitalWrite(pin, LOW); // Turn on the relay
    state = true;
    Serial.println("Heat lamp ON");
  }

  void turnOff() override {
    digitalWrite(pin, HIGH); // Turn off the relay
    state = false;
    Serial.println("Heat lamp OFF");
  }

  bool isOn() const override { return state; }

  void setHeatLampTemp(float onTempF, float offTempF) {
    heatLampOnTempF = onTempF;
    heatLampOffTempF = offTempF;
  }

  void applyState(JsonVariantConst desired) override {
    Serial.print("HeatLamp applyState called\n");
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
    doc["state"] = isOn();
    doc["heatLampOnTempF"] = heatLampOnTempF;
    doc["heatLampOffTempF"] = heatLampOffTempF;
  }

private:
  uint8_t pin;
  float heatLampOnTempF;
  float heatLampOffTempF;
  bool state;
};
