#include <SoftwareSerial.h>
#include <ArduinoJson.h>

// Initialize Arduino to NodeMCU (SoftwareSerial on Arduino)
SoftwareSerial nodemcu(5, 6);  // 5 = Rx, 6 = Tx

#define CO2PIN A0  // CO2 sensor connected to analog pin A0
int co2Value;

void setup() {
  Serial.begin(9600);  // For debugging
  nodemcu.begin(9600); // SoftwareSerial baud rate to match ESP8266

  Serial.println("CO2 sensor program started");
}

void loop() {
  StaticJsonDocument<1000> doc;

  // Read CO2 sensor data
  readCO2();

  // Print CO2 values to serial monitor
  Serial.print("CO2: ");
  Serial.print(co2Value);
  Serial.println(" ppm");

  // Assign collected data to JSON Object
  doc["co2"] = co2Value;

  // Serialize JSON and send data to NodeMCU
  serializeJson(doc, nodemcu);
  nodemcu.println();  // Ensure we send a newline character to end the transmission
  delay(1000);
}

void readCO2() {
  co2Value = analogRead(CO2PIN);  // Read the analog value from CO2 sensor
  co2Value = map(co2Value, 0, 1023, 0, 5000);  // Convert to CO2 in ppm (adjust as needed)
  Serial.print("CO2 Value: ");
  Serial.println(co2Value);
}
