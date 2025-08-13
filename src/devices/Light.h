#pragma once
#include "../utils/TimeOfDay.h"
#include "Device.h"
#include <Arduino.h>

class Light : public Device {
public:
  Light(uint8_t pin, TimeOfDay onTime, TimeOfDay offTime)
      : pin(pin), onTime(onTime), offTime(offTime), state(false) {}

  void begin() {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH); // OFF initially
    state = false;
  }

  void update() override {
    TimeOfDay now = TimeOfDay::now();
    bool shouldBeOn;

    if (onTime < offTime) {
      // Normal case (same day)
      shouldBeOn = (now >= onTime && now < offTime);
    } else {
      // Overnight case
      shouldBeOn = (now >= onTime || now < offTime);
    }

    if (shouldBeOn && !state)
      turnOn();
    if (!shouldBeOn && state)
      turnOff();
  }

  void turnOn() override {
    digitalWrite(pin, LOW);
    state = true;
    Serial.println("Light ON");
  }

  void turnOff() override {
    digitalWrite(pin, HIGH);
    state = false;
    Serial.println("Light OFF");
  }

  bool isOn() const override { return state; }

private:
  uint8_t pin;
  TimeOfDay onTime;
  TimeOfDay offTime;
  bool state;
};
