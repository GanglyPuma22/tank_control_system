#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT11
#define RELAY_PIN 5

DHT dht(DHTPIN, DHTTYPE);

const float heatLampOffTempF = 98.0;
const float heatLampOnTempF = 92.0;

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // start off
  Serial.begin(115200);
  delay(1000);
  Serial.println("DHT11 sensor initializing...");

  dht.begin();
}

void loop() {
  // Wait a few seconds between measurements (DHT11 max rate ~1Hz)
  digitalWrite(RELAY_PIN, HIGH); // turn relay ON
  delay(2000);
  digitalWrite(RELAY_PIN, LOW);  // turn relay OFF
  delay(2000);

  float humidity = dht.readHumidity();
  float temperatureF = dht.readTemperature(true);

  if (isnan(humidity) || isnan(temperatureF)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperatureF);
  Serial.println(" °F");
}
