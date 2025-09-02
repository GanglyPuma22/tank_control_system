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

    if (shouldBeOn && !this->isOn())
      turnOn();
    if (!shouldBeOn && this->isOn())
      turnOff();
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

    if (desired["on"].is<JsonVariantConst>()) {
      bool shouldBeOn = desired["on"].as<bool>();
      if (shouldBeOn) {
        turnOn();
      } else {
        turnOff();
      }
    } else if (desired["timeOn"].is<JsonVariantConst>()) {
      Serial.print("Implement this! \n");
      // float onTime = desired["timeOn"].as<float>();
      // float offTime = desired["timeOff"].as<float>();
      // setOnOffTimes(onTime, offTime);
      // Serial.printf(
      //     "Light on/off times set to: on: %.1s off: %.1s\n",
      //     onTempF, offTempF);
    }
  }

  void reportState(JsonDocument &doc) override {
    doc["state"] = this->isOn();
    doc["onTime"] = onTime.getHour() * 100 + onTime.getMinute();
    doc["offTime"] = offTime.getHour() * 100 + offTime.getMinute();
  }

private:
  uint8_t pin;
  TimeOfDay onTime;
  TimeOfDay offTime;
};
