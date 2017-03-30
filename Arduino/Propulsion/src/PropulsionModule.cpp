#include <Arduino.h>
#include <SoftwareSerial.h>  // specific to Propulsion Module
#include <PololuQik.h>  // specific to Propulsion Module
#include <Crc16.h>

#define RESET_CMD (byte) 0x10
#define SET_CONFIG (byte) 0x11
#define GET_CONFIG (byte) 0x12
#define SET_SPEEDS (byte) 0x13
#define GET_CURRENTS (byte) 0x14
#define GET_ERRORS (byte) 0x15
#define ERROR_CMD (byte) 0x1F
#define SERIAL_BAUD 115200
#define MAX_MSG_LENGTH 10
#define RESET_RESP 5
#define GET_CONFIG_RESP 6
#define SET_CONFIG_RESP 6
#define SET_SPEED_RESP 8
#define GET_CURRENT_RESP 8
#define GET_ERRORS_RESP 5
#define ERROR_RESP 5

/*****GENERIC PARAMETERS COMMON TO ALL CONTROL MODULES***************/
char sRead[MAX_MSG_LENGTH];
int serialTimeout = 5;  // the timeout in ms of a serial.readBytes commandByte
const uint8_t newMessage = 0xAA;
uint8_t commandByte = 0;
Crc16 crc;
uint16_t respLength;
uint16_t reqLength;
uint8_t errorByte = 0;
uint16_t respCRC;
uint16_t reqCRC;
void respond(uint8_t response[]);
bool reqCheck();
uint16_t byteToShort(int index);
void errorReply(uint8_t errorByte);

/*****Propulsion Module Specific Parameters**************************/
PololuQik2s12v10 qik(12, 13, 11);  // RX, TX, RESET pins on motor driver
int M0Speed = 0;
int M1Speed = 0;
int M0Current = 0;
int M1Current = 0;
int errors = 0;
void ResetModule();
void GetConfig();
void SetConfig();
void SetSpeeds();
void GetCurrents();
void GetErrors();
void errorReply();
int byteToInt(int index);

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.setTimeout(serialTimeout);
  qik.init();
}

void loop() {
  if (Serial.available()) {
    Serial.readBytes(sRead, MAX_MSG_LENGTH);
    if ((sRead[0] == (char) newMessage) && (sRead[1] >= 0x10) && (sRead[1] <= 0x1F)) {
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
    if (commandByte == SET_SPEEDS) {
      reqLength = 8;
      if (reqCheck()) {
        SetSpeeds();
      } else {
        return;
      }
    }
    if (commandByte == GET_CURRENTS) {
      reqLength = 5;
      if (reqCheck()) {
        GetCurrents();
      } else {
        return;
      }
    }
    if (commandByte == GET_ERRORS) {
      reqLength = 5;
      if (reqCheck()) {
        GetErrors();
      } else {
        return;
      }
    }
  }
}

void respond(uint8_t response[]) {
  for (int i = 0; i < respLength; i++) {
    Serial.write(response[i]);
  }
}

void ResetModule() {
  commandByte = 0;
  uint8_t response[RESET_RESP];
  qik.init();
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
  uint8_t response[GET_CONFIG_RESP];
  response[0] = newMessage;
  response[1] = GET_CONFIG;
  response[2] = sRead[2];
  response[3] = qik.getConfigurationParameter(sRead[2]);
  respCRC = crc.XModemCrc(response, 0, (respLength-2));
  response[respLength-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[respLength-1] = (respCRC);  // get the LS 8 bits
  respond(response);
}

void SetConfig() {
  commandByte = 0;
  uint8_t response[SET_CONFIG_RESP];
  qik.setConfigurationParameter(sRead[2], sRead[3]);
  delay(4);  // Manual specifies wait 4ms https://www.pololu.com/docs/0J29/5.d
  response[0] = newMessage;
  response[1] = SET_CONFIG;
  response[2] = sRead[2];
  response[3] = qik.getConfigurationParameter(sRead[2]);
  respCRC = crc.XModemCrc(response, 0, (respLength-2));
  response[respLength-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[respLength-1] = (respCRC);  // get the LS 8 bits
  respond(response);
}

void SetSpeeds() {
  commandByte = 0;
  M0Speed = byteToInt(2);
  M1Speed = byteToInt(4);
  if ((M0Speed > 127) || (M0Speed < -127) || (M1Speed > 127) || (M1Speed < -127)) {
    errorByte = 0x02;
    errorReply(errorByte);
    return;
  }
  qik.setSpeeds(M0Speed, M1Speed);
  uint8_t response[SET_SPEED_RESP];
  response[0] = newMessage;
  response[1] = SET_SPEEDS;
  response[2] = ((byte)(M0Speed >> 8));  // get the MS 8 bits
  response[3] = (M0Speed);  // get the LS 8 bits
  response[4] = ((byte)(M1Speed >> 8));  // get the MS 8 bits
  response[5] = (M1Speed);  // get the LS 8 bits
  respCRC = crc.XModemCrc(response, 0, (respLength-2));
  response[respLength-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[respLength-1] = (respCRC);  // get the LS 8 bits
  respond(response);
}

void GetCurrents() {
  commandByte = 0;
  M0Current = qik.getM0CurrentMilliamps();
  M1Current = qik.getM1CurrentMilliamps();
  uint8_t response[GET_CURRENT_RESP];
  response[0] = newMessage;
  response[1] = GET_CURRENTS;
  response[2] = ((byte)(M0Current >> 8));  // get the MS 8 bits
  response[3] = (M0Current);  // get the LS 8 bits
  response[4] = ((byte)(M1Current >> 8));  // get the MS 8 bits
  response[5] = (M1Current);  // get the LS 8 bits
  respCRC = crc.XModemCrc(response, 0, (respLength-2));
  response[respLength-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[respLength-1] = (respCRC);  // get the LS 8 bits
  respond(response);
}

void GetErrors() {
  commandByte = 0;
  errors = qik.getErrors();
  uint8_t response[GET_ERRORS_RESP];
  response[0] = newMessage;
  response[1] = GET_ERRORS;
  response[2] = errors;
  respCRC = crc.XModemCrc(response, 0, (respLength-2));
  response[respLength-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[respLength-1] = (respCRC);  // get the LS 8 bits
  respond(response);
}

int byteToInt(int index) {
  int result = 0;
  int first = (sRead[index]) * 256;  // bit shifting to the left 8 bits
  int second = (sRead[index+1]) & 0x00FF;
  result = (first | second);  // combine two bytes (now ints) into a single int
  return result;
}

uint16_t byteToShort(int index) {
  uint16_t result = 0;
  uint16_t first = (sRead[index]) * 256;  // bit shifting to the left 8 bits
  uint16_t second = (sRead[index+1]) & 0x00FF;
  result = (first | second);  // combine two bytes (now ints) into a single int
  return result;
}

bool reqCheck() {
  uint8_t request[MAX_MSG_LENGTH];
  memcpy(request, sRead, reqLength);
  reqCRC = crc.XModemCrc(request, 0, (reqLength-2));
  if (reqCRC == byteToShort(reqLength-2)) {
    return true;
  } else {
    errorByte = 0x01;
    errorReply(errorByte);
    return false;
  }
}

void errorReply(uint8_t errorByte) {
  respLength = 5;
  uint8_t response[ERROR_RESP];
  response[0] = newMessage;
  response[1] = ERROR_CMD;
  response[2] = errorByte;
  respCRC = crc.XModemCrc(response, 0, (respLength-2));
  response[respLength-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[respLength-1] = (respCRC);  // get the LS 8 bits
  errorByte = 0;
  respond(response);
}
