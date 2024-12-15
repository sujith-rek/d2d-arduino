#include "DHT.h"
#include <ArduinoJson.h>

// Define the pins for sensors
#define DHTPIN 2
#define DHTTYPE DHT22
#define CO2PIN A0
#define COPIN A1
#define PM10PIN A2
#define PM25PIN 8

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Sampling parameters for PM2.5 sensor
unsigned long lowPulseOccupancy = 0;
unsigned long startTime = 0;
unsigned long sampleTimeMs = 30000; // 30 seconds

void setup() {
  Serial.begin(9600);  // Communication with ESP8266
  dht.begin();         // Initialize DHT22
  pinMode(PM25PIN, INPUT);
  startTime = millis();
}

void loop() {
  // Read all sensors
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int co2 = map(analogRead(CO2PIN), 0, 1023, 0, 5000); // Adjust based on calibration
  int co = map(analogRead(COPIN), 0, 1023, 0, 1000);   // Adjust based on calibration
  
  // Read PM2.5
  unsigned long duration = pulseIn(PM25PIN, LOW);
  lowPulseOccupancy += duration;
  float pm25 = 0;
  if ((millis() - startTime) >= sampleTimeMs) {
    float ratio = lowPulseOccupancy / (sampleTimeMs * 10.0);
    pm25 = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62;
    lowPulseOccupancy = 0;
    startTime = millis();
  }

  // Read PM10
  float voMeasured = analogRead(PM10PIN) * (5.0 / 1024.0); // Convert to voltage
  float pm10 = 170 * voMeasured - 0.1;                    // Calibrated value
  
  // Check if any readings failed
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Create JSON object
  StaticJsonDocument<300> doc;
  doc["co2"] = co2;
  doc["co"] = co;
  doc["temp"] = temperature;
  doc["hum"] = humidity;
  doc["pm2.5"] = pm25;
  doc["pm10"] = pm10;

  // Serialize JSON
  String jsonStr;
  serializeJson(doc, jsonStr);

  // Send data to ESP8266
  Serial.println(jsonStr);
  delay(1000);  // Delay between readings
}
