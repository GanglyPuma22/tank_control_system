#pragma once
#include "Device.h"
#include <Arduino.h>

// TODO IM REALIZING HEAT LAMP SHOULD PROBABLY JUST HAVE ONE CUTOFF TEMP.

// AND THINK HOW IT WILL INTEGRATE WITH dynamic env control to match another
// place.
class HeatLamp : public Device {
public:
  HeatLamp(const std::string &name, uint8_t pin, float onTempF, float offTempF)
      : Device(name), pin(pin), onAboveTempF(onTempF), offAboveTempF(offTempF) {
  }

  void begin() override {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH); // OFF initially
    this->setState(false);
  }

  void update() override {
    // No-op, use update(temperatureF) for temp-based control
  }

  void update(float temperatureF) {
    if ((temperatureF < onAboveTempF))
      turnOn();
    else if (temperatureF >= offAboveTempF)
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

  void setHeatLampTemps(float onTempF, float offTempF) {
    this->onAboveTempF = onTempF;
    this->offAboveTempF = offTempF;
  }

  void applyState(JsonVariantConst desired) override {
    if (desired["state"].is<JsonVariantConst>()) {
      bool shouldBeOn = desired["state"].as<bool>();
      if (shouldBeOn) {
        turnOn();
      } else {
        turnOff();
      }
    }

    if (desired["onAbove"].is<JsonVariantConst>() &&
        desired["offAbove"].is<JsonVariantConst>()) {
      float onTempF = desired["onAbove"].as<float>();
      float offTempF = desired["offAbove"].as<float>();
      setHeatLampTemps(onTempF, offTempF);
      Serial.printf(
          "Heat lamp target temps set to: onTemp: %.1f°F offTemp: %.1f°F\n",
          onTempF, offTempF);
    }
  }

  void reportState(JsonDocument &doc) override {
    doc["state"] = this->isOn();
    doc["onAbove"] = onAboveTempF;
    doc["offAbove"] = offAboveTempF;
  }

private:
  uint8_t pin;
  float onAboveTempF;
  float offAboveTempF;
};
