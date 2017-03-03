/*
Required connections between Arduino and qik 2s12v10:

      Arduino   qik 2s12v10
---------------------------
          GND - GND
Digital Pin 11 - TX
Digital Pin 12 - RX
Digital Pin 13 - RESET
*/

#include <SoftwareSerial.h>
#include <PololuQik.h>

PololuQik2s12v10 qik(11, 12, 13);

void setup() {
  Serial.begin(115200);
  Serial.println("qik 2s12v10 dual serial motor controller");
  
  qik.init();
  
  Serial.print("Firmware version: ");
  Serial.write(qik.getFirmwareVersion());
  Serial.println();
}

void loop() {
  for (int i = 0; i <= 127; i++) {
    qik.setM0Speed(i);
    if (abs(i) % 64 == 32) {
      Serial.print("M0 current: ");
      Serial.println(qik.getM0CurrentMilliamps());
    }
    delay(5);
  }

  for (int i = 127; i >= -127; i--) {
    qik.setM0Speed(i);
    if (abs(i) % 64 == 32) {
      Serial.print("M0 current: ");
      Serial.println(qik.getM0CurrentMilliamps());
    }
    delay(5);
  }

  for (int i = -127; i <= 0; i++) {
    qik.setM0Speed(i);
    if (abs(i) % 64 == 32) {
      Serial.print("M0 current: ");
      Serial.println(qik.getM0CurrentMilliamps());
    }
    delay(5);
  }

  /*
  for (int i = 0; i <= 127; i++) {
    qik.setM1Speed(i);
    if (abs(i) % 64 == 32) {
      Serial.print("M1 current: ");
      Serial.println(qik.getM1CurrentMilliamps());
    }
    delay(5);
  }

  for (int i = 127; i >= -127; i--) {
    qik.setM1Speed(i);
    if (abs(i) % 64 == 32) {
      Serial.print("M1 current: ");
      Serial.println(qik.getM1CurrentMilliamps());
    }
    delay(5);
  }

  for (int i = -127; i <= 0; i++) {
    qik.setM1Speed(i);
    if (abs(i) % 64 == 32) {
      Serial.print("M1 current: ");
      Serial.println(qik.getM1CurrentMilliamps());
    }
    delay(5);
  }
  */
}