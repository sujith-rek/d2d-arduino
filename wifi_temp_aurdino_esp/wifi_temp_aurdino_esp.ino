#include <ESP8266WiFi.h>

// Wi-Fi credentials
const char* ssid = "meowmeow";
const char* password = "Mang0e@ter12";

// Server details
const char* server = "192.168.248.158";  // Replace with your server IP
const int port = 8080;                   // Replace with your server port

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  
  // Connect to Wi-Fi
  connectToWiFi();
}

void loop() {
  // Check if there is incoming data from Arduino
  if (Serial.available()) {
    String receivedData = Serial.readStringUntil('\n');  // Read data until newline character

    if (receivedData.length() > 0) {  // Check if there is any data received
      sendDataToServer(receivedData);  // Send the received data to the server
    }
  }

  delay(100);  // Small delay to prevent overwhelming the server
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nConnected to WiFi!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  sendDataToServer("Connected");  // Optionally send a connection message
}

void sendDataToServer(String jsonData) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;

    // Connect to the server
   if (client.connect(server, port)) {
      Serial.println("Connected to server.");

      // Encode the data for application/x-www-form-urlencoded
      String postData = "data=" + jsonData;

      // Build the HTTP POST request
      String httpRequest = "POST /data HTTP/1.1\r\n";
      httpRequest += "Host: " + String(server) + "\r\n";
      httpRequest += "Content-Type: application/x-www-form-urlencoded\r\n";
      httpRequest += "Content-Length: " + String(postData.length()) + "\r\n";
      httpRequest += "Connection: close\r\n\r\n";
      httpRequest += postData;  // Add the encoded data to the body

      // Send the entire HTTP request as a single string
      client.print(httpRequest);
      Serial.println("Data sent to server: " + postData);

      // Wait for server response
      while (client.connected()) {
        while (client.available()) {
          String line = client.readStringUntil('\n');
          Serial.println(line);  // Print server response to Serial Monitor
        }
      }

      // Disconnect
      client.stop();
      Serial.println("Disconnected from server.");
    } else {
      Serial.println("Connection to server failed.");
    }
  } else {
    Serial.println("WiFi not connected.");
  }
}
