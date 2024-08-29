#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

const char* ssid = "meowmeow";
const char* password = "Mang0e@ter12";

// Server details
const char* server = "192.168.248.158";  // Replace with your server IP
const int port = 8080;  // Replace with your server port

void setup() {
  Serial.begin(9600);  // Adjusted to match Arduino's SoftwareSerial baud rate
  
  connectToWiFi();
}

void loop() {
  if (Serial.available() > 0) {
    String jsonData = Serial.readStringUntil('\n');  // Read data from Arduino

    // Parse and send the data if it's a valid JSON object
    if (isValidJson(jsonData)) {
      sendDataToServer(jsonData);
    } else {
      Serial.println("Invalid JSON received from Arduino.");
    }
  }
  
  delay(1000);  // Wait before checking again
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

bool isValidJson(String data) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, data);
  return !error;  // Return true if JSON is valid
}

void sendDataToServer(String jsonData) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;

    // Connect to the server
    if (client.connect(server, port)) {
      // Prepare the HTTP GET request
      String httpRequest = "GET /data?json=" + jsonData + " HTTP/1.1\r\n" +
                           "Host: " + server + "\r\n" +
                           "Connection: close\r\n\r\n";

      // Send the request
      client.print(httpRequest);
      Serial.println("Data sent to server: " + jsonData);
      
      // Wait for server response
      while (client.connected() || client.available()) {
        if (client.available()) {
          String line = client.readStringUntil('\n');
          Serial.println(line);  // Print server response to Serial Monitor
        }
      }

      // Disconnect
      client.stop();
    } else {
      Serial.println("Connection to server failed.");
    }
  } else {
    Serial.println("WiFi not connected.");
  }
}
