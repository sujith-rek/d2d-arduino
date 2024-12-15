#include <DHT.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

// Define sensor pins
#define DHTPIN 2           // DHT22 data pin connected to digital pin 2
#define DHTTYPE DHT22      // DHT 22 sensor
#define CO2PIN A0          // MQ135 CO2 sensor connected to analog pin A0
#define COPIN A1           // MQ2 CO sensor connected to analog pin A1
#define PM10PIN A2         // GP2Y10 PM10 sensor connected to analog pin A2
#define PM25PIN 8          // DSM501A PM2.5 sensor connected to digital pin 8

// Initialize DHT sensor and software serial
DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial espSerial(5, 6); // RX, TX pins for ESP8266 communication

// PM2.5 sensor parameters
unsigned long lowPulseOccupancy = 0;
unsigned long pm25StartTime = 0;
const unsigned long pm25SampleTimeMs = 30000; // 30 seconds for PM2.5 sampling

// Timing variables for data averaging
unsigned long startTime = 0;
const unsigned long averagingTimeMs = 60000; // 1 minute averaging

// Accumulators for sensor data averaging
float tempSum = 0, humSum = 0, pm10Sum = 0, pm25Sum = 0;
int co2Sum = 0, coSum = 0;
int tempCount = 0, humCount = 0, pm10Count = 0, pm25Count = 0, co2Count = 0, coCount = 0;

void setup() {
  Serial.begin(9600);       // For debugging
  espSerial.begin(115200);  // ESP8266 communication
  dht.begin();              // Initialize DHT sensor
  pinMode(PM25PIN, INPUT);
  startTime = millis();     // Initialize averaging timer
  pm25StartTime = millis(); // Initialize PM2.5 sampling timer
}

void loop() {
  // Read temperature and humidity every 2 seconds
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Check if DHT sensor read failed
  if (!isnan(humidity) && !isnan(temperature)) {
    humSum += humidity;
    tempSum += temperature;
    humCount++;
    tempCount++;
  } else {
    Serial.println("Failed to read from DHT sensor!");
  }

  // Read CO2 and CO levels (adjust calibration as needed)
  int co2 = map(analogRead(CO2PIN), 0, 1023, 0, 5000);
  int co = map(analogRead(COPIN), 0, 1023, 0, 1000);
  if (co2 > 0) {
    co2Sum += co2;
    co2Count++;
  }
  if (co > 0) {
    coSum += co;
    coCount++;
  }

  // Calculate PM2.5 concentration every 30 seconds
  unsigned long duration = pulseIn(PM25PIN, LOW);
  lowPulseOccupancy += duration;
  if ((millis() - pm25StartTime) >= pm25SampleTimeMs) {
    float ratio = lowPulseOccupancy / (pm25SampleTimeMs * 10.0);
    float pm25 = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62;
    if (pm25 > 0) {
      pm25Sum += pm25;
      pm25Count++;
    }
    lowPulseOccupancy = 0;
    pm25StartTime = millis();
  }

  // Read PM10 level
  float voMeasured = analogRead(PM10PIN) * (5.0 / 1024.0); // Convert to voltage
  float pm10 = 170 * voMeasured - 0.1;                    // Calibrated value
  if (pm10 > 0) {
    pm10Sum += pm10;
    pm10Count++;
  }

  // Every 1 minute, calculate averages and send data to ESP8266
  if ((millis() - startTime) >= averagingTimeMs) {
    float avgTemp = tempCount > 0 ? tempSum / tempCount : 0;
    float avgHum = humCount > 0 ? humSum / humCount : 0;
    float avgPM10 = pm10Count > 0 ? pm10Sum / pm10Count : 0;
    float avgPM25 = pm25Count > 0 ? pm25Sum / pm25Count : 0;
    int avgCO2 = co2Count > 0 ? co2Sum / co2Count : 0;
    int avgCO = coCount > 0 ? coSum / coCount : 0;

    // Create a JSON object for sensor data
    StaticJsonDocument<300> doc;
    doc["temp"] = avgTemp;
    doc["hum"] = avgHum;
    doc["co2"] = avgCO2;
    doc["co"] = avgCO;
    doc["pm2.5"] = avgPM25;
    doc["pm10"] = avgPM10;

    // Serialize JSON to string
    String jsonStr;
    serializeJson(doc, jsonStr);

    // Print JSON to Serial Monitor for debugging
    Serial.println("Averaged Sensor Data (JSON):");
    Serial.println(jsonStr);

    // Send JSON to ESP8266
    espSerial.println(jsonStr);

    // Reset accumulators and counters
    tempSum = humSum = pm10Sum = pm25Sum = 0;
    co2Sum = coSum = 0;
    tempCount = humCount = pm10Count = pm25Count = co2Count = coCount = 0;
    startTime = millis();
  }

  delay(2000); // 2-second delay between sensor readings
}
