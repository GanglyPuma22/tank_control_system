// devices/Light.h
#pragma once
#include "Device.h"
#include <Arduino.h>

class Light : public Device {
public:
  // onHour and offHour in 24-hour format (0-23)
  Light(uint8_t pin, uint8_t onHour, uint8_t offHour)
      : pin(pin), onHour(onHour), offHour(offHour), state(false) {}

  void begin() {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    state = false;
  }

  void update() override {
    uint8_t currentHour =
        getCurrentHour(); // You need to implement this (RTC or NTP)
    if (onHour <= offHour) {
      // Normal case, e.g., on at 7, off at 19
      if (currentHour >= onHour && currentHour < offHour) {
        if (!state)
          turnOn();
      } else {
        if (state)
          turnOff();
      }
    } else {
      // Overnight case, e.g., on at 20, off at 6
      if (currentHour >= onHour || currentHour < offHour) {
        if (!state)
          turnOn();
      } else {
        if (state)
          turnOff();
      }
    }
  }

  void turnOn() override {
    digitalWrite(pin, HIGH);
    state = true;
    Serial.println("Light ON");
  }

  void turnOff() override {
    digitalWrite(pin, LOW);
    state = false;
    Serial.println("Light OFF");
  }

  bool isOn() const override { return state; }

private:
  uint8_t pin;
  uint8_t onHour;
  uint8_t offHour;
  bool state;

  uint8_t getCurrentHour() {
    // TODO: Implement this using RTC, NTP, or system time
    // For testing, just return a fixed hour like 12
    return 12;
  }
};
