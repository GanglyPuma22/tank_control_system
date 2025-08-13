// devices/HeatLamp.h
#pragma once
#include "Device.h"
#include <Arduino.h>

class HeatLamp : public Device {
public:
  HeatLamp(uint8_t pin, float onTempF, float offTempF)
      : pin(pin), heatLampOnTempF(onTempF), heatLampOffTempF(offTempF),
        state(false) {}

  void begin() {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH); // Start with heat lamp OFF
    state = false;
  }

  void update(float temperatureF) {
    if ((temperatureF < heatLampOnTempF || temperatureF < heatLampOffTempF) &&
        !state) {
      turnOn();
    } else if (temperatureF >= heatLampOffTempF && state) {
      turnOff();
    }
  }

  // Override base Device methods for manual control
  void update() override {
    // No-op for now: update with temperature must be called separately
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

private:
  uint8_t pin;
  float heatLampOnTempF;
  float heatLampOffTempF;
  bool state;
};
