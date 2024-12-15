#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include "DHT.h"

// Pin Definitions
#define DHTPIN 2          // DHT22 connected to digital pin 2
#define DHTTYPE DHT22     // DHT22 sensor type
#define CO2PIN A0         // MQ135 CO2 sensor connected to analog pin A0

// Initialize sensors
DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial nodemcu(5, 6); // RX, TX for ESP8266 communication

// Variables
int co2Value;
float temperature;
float humidity;

void setup() {
  // Serial for debugging
  Serial.begin(9600);
  nodemcu.begin(9600); // Match baud rate with ESP8266

  // Initialize sensors
  dht.begin();

  Serial.println("Combined sensor program started");
}

void loop() {
  StaticJsonDocument<200> doc;

  // Read sensor data
  readCO2();
  readDHT();

  // Check for valid data
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(2000); // Retry after a delay
    return;
  }

  // Assign data to JSON object
  doc["co2"] = co2Value;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;

  // Serialize JSON and send data to ESP8266
  serializeJson(doc, nodemcu);
  nodemcu.println();  // Ensure newline to end transmission

  // Debugging: Print JSON to Serial Monitor
  Serial.print("Sent JSON: ");
  serializeJson(doc, Serial);
  Serial.println();

  delay(1000); // Wait before the next reading
}

void readCO2() {
  co2Value = analogRead(CO2PIN);  // Read analog value
  co2Value = map(co2Value, 0, 1023, 0, 5000);  // Convert to CO2 in ppm (adjust scale as needed)
  Serial.print("CO2 Value: ");
  Serial.println(co2Value);
}

void readDHT() {
  humidity = dht.readHumidity();     // Read humidity
  temperature = dht.readTemperature(); // Read temperature
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C, Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
}
