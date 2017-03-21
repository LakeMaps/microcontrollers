// Sends NMEA sentences from the EM406 through to a computer via the Arduino
#include <Arduino.h>
#include <SoftwareSerial.h>

// Define which pins you will use on the Arduino to communicate with GPS
#define RXPIN 2
#define TXPIN 3
#define GPSBAUD 4800  // Set this value equal to the baud rate of your GPS
#define SERIALBAUD 9600  // Set this to a speed appropriate to keep up
SoftwareSerial gps(RXPIN, TXPIN);
char c;

void setup() {
  // This is the serial rate for your terminal program. It must be fast
  Serial.begin(SERIALBAUD);
  // Sets baud rate of your GPS
  gps.begin(GPSBAUD);
}

void loop() {
  while (gps.available()) {  // While there is data on the RX pin...
        c = gps.read();  // load the data into a variable...
        Serial.print(c);
  }
}
