#include <SPI.h>
#include <RFM69_OTA.h>//get it here: https://www.github.com/lowpowerlab/rfm69
 
//*********************************************************************************************
// *********** IMPORTANT SETTINGS - YOU MUST CHANGE/CONFIGURE TO FIT YOUR HARDWARE ************
//*********************************************************************************************
#define NETWORKID 100  //the same on all nodes that talk to each other
#define NODEID 1 
#define RECEIVER  2// The recipient of packets
#define FREQUENCY  RF69_915MHZ
#define ENCRYPTKEY "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HCW true // set to 'true' if you are using an RFM69HCW module
  
#define SERIAL_BAUD   115200
#define RFM69_CS  10
#define RFM69_IRQ 2
#define RFM69_IRQN 0  // Pin 2 is IRQ 0!
#define RFM69_RST 9

#define RESET_CMD ((byte) 0x00)
#define SET_CONFIG ((byte) 0x01)
#define GET_CONFIG ((byte) 0x02)
#define RECEIVE_CMD ((byte) 0x03)
#define TRANSMIT_CMD ((byte) 0x04)
 
bool ackReceived = 0;
byte command;
int TXRetries = 5;  // the number of times the radio will try resending
int TXRetryWaitTime = 100;  //the time in ms to keep trying to TX
int serialTimeout = 10;  // the timeout in ms of a serial.readBytes command
byte newMessage = 0xAA;
const int maxMsgLength = 72;  // the maximum length of a request between the arduino and computer (header, payload, footer)
char request[maxMsgLength];
RFM69 radio = RFM69(RFM69_CS, RFM69_IRQ, IS_RFM69HCW, RFM69_IRQN);
void CRC16(byte command, byte payload);
void ResetModule();
void ResetReply();
void GetConfigReply();
void SetConfigReply();
void TransmitParse();
void TransmitReply(int ackReceived);
void ReceiveReply();

void setup() {
  while (!Serial); // wait until serial console is open
  Serial.begin(SERIAL_BAUD);
  Serial.setTimeout(serialTimeout);  // the timeout for reading multiple bytes sequentially, in milliseconds
  ResetModule();
}
 
void loop() {
  if (Serial.available()) {
    Serial.readBytes(request, maxMsgLength);  // read in a max of maxMsgLength bytes. Times out after serialTimeout
    command = request[1];  // the command byte is byte 1 of the request (second byte sent)
  }
  if (command == RESET_CMD) {
    ResetModule();
    ResetReply();
}

if (command == GET_CONFIG) {
  GetConfigReply();
}

if (command == SET_CONFIG) {
  SetConfigReply();
}

if (command == TRANSMIT_CMD) {
  TransmitParse();
  TransmitReply();
}

if (command == RECEIVE_CMD) {
  ReceiveReply();
}
  }

void ResetModule() {
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, HIGH);
  delay(100);
  digitalWrite(RFM69_RST, LOW);
  delay(100);
  // Initialize radio
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  radio.setHighPower();// Only for RFM69HCW & HW!
  radio.setPowerLevel(31); // power output ranges from 0 (5dBm) to 31 (20dBm)
  radio.encrypt(ENCRYPTKEY);
}

void ResetReply() {
  Serial.write(newMessage);
  Serial.write(RESET_CMD);
  Serial.write(0x00);
  CRC16(RESET_CMD, ((byte)0));
}

void GetConfigReply () {
  Serial.write(newMessage);
  Serial.write(GET_CONFIG);
  Serial.write(request[2]);
  Serial.write(radio.readReg(request[2]));
  CRC16(GET_CONFIG, radio.readReg(request[2]));
}

void SetConfigReply () {
  radio.writeReg(request[2], request[3]);
  Serial.write(newMessage);
  Serial.write(SET_CONFIG);
  Serial.write(request[2]);
  Serial.write(radio.readReg(request[2]));
  CRC16(SET_CONFIG, radio.readReg(request[2]));
}

void TransmitParse() {
  byte TXPayload [61];
  for (int i = 2; i <= 62; i++) {
    TXPayload[i] = request[i];  //load the payload into the radio packet
  }
  if (radio.sendWithRetry(RECEIVER, TXPayload, 61)) { //target node Id, request as byte array, request length
  ackReceived = true;
  }
  else ackReceived = false;
  radio.receiveDone(); //put radio in RX mode
  Serial.flush(); //make sure all serial data is clocked out before sleeping the MCU
}

void TransmitReply () {
  Serial.write(newMessage);
  Serial.write(TRANSMIT_CMD);
  Serial.write(ackReceived);
  CRC16(TRANSMIT_CMD, ackReceived);
}

void ReceiveReply() {
  byte RXPayload [61];
  byte RXReplyPayload [63];
  //check if something was received (could be an interrupt from the radio)
  if (radio.receiveDone()) {
    memcpy(RXPayload, (void*)radio.DATA, 61);
  }
  if (radio.ACKRequested()) {
    radio.sendACK();
  }
  memcpy(RXReplyPayload, RXPayload, 61);
  RXReplyPayload[61] = radio.RSSI;
  RXReplyPayload[62] = radio.RSSI << 8; 
  Serial.write(newMessage);
  Serial.write(RECEIVE_CMD);
  for (int i = 0; i <= 60; i++) {
    Serial.write((char*)radio.DATA);
  }
  Serial.write(radio.RSSI);
  CRC16(RECEIVE_CMD, RXReplyPayload);  
  radio.receiveDone(); //put radio in RX mode
  Serial.flush(); //make sure all serial data is clocked out before sleeping the MCU
}

void CRC16(byte command, byte* payload) {
  Serial.write(((byte)0x00));
  return;
}
