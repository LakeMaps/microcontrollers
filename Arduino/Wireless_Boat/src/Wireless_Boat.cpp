#include <Arduino.h>
#include <SPI.h>
#include <RFM69.h>  // get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPIFlash.h>
#include <Crc16.h>

// *********** IMPORTANT SETTINGS - CHANGE/CONFIGURE TO FIT YOUR HARDWARE*******

#define NETWORKID 100  // the same on all nodes that talk to each other
#define NODEID 1  // address of this node
#define RECEIVER  2  // address of recipient of packets
#define FREQUENCY  RF69_915MHZ
#define ENCRYPTKEY "sampleEncryptKey"  // exactly the same on all nodes!
#define IS_RFM69HCW true  // set to 'true' if you are using an RFM69HCW module

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
#define ERROR_CMD ((byte) 0x0F)

/*****GENERIC PARAMETERS COMMON TO ALL CONTROL MODULES*************************/
const int maxMsgLength = 68;  // varies depending on module but still required
char sRead[maxMsgLength];
int serialTimeout = 5;  // the timeout in ms of a serial.readBytes commandByte
const byte newMessage = 0xAA;
byte commandByte = 0;
Crc16 crc;
uint16_t respLength;
uint16_t reqLength;
byte errorByte = 0;
short respCRC;
short reqCRC;
void respond(byte response[]);
bool reqCheck();
short byteToShort(int index);
void ErrorReply(byte errorByte);

/*****Wireless Module Specific Parameters**************************************/
int pwrLvl = 31;
bool ackReceived = false;
bool rxDone;
byte validData = 0;
byte RXPayload[61];
int TXRetries = 5;  // the number of times the radio will try resending
int TXRetryWaitTime = 100;  // the time in ms to keep trying to TX
RFM69 radio = RFM69(RFM69_CS, RFM69_IRQ, IS_RFM69HCW, RFM69_IRQN);
void ResetModule();
void GetConfig();
void SetConfig();
void Transmit();
void Receive();

void setup() {
  while (!Serial) {}  // wait until serial console is open
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
    memcpy(RXPayload, (void*)radio.DATA, 61);
  }
  if (radio.ACKRequested()) {
    radio.sendACK();
  }

  if (Serial.available()) {
    Serial.readBytes(sRead, maxMsgLength);
    if ((sRead[0] == (char) newMessage) && (sRead[1] >= 0x00) && (sRead[1] <= 0x0F)) {
      commandByte = sRead[1];  // the commandByte byte is byte 1 of the request
    }
    if (commandByte == RESET_CMD) {
      respLength = 5;
      reqLength = 5;
      if (reqCheck()) {
        ResetModule();
      } else {
        return;
      }
    }

    if (commandByte == GET_CONFIG) {
      respLength = 6;
      reqLength = 5;
      if (reqCheck()) {
        GetConfig();
      } else {
        return;
      }
    }

    if (commandByte == SET_CONFIG) {
      respLength = 6;
      reqLength = 6;
      if (reqCheck()) {
        SetConfig();
      } else {
        return;
      }
    }

    if (commandByte == TRANSMIT_CMD) {
      respLength = 5;
      reqLength = 65;
      if (reqCheck()) {
        Transmit();
      } else {
        return;
      }
    }

    if (commandByte == RECEIVE_CMD) {
      respLength = 68;
      reqLength = 5;
      if (reqCheck()) {
        Receive();
      } else {
        return;
      }
    }
  }
}


void respond(byte response[]) {
  for (int i = 0; i < respLength; i++) {
    Serial.write(response[i]);
  }
}

void ResetModule() {
  commandByte = 0;
  byte response[respLength];
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
  respCRC = crc.XModemCrc(response, 0, (respLength-2));
  response[respLength-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[respLength-1] = (respCRC);  // get the LS 8 bits
  respond(response);
}

void GetConfig() {
  commandByte = 0;
  byte response[respLength];
  response[0] = newMessage;
  response[1] = GET_CONFIG;
  response[2] = sRead[2];
  response[3] = radio.readReg(sRead[2]);
  respCRC = crc.XModemCrc(response, 0, (respLength-2));
  response[respLength-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[respLength-1] = (respCRC);  // get the LS 8 bits
  respond(response);
}

void SetConfig() {
  commandByte = 0;
  byte response[respLength];
  radio.writeReg(sRead[2], sRead[3]);
  response[0] = newMessage;
  response[1] = SET_CONFIG;
  response[2] = sRead[2];
  response[3] = radio.readReg(sRead[2]);
  respCRC = crc.XModemCrc(response, 0, (respLength-2));
  response[respLength-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[respLength-1] = (respCRC);  // get the LS 8 bits
  respond(response);
}

void Transmit() {
  commandByte = 0;
  byte response[respLength];
  byte TXPayload[61];
  for (int i = 0; i < 61; i++) {
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
  respCRC = crc.XModemCrc(response, 0, (respLength-2));
  response[respLength-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[respLength-1] = (respCRC);  // get the LS 8 bits
  respond(response);
}

void Receive() {
  commandByte = 0;
  byte response[respLength];
  // Prepare the response message
  response[0] = newMessage;
  response[1] = RECEIVE_CMD;
    for (int i = 0; i < 61; i++) {
      if (RXPayload[i] != 0x00) {
        validData = 1;
      }
    }
  response[2] = validData;
  for (int i = 0; i < 61; i++) {  // load data received into response message
    response[i+3] = RXPayload[i];
  }
  response[respLength-4] = (byte) (radio.RSSI >> 8);  // RSSI is an int16_t
  response[respLength-3] = radio.RSSI;
  respCRC = crc.XModemCrc(response, 0, (respLength-2));
  response[respLength-2] = (byte) (respCRC >> 8);  // get the MS 8 bits
  response[respLength-1] = (respCRC);  // get the LS 8 bits
  respond(response);
  validData = 0;
  for (int i = 0; i < 61; i++) {
    RXPayload[i] = 0;
  }
}

bool reqCheck() {
  byte request[reqLength];
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

void ErrorReply(byte errorByte) {
  respLength = 5;
  byte response[respLength];
  response[0] = newMessage;
  response[1] = ERROR_CMD;
  response[2] = errorByte;
  respCRC = crc.XModemCrc(response, 0, (respLength-2));
  response[respLength-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[respLength-1] = (respCRC);  // get the LS 8 bits
  errorByte = 0;
  respond(response);
}

short byteToShort(int index) {
  short result = 0;
  short first = (sRead[index]) * 256;  // bit shifting to the left 8 bits
  short second = (sRead[index+1]) & 0x00FF;
  result = (first | second);  // combine bytes (now shorts) into a single int
  return result;
}
