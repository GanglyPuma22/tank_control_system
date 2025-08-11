// devices/Device.h
#pragma once

class Device {
public:
  virtual ~Device() = default;

  // Call this periodically to update the device state
  virtual void update() = 0;

  // Optionally, a way to manually turn on/off the device
  virtual void turnOn() = 0;
  virtual void turnOff() = 0;

  // Optional: get the device's current state
  virtual bool isOn() const = 0;
};
