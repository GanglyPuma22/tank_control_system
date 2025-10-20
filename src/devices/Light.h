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
    turnOff(); // OFF initially
  }

  void update() override {
    if (this->getOverrideMode()) {
      // In override mode, do not change state automatically
      return;
    }
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
    digitalWrite(pin, HIGH);
    this->setState(true);
  }

  void turnOff() override {
    digitalWrite(pin, LOW);
    this->setState(false);
  }

  void applyState(JsonVariantConst desired) override {
    if (desired["state"].is<JsonVariantConst>()) {
      this->setOverrideMode(true); // Manual override
      bool shouldBeOn = desired["state"].as<bool>();
      if (shouldBeOn) {
        turnOn();
      } else {
        turnOff();
      }
    } else {
      this->setOverrideMode(false); // Resume automatic control
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

  void logState(Values::MapValue &lightState) override {
    lightState.add("state", Values::BooleanValue(this->isOn()))
        .add("onTime", Values::StringValue(this->onTime.toString()))
        .add("offTime", Values::StringValue(this->offTime.toString()));
  }

private:
  uint8_t pin;
  TimeOfDay onTime;
  TimeOfDay offTime;
};
