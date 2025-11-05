#pragma once
#include "Sensor.h"
#include <Adafruit_MLX90614.h>

class MLX90614 : public Sensor {
public:
  MLX90614(double emissivity = 1.0) : emissivity(emissivity) {}

  void begin() {
    if (!mlx.begin()) {
      // Serial.println("Error connecting to MLX sensor. Check wiring.");
    } else {
      this->initializeSuccessful();
      mlx.writeEmissivity(this->emissivity);
      // Serial.print("MLX Initialized and its Emissivity = ");
      // Serial.println(mlx.readEmissivity());
    }
  }

  // Returns object temp then ambient temp
  std::optional<std::tuple<float, float>> readData() override {
    if (!isInitialized()) {
      // Serial.println("MLX Sensor not initialized");
      return std::nullopt;
    }
    float temp = mlx.readObjectTempF();
    float amb = mlx.readAmbientTempF();
    if (isnan(temp) || isnan(amb)) {
      // Serial.println("Failed to read MLX sensor");
      return std::nullopt; // Sensor read failed
    }
    return std::make_tuple(temp, amb);
  }

private:
  Adafruit_MLX90614 mlx = Adafruit_MLX90614();
  double emissivity;
};
