#include <Arduino.h>
#include <SPI.h>
#include <RFM69.h>  // get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPIFlash.h>

#define NETWORKID 100  // the same on all nodes that talk to each other
#define NODEID 2  // address of this node
#define RECEIVER 1  // address of recipient of packets
#define FREQUENCY RF69_915MHZ
#define ENCRYPTKEY "sampleEncryptKey"  // exactly the same on all nodes!
#define IS_RFM69HCW true  // set to 'true' if you are using an RFM69HCW module
#define RFM69_CS 10
#define RFM69_IRQ 2
#define RFM69_IRQN 0  // Pin 2 is IRQ 0!
#define RFM69_RST 9

int pwrLvl = 31;
bool ackReceived = false;
bool rxDone;
int TXRetries = 5;  // the number of times the radio will try resending
int TXRetryWaitTime = 100;  // the time in ms to keep trying to TX
RFM69 radio = RFM69(RFM69_CS, RFM69_IRQ, IS_RFM69HCW, RFM69_IRQN);
uint8_t TXPayload[61] = {0};

void setup() {
  Serial.begin(115200);
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, HIGH);
  delay(100);
  digitalWrite(RFM69_RST, LOW);
  delay(100);
  // Initialize radio
  radio.initialize(FREQUENCY, NODEID, NETWORKID);
  radio.setHighPower();  // Only for RFM69HCW & HW!
  radio.setPowerLevel(pwrLvl);  // power ranges from 0 (5dBm) to 31 (20dBm)
  radio.encrypt(ENCRYPTKEY);
}

void loop() {
  Serial.println("Starting");
  delay(1000);
  Serial.print("Start Time: ");
  Serial.println(micros());
  if (radio.sendWithRetry(RECEIVER, TXPayload, 61)) {  // target, msg, length
    ackReceived = true;
    Serial.print("Sent. Time: ");
    Serial.println(micros());
  } else {
    ackReceived = false;
    Serial.print("Send Failed. Time Elapsed: ");
    Serial.println(micros());
  }
  radio.receiveDone();  // put radio in RX mode
  delay(2000);
}
