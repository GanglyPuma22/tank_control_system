#pragma once
#include <Arduino.h>

class Device {
public:
  Device(const String &name = "") : name(name) {}
  virtual void begin() = 0;
  virtual void update() = 0; // called in main loop
  virtual void turnOn() = 0;
  virtual void turnOff() = 0;
  virtual bool isOn() const = 0;

  String getName() const { return name; }

private:
  String name;
};
