#pragma once
#include "Sensor.h"
#include <Adafruit_MLX90614.h>

class MLX90614 : public Sensor {
public:
  MLX90614() {}

  void begin() {
    if (!mlx.begin()) {
      Serial.println("Error connecting to MLX sensor. Check wiring.");
    };

    Serial.print("MLX Emissivity = ");
    Serial.println(mlx.readEmissivity());
  }

  // Returns object temp then ambient temp
  std::optional<std::tuple<float, float>> readData() override {
    float temp = mlx.readObjectTempF();
    float amb = mlx.readAmbientTempF();
    if (isnan(temp) || isnan(amb)) {
      return std::nullopt; // Sensor read failed
    }
    return std::make_tuple(temp, amb);
  }

private:
  Adafruit_MLX90614 mlx = Adafruit_MLX90614();
};
