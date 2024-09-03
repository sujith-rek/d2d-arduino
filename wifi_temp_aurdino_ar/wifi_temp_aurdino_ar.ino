#include "DHT.h"
#include <SoftwareSerial.h>

// Define the DHT sensor type and pin
#define DHTPIN 2        // DHT22 data pin connected to digital pin 2
#define DHTTYPE DHT22   // DHT 22 (AM2302)

// Initialize DHT sensor and software serial communication
DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial espSerial(5, 6); // RX, TX pins for communication with ESP8266

void setup() {
  // Begin serial communication with ESP8266
  espSerial.begin(115200);
  // Begin DHT sensor
  dht.begin();
}

void loop() {
  // Read humidity and temperature from DHT sensor
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  // // Check if any reads failed and exit early (to try again)
  // if (isnan(h) || isnan(t)) {
  //   espSerial.println("Failed to read from DHT sensor!");
  //   delay(2000); // Wait a while before trying again
  //   return;
  // }

  // Format the data as a single string to send to ESP8266
  String str = "H" + String(h) + "T" + String(t);

  // Send the formatted string to the ESP8266
  espSerial.print(str);
  espSerial.println();

  // Optional: Add a delay before the next reading
  delay(1000); // Send data every second
}
