/*
Required connections between Arduino and qik 2s9v1
      Arduino   qik 2s9v1
-------------------------
           5V - VCC
          GND - GND
Digital Pin 2 - TX1
Digital Pin 3 - RX1
Digital Pin 4 - RESET1
Digital Pin 5 - TX1
Digital Pin 6 - RX1
Digital Pin 7 - RESET1
*/

#include <SoftwareSerial.h>
#include <PololuQik.h>

PololuQik2s12v10 qik(11, 12, 13);

void setup() {
  qik.init();
}

void loop() {
   //FORWARD
  qik.setM0Speed(127);
  qik.setM1Speed(127);
  delay(10000);
   //BACKWARD
  qik.setM0Speed(-127);
  qik.setM1Speed(-127);
  delay(10000);
   //RIGHT FULL
  qik.setM0Speed(127);
  qik.setM1Speed(-127);
  delay(10000);
  //LEFT FULL
  qik.setM0Speed(-127);
  qik.setM1Speed(127);
  delay(10000);
  //KINDA RIGHT
  qik.setM0Speed(127);
  qik.setM1Speed(0);
  delay(10000);
  //KINDA LEFT
  qik.setM0Speed(0);
  qik.setM1Speed(127);
  delay(10000);
}
