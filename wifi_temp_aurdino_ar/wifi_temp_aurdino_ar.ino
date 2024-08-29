//Arduino side code
#include <DHT.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

//Initialize Arduino to NodeMCU (SoftwareSerial on Arduino)
SoftwareSerial nodemcu(5, 6);  // 5 = Rx, 6 = Tx

#define DHTPIN 4  // DHT sensor pin
DHT dht(DHTPIN, DHT22);
float temp;
float hum;

void setup() {
  Serial.begin(9600);  // For debugging
  nodemcu.begin(9600); // SoftwareSerial baud rate to match ESP8266

  dht.begin();
  delay(1000);
  Serial.println("Program started");
}

void loop() {
  StaticJsonDocument<1000> doc;

  // Read Temp and Hum data
  dht11_func();

  // Assign collected data to JSON Object
  doc["humidity"] = hum;
  doc["temperature"] = temp;

  // Serialize JSON and send data to NodeMCU
  serializeJson(doc, nodemcu);
  nodemcu.println();  // Ensure we send a newline character to end the transmission
  delay(1000);
}

void dht11_func() {
  hum = dht.readHumidity();
  temp = dht.readTemperature();
  Serial.print("Humidity: ");
  Serial.println(hum);
  Serial.print("Temperature: ");
  Serial.println(temp);
}
