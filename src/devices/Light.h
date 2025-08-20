#pragma once
#include "../utils/TimeOfDay.h"
#include "Device.h"
#include <Arduino.h>

class Light : public Device {
public:
  Light(const String &name, uint8_t pin, TimeOfDay onTime, TimeOfDay offTime)
      : Device(name), pin(pin), onTime(onTime), offTime(offTime), state(false) {
  }

  void begin() override {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH); // OFF initially
    state = false;
  }

  void update() override {
    TimeOfDay now = TimeOfDay::now();
    bool shouldBeOn = (onTime < offTime) ? (now >= onTime && now < offTime)
                                         : (now >= onTime || now < offTime);

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
