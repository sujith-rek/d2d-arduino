#include <ESP8266WiFi.h>
#include <DHT.h>

// 3.3 V to +
// GND to -
// D4 to out

// WiFi credentials
const char* ssid = "meowmeow";
const char* password = "Mang0e@ter12";
const char* host = "192.168.248.158";  // Replace with your Raspberry Pi's IP address

// DHT sensor configuration
#define DHTPIN 2        // GPIO2 on NodeMCU corresponds to D4
#define DHTTYPE DHT22   // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

// Setup function
void setup() {
  Serial.begin(115200);
  dht.begin();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

// Loop function
void loop() {
  // Read data from DHT22 sensor
  float hum = dht.readHumidity();
  float temp = dht.readTemperature();

  if (isnan(hum) || isnan(temp)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Print temp and humidity values to serial monitor
  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.print(" %, Temp: ");
  Serial.print(temp);
  Serial.println(" Celsius");

  // Send data to the server
  WiFiClient client;
  if (client.connect(host, 8080)) {
    String url = "/data?temp=" + String(temp) + "&hum=" + String(hum);
    Serial.print("Requesting URL: ");
    Serial.println(url);

    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(10);

    // Read all the lines of the reply from the server and print them to Serial
    while(client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
    
    client.stop();
    Serial.println("\nClient disconnected.");
  } else {
    Serial.println("Connection to server failed");
  }

  // Wait before sending the next data point
  delay(10000);
}
