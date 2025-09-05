#pragma once
#include "../utils/TimeOfDay.h"
#include "Device.h"
#include <Arduino.h>

class Light : public Device {
public:
  Light(const std::string &name, uint8_t pin, TimeOfDay onTime,
        TimeOfDay offTime)
      : Device(name), pin(pin), onTime(onTime), offTime(offTime) {}

  void begin() override {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH); // OFF initially
    this->setState(true);
  }

  void update() override {
    TimeOfDay now = TimeOfDay::now();
    bool shouldBeOn = (onTime < offTime) ? (now >= onTime && now < offTime)
                                         : (now >= onTime || now < offTime);

    if (shouldBeOn) {
      turnOn();
    } else {
      turnOff();
    }
  }

  void turnOn() override {
    digitalWrite(pin, LOW);
    this->setState(true);
    Serial.println("Light ON");
  }

  void turnOff() override {
    digitalWrite(pin, HIGH);
    this->setState(false);
    Serial.println("Light OFF");
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

    if (desired["onTime"].is<JsonVariantConst>() &&
        desired["offTime"].is<JsonVariantConst>()) {
      String onTimeStr = desired["onTime"].as<String>();
      String offTimeStr = desired["offTime"].as<String>();

      setOnOffTimes(TimeOfDay::fromString(onTimeStr),
                    TimeOfDay::fromString(offTimeStr));
      Serial.printf("Light on/off times set to: on: %s off: %s\n", onTimeStr,
                    offTimeStr);
    }
  }

  void setOnOffTimes(TimeOfDay newOnTime, TimeOfDay newOffTime) {
    this->onTime = newOnTime;
    this->offTime = newOffTime;
  }

  void reportState(JsonDocument &doc) override {
    doc["state"] = this->isOn();
    doc["onTime"] = onTime.toString();
    doc["offTime"] = offTime.toString();
  }

private:
  uint8_t pin;
  TimeOfDay onTime;
  TimeOfDay offTime;
};
