#pragma once
#include <optional>
#include <tuple>

class Sensor {
public:
  virtual ~Sensor() = default;

  // Returns optional tuple: temperature, humidity
  virtual std::optional<std::tuple<float, float>> readData() = 0;
};
