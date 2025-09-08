#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>

class Device {
public:
  Device(const std::string &name = "unregistered") : name(name) {
    // Safe auto-registration
    registry()[name] = this;
    state = false;
  } // todo: Figure out way to not have name in html depend on this name

  static Device *getDevice(const std::string &name) {
    Serial.printf("Looking up device by name: %s\n", name.c_str());
    auto it = registry().find(name);
    return (it != registry().end()) ? it->second : nullptr;
  }

  virtual void begin() = 0;
  virtual void update() = 0; // called in main loop
  virtual void turnOn() = 0;
  virtual void turnOff() = 0;
  // Functions to handle state syncing between webpage and esp32
  virtual void applyState(JsonVariantConst desired) = 0;
  virtual void reportState(JsonDocument &doc) = 0;

  // Return const reference to the entire registry
  static const std::map<std::string, Device *> &getAllDevices() {
    return registry();
  }
  virtual bool isOn() { return state; }
  virtual void setState(bool newState) { state = newState; }
  virtual void setOverrideMode(bool mode) { overrideMode = mode; }
  virtual bool getOverrideMode() { return overrideMode; }

private:
  bool state;
  bool overrideMode = false;
  std::string name;
  // Singleton accessor for registry
  static std::map<std::string, Device *> &registry() {
    static std::map<std::string, Device *> instance;
    return instance;
  }
};