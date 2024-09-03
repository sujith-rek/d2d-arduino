#include "DHT.h"
#include <SoftwareSerial.h>
#include <ArduinoJson.h> // Include the ArduinoJson library

// Define the DHT sensor type and pin
#define DHTPIN 2        // DHT22 data pin connected to digital pin 2
#define DHTTYPE DHT22   // DHT 22 (AM2302)

// Initialize DHT sensor and software serial communication
DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial espSerial(5, 6); // RX, TX pins for communication with ESP8266

void setup() {
  // Begin serial communication with ESP8266
  espSerial.begin(115200);
  // Initialize DHT sensor
  dht.begin();
}

void loop() {
  // Read humidity and temperature from DHT sensor
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  // Check if any reads failed and exit early (to try again)
  if (isnan(h) || isnan(t)) {
    espSerial.println("Failed to read from DHT sensor!");
    delay(2000); // Wait a while before trying again
    return;
  }

  // Create a JSON object
  StaticJsonDocument<200> doc;
  doc["humidity"] = h;
  doc["temperature"] = t;

  // Serialize JSON to string
  String jsonStr;
  serializeJson(doc, jsonStr);

  // Send the JSON string to the ESP8266
  espSerial.println(jsonStr);

  // Optional: Add a delay before the next reading
  delay(1000); // Send data every second
}
