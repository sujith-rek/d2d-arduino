#include <SoftwareSerial.h>

SoftwareSerial mySerial(12, 13); // RX, TX pins on Arduino
int co2 = 0;
double multiplier = 1; // 1 for 2% = 20000 PPM, 10 for 20% = 200,000 PPM
uint8_t buffer[50]; // Increased buffer size to 50 for safety
uint8_t ind = 0;
uint8_t index = 0;

// Function prototypes
int fill_buffer();
int format_output();

void setup() {
  Serial.begin(9600); // Start serial communication with host computer
  Serial.println("\n\nArduino to COZIR CO2 Sensor - Demonstration code 2/14/18\n\n");

  mySerial.begin(9600); // Start serial communications with COZIR sensor
  delay(1000); // Allow sensor some time to initialize

  // Send commands to the sensor
  Serial.println("Sending configuration to the COZIR sensor...");
  mySerial.println("M 6"); // Set mode for Z and z outputs (filtered and unfiltered CO2)
  mySerial.println("K 1"); // Set streaming mode
}

void loop() {
  // Debugging message to check if loop is running
  Serial.println("Reading sensor data...");

  if (fill_buffer()) { // If buffer is filled correctly
    // Debugging message to check buffer contents
    Serial.print("Buffer contains: ");
    for (int j = 0; j < ind; j++) Serial.print(buffer[j], HEX);
    
    index = 0;
    format_output();
    Serial.print(" Raw PPM ");

    index = 8; // In ASCII buffer, filtered value is offset from raw by 8 bytes
    format_output();
    Serial.println(" Filtered PPM\n\n");
  } else {
    Serial.println("Failed to read from sensor or timeout occurred.");
  }

  delay(1000); // Add delay to avoid flooding serial output
}

int fill_buffer() {
  // Fill buffer with sensor ASCII data
  ind = 0;
  unsigned long startMillis = millis();
  unsigned long timeout = 2000; // Timeout after 2 seconds

  while (millis() - startMillis < timeout) { // Timeout loop
    if (mySerial.available()) {
      buffer[ind] = mySerial.read();
      if (buffer[ind - 1] == 0x0A) { // End of line (CR character)
        ind = ind - 2; // Adjust ind to match last numerical character
        return 1; // Successfully read data
      }
      ind++;
    }
  }
  
  // If we reach here, we hit the timeout
  return 0; // Failed to fill buffer within timeout
}

int format_output() {
  // Read buffer, extract 6 ASCII chars, convert to PPM, and print
  co2 = buffer[15 - index] - 0x30;
  co2 += (buffer[14 - index] - 0x30) * 10;
  co2 += (buffer[13 - index] - 0x30) * 100;
  co2 += (buffer[12 - index] - 0x30) * 1000;
  co2 += (buffer[11 - index] - 0x30) * 10000;

  Serial.print("\nCO2 = ");
  Serial.print(co2 * multiplier, 0);
  Serial.println(" PPM");
  return 0;
}
