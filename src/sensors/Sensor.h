#pragma once
#include <optional>
#include <tuple>

class Sensor {
public:
  virtual ~Sensor() = default;

  // Returns optional tuple
  // examples temperature, humidity or object_temp,ambient_temp
  virtual std::optional<std::tuple<float, float>> readData() = 0;
};
