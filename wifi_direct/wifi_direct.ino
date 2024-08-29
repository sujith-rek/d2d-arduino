#include <ESP8266WiFi.h>

const char* ssid = "meowmeow";
const char* password = "Mang0e@ter12";
const char* host = "192.168.248.158";  // Replace with your Raspberry Pi's IP address
int counter = 0;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  WiFiClient client;
  
  if (!client.connect(host, 8080)) {
    Serial.println("Connection failed");
    delay(1000);
    return;
  }

  // Send data to Raspberry Pi
  client.print(String("GET /data?data=") + counter + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");

  counter++;
  delay(1000);
}
