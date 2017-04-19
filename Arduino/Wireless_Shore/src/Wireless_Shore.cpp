#include <Arduino.h>
#include <SPI.h>
#include <RFM69.h>  // get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPIFlash.h>
#include <Crc16.h>

// *********** IMPORTANT SETTINGS - CHANGE/CONFIGURE TO FIT YOUR HARDWARE*******

#define NETWORKID 100  // the same on all nodes that talk to each other
#define NODEID 2  // address of this node
#define RECEIVER  1  // address of recipient of packets
#define FREQUENCY  RF69_915MHZ
#define ENCRYPTKEY "sampleEncryptKey"  // exactly the same on all nodes!
#define IS_RFM69HCW true  // set to 'true' if you are using an RFM69HCW module

#define SERIAL_BAUD   57600
#define RFM69_CS  10
#define RFM69_IRQ 2
#define RFM69_IRQN 0  // Pin 2 is IRQ 0!
#define RFM69_RST 9

#define RESET_CMD ((byte) 0x00)
#define SET_CONFIG ((byte) 0x01)
#define GET_CONFIG ((byte) 0x02)
#define RECEIVE_CMD ((byte) 0x03)
#define TRANSMIT_CMD ((byte) 0x04)
#define ERROR_CMD ((byte) 0x0F)
#define MAX_MSG_LENGTH 68
#define RESET_RESP 5
#define GET_CONFIG_RESP 6
#define SET_CONFIG_RESP 6
#define TX_RESP 5
#define RX_RESP 68
#define ERROR_RESP 5

/*****GENERIC PARAMETERS COMMON TO ALL CONTROL MODULES*************************/
uint8_t sRead[MAX_MSG_LENGTH];
int16_t serialTimeout = 5;  // the timeout (ms) of serial.readBytes commandByte
const uint8_t newMessage = 0xAA;
uint8_t commandByte = 0;
Crc16 crc;
uint16_t respLength;
uint16_t reqLength;
uint8_t errorByte = 0;
uint16_t respCRC;
uint16_t reqCRC;
void respond(byte response[], uint16_t respLength);
bool reqCheck();
uint16_t byteToShort(int16_t index);
void ErrorReply(byte errorByte);

/*****Wireless Module Specific Parameters**************************************/
int16_t pwrLvl = 31;
bool ackReceived = false;
bool rxDone;
uint8_t validData = 0;
uint8_t RXPayload[61];
int16_t TXRetries = 5;  // the number of times the radio will try resending
int16_t TXRetryWaitTime = 100;  // the time in ms to keep trying to TX
RFM69 radio = RFM69(RFM69_CS, RFM69_IRQ, IS_RFM69HCW, RFM69_IRQN);
void ResetModule();
void GetConfig();
void SetConfig();
void Transmit();
void Receive();

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.setTimeout(serialTimeout);  // reading bytes sequentially, in ms
  // Reset Radio Module
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
  // check if something was received (could be an interrupt from the radio)
  rxDone = radio.receiveDone();
  if (rxDone) {
    memcpy(RXPayload, reinterpret_cast<volatile void*>(radio.DATA), 61);
  }
  if (radio.ACKRequested()) {
    radio.sendACK();
  }
  if (Serial.available()) {
    Serial.readBytes(sRead, MAX_MSG_LENGTH);
    if (
         (sRead[0] == static_cast<uint8_t>(newMessage))
      && (sRead[1] >= 0x00)
      && (sRead[1] <= 0x0F)
    ) {
      commandByte = sRead[1];  // the commandByte byte is byte 1 of the request
    }
    if (commandByte == RESET_CMD) {
      reqLength = 5;
      if (reqCheck()) {
        ResetModule();
      } else {
        return;
      }
    }

    if (commandByte == GET_CONFIG) {
      reqLength = 5;
      if (reqCheck()) {
        GetConfig();
      } else {
        return;
      }
    }

    if (commandByte == SET_CONFIG) {
      reqLength = 6;
      if (reqCheck()) {
        SetConfig();
      } else {
        return;
      }
    }

    if (commandByte == TRANSMIT_CMD) {
      reqLength = 65;
      if (reqCheck()) {
        Transmit();
      } else {
        return;
      }
    }

    if (commandByte == RECEIVE_CMD) {
      reqLength = 5;
      if (reqCheck()) {
        Receive();
      } else {
        return;
      }
    }
  }
}


void respond(uint8_t response[], uint16_t respLength) {
  for (uint16_t i = 0; i < respLength; i++) {
    Serial.write(response[i]);
  }
}

void ResetModule() {
  commandByte = 0;
  uint8_t response[RESET_RESP];
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
  response[0] = newMessage;
  response[1] = RESET_CMD;
  response[2] = 0x01;  // always write true after reset
  respCRC = crc.XModemCrc(response, 0, (RESET_RESP-2));
  response[RESET_RESP-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[RESET_RESP-1] = (respCRC);  // get the LS 8 bits
  respond(response, RESET_RESP);
}

void GetConfig() {
  commandByte = 0;
  uint8_t response[GET_CONFIG_RESP];
  response[0] = newMessage;
  response[1] = GET_CONFIG;
  response[2] = sRead[2];
  response[3] = radio.readReg(sRead[2]);
  respCRC = crc.XModemCrc(response, 0, (GET_CONFIG_RESP-2));
  response[GET_CONFIG_RESP-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[GET_CONFIG_RESP-1] = (respCRC);  // get the LS 8 bits
  respond(response, GET_CONFIG_RESP);
}

void SetConfig() {
  commandByte = 0;
  uint8_t response[SET_CONFIG_RESP];
  radio.writeReg(sRead[2], sRead[3]);
  response[0] = newMessage;
  response[1] = SET_CONFIG;
  response[2] = sRead[2];
  response[3] = radio.readReg(sRead[2]);
  respCRC = crc.XModemCrc(response, 0, (SET_CONFIG_RESP-2));
  response[SET_CONFIG_RESP-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[SET_CONFIG_RESP-1] = (respCRC);  // get the LS 8 bits
  respond(response, SET_CONFIG_RESP);
}

void Transmit() {
  commandByte = 0;
  uint8_t response[TX_RESP];
  uint8_t TXPayload[61];
  for (uint16_t i = 0; i < 61; i++) {
    TXPayload[i] = sRead[i+2];  // load the payload into the radio packet
  }
  if (radio.sendWithRetry(RECEIVER, TXPayload, 61)) {  // target, msg, length
    ackReceived = true;
  } else {
    ackReceived = false;
  }
  radio.receiveDone();  // put radio in RX mode
  response[0] = newMessage;
  response[1] = TRANSMIT_CMD;
  response[2] = ackReceived;
  respCRC = crc.XModemCrc(response, 0, (TX_RESP-2));
  response[TX_RESP-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[TX_RESP-1] = (respCRC);  // get the LS 8 bits
  respond(response, TX_RESP);
}

void Receive() {
  commandByte = 0;
  uint8_t response[RX_RESP];
  // Prepare the response message
  response[0] = newMessage;
  response[1] = RECEIVE_CMD;
    for (uint16_t i = 0; i < 61; i++) {
      if (RXPayload[i] != 0x00) {
        validData = 1;
      }
    }
  response[2] = validData;
  for (uint16_t i = 0; i < 61; i++) {  // load data received into response msg
    response[i+3] = RXPayload[i];
  }
  response[RX_RESP-4] = (byte) (radio.RSSI >> 8);  // RSSI is an int16_t
  response[RX_RESP-3] = radio.RSSI;
  respCRC = crc.XModemCrc(response, 0, (RX_RESP-2));
  response[RX_RESP-2] = (byte) (respCRC >> 8);  // get the MS 8 bits
  response[RX_RESP-1] = (respCRC);  // get the LS 8 bits
  respond(response, RX_RESP);
  validData = 0;
  for (uint16_t i = 0; i < 61; i++) {
    RXPayload[i] = 0;
  }
}

bool reqCheck() {
  uint8_t request[MAX_MSG_LENGTH];
  memcpy(request, sRead, reqLength);
  reqCRC = crc.XModemCrc(request, 0, (reqLength-2));
  if (reqCRC == byteToShort(reqLength-2)) {
    return true;
  } else {
    errorByte = 0x01;
    ErrorReply(errorByte);
    return false;
  }
}

void ErrorReply(uint8_t errorByte) {
  uint8_t response[ERROR_RESP];
  response[0] = newMessage;
  response[1] = ERROR_CMD;
  response[2] = errorByte;
  respCRC = crc.XModemCrc(response, 0, (ERROR_RESP-2));
  response[ERROR_RESP-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[ERROR_RESP-1] = (respCRC);  // get the LS 8 bits
  errorByte = 0;
  respond(response, ERROR_RESP);
}

uint16_t byteToShort(int16_t index) {
  uint16_t result = 0;
  uint16_t first = (sRead[index]) * 256;  // bit shifting to the left 8 bits
  uint16_t second = (sRead[index+1]) & 0x00FF;
  result = (first | second);  // combine bytes (now shorts) into a single int
  return result;
}
