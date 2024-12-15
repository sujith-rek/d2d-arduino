#include <SoftwareSerial.h>
#include <ArduinoJson.h>

// Initialize Arduino to NodeMCU (SoftwareSerial on Arduino)
SoftwareSerial nodemcu(5, 6);  // 5 = Rx, 6 = Tx

int IRSensor = 9; // Connect IR sensor module to Arduino pin D9
int irValue;

void setup() {
  Serial.begin(9600);  // For debugging
  nodemcu.begin(9600); // SoftwareSerial baud rate to match ESP8266

  pinMode(IRSensor, INPUT);  // Set IR sensor pin as input
  Serial.println("IR sensor program started");
}

void loop() {
  StaticJsonDocument<1000> doc;

  // Read IR sensor data
  readIRSensor();

  // Print IR sensor state to serial monitor
  Serial.print("IR Sensor State: ");
  Serial.println(irValue == HIGH ? "Object Detected" : "No Object");

  // Assign collected data to JSON Object
  doc["irSensor"] = (irValue == HIGH) ? "Object Detected" : "No Object";

  // Serialize JSON and send data to NodeMCU
  serializeJson(doc, nodemcu);
  nodemcu.println();  // Ensure we send a newline character to end the transmission
  delay(1000);
}

void readIRSensor() {
  irValue = digitalRead(IRSensor);  // Read the digital value from IR sensor
  Serial.print("IR Sensor Value: ");
  Serial.println(irValue);
}
