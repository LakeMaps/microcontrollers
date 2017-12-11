#include <Arduino.h>
#include <SoftwareSerial.h>
#ifdef FEATHER
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"
#else
#include <PololuQik.h>
#endif
#include <Crc16.h>

#define RESET_CMD (byte) 0x10
#define SET_CONFIG (byte) 0x11
#define GET_CONFIG (byte) 0x12
#define SET_SPEEDS (byte) 0x13
#define GET_CURRENTS (byte) 0x14
#define GET_ERRORS (byte) 0x15
#define ERROR_CMD (byte) 0x1F
#define SERIAL_BAUD 57600
#define MAX_MSG_LENGTH 10
#define RESET_RESP 5
#define GET_CONFIG_RESP 6
#define SET_CONFIG_RESP 6
#define SET_SPEED_RESP 8
#define GET_CURRENT_RESP 8
#define GET_ERRORS_RESP 5
#define ERROR_RESP 5

/*****GENERIC PARAMETERS COMMON TO ALL CONTROL MODULES***************/
uint8_t sRead[MAX_MSG_LENGTH];
int16_t serialTimeout = 5;  // the timeout (ms) of serial.readBytes commandByte
const uint8_t newMessage = 0xAA;
uint8_t commandByte = 0;
Crc16 crc;
uint16_t reqLength;
uint16_t respCRC;
uint16_t reqCRC;
void respond(uint8_t response[], uint16_t respLength);
bool reqCheck();
uint16_t byteToShort(int16_t index);
void errorReply(uint8_t errorByte);

/*****Propulsion Module Specific Parameters**************************/
#ifdef FEATHER
auto motorShield = Adafruit_MotorShield();
Adafruit_DCMotor *m0 = motorShield.getMotor(3);
Adafruit_DCMotor *m1 = motorShield.getMotor(4);
void initDriver() {
  motorShield.begin();
  m0->run(RELEASE);
  m1->run(RELEASE);
}
#else
PololuQik2s12v10 qik(3, 2, 4);  // TX, RX, RESET pins as written on motor driver
void initDriver() {
  qik.init();
}
#endif
int16_t M0Speed = 0;
int16_t M1Speed = 0;
int16_t M0Current = 0;
int16_t M1Current = 0;
int16_t errors = 0;
void ResetModule();
void GetConfig();
void SetConfig();
void SetSpeeds();
void GetCurrents();
void GetErrors();
void errorReply();
int16_t byteToInt(int16_t index);

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.setTimeout(serialTimeout);
  initDriver();
}

void loop() {
  if (Serial.available()) {
    Serial.readBytes(sRead, MAX_MSG_LENGTH);
    if (
         (sRead[0] == static_cast<uint8_t>(newMessage))
      && (sRead[1] >= 0x10)
      && (sRead[1] <= 0x1F)
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

void respond(uint8_t response[], uint16_t respLength) {
  for (uint16_t i = 0; i < respLength; i++) {
    Serial.write(response[i]);
  }
}

void ResetModule() {
  commandByte = 0;
  uint8_t response[RESET_RESP];
  initDriver();
  response[0] = newMessage;
  response[1] = RESET_CMD;
  response[2] = 0x01;  // always write true after reset
  respCRC = crc.XModemCrc(response, 0, (RESET_RESP-2));
  response[RESET_RESP-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[RESET_RESP-1] = (respCRC);  // get the LS 8 bits
  respond(response, RESET_RESP);
}

void GetConfig() {
#ifdef FEATHER
  errorReply(0x03);
#else
  commandByte = 0;
  uint8_t response[GET_CONFIG_RESP];
  response[0] = newMessage;
  response[1] = GET_CONFIG;
  response[2] = sRead[2];
  response[3] = qik.getConfigurationParameter(sRead[2]);
  respCRC = crc.XModemCrc(response, 0, (GET_CONFIG_RESP-2));
  response[GET_CONFIG_RESP-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[GET_CONFIG_RESP-1] = (respCRC);  // get the LS 8 bits
  respond(response, GET_CONFIG_RESP);
#endif
}

void SetConfig() {
#ifdef FEATHER
  errorReply(0x03);
#else
  commandByte = 0;
  uint8_t response[SET_CONFIG_RESP];
  qik.setConfigurationParameter(sRead[2], sRead[3]);
  delay(4);  // Manual specifies wait 4ms https://www.pololu.com/docs/0J29/5.d
  response[0] = newMessage;
  response[1] = SET_CONFIG;
  response[2] = sRead[2];
  response[3] = qik.getConfigurationParameter(sRead[2]);
  respCRC = crc.XModemCrc(response, 0, (SET_CONFIG_RESP-2));
  response[SET_CONFIG_RESP-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[SET_CONFIG_RESP-1] = (respCRC);  // get the LS 8 bits
  respond(response, SET_CONFIG_RESP);
#endif
}

void SetSpeeds() {
  commandByte = 0;
  M0Speed = byteToInt(2);
  M1Speed = byteToInt(4);
  if (
       (M0Speed > 127)
    || (M0Speed < -127)
    || (M1Speed > 127)
    || (M1Speed < -127)
  ) {
    errorReply(0x02);
    return;
  }
#ifdef FEATHER
  if (M0Speed < 0) {
    m0->run(BACKWARD);
  } else {
    m0->run(FORWARD);
  }
  if (M1Speed < 0) {
    m1->run(BACKWARD);
  } else {
    m1->run(FORWARD);
  }
  m0->setSpeed(abs(M0Speed));
  m1->setSpeed(abs(M1Speed));
#else
  qik.setSpeeds(M0Speed, M1Speed);
#endif
  uint8_t response[SET_SPEED_RESP];
  response[0] = newMessage;
  response[1] = SET_SPEEDS;
  response[2] = ((byte)(M0Speed >> 8));  // get the MS 8 bits
  response[3] = (M0Speed);  // get the LS 8 bits
  response[4] = ((byte)(M1Speed >> 8));  // get the MS 8 bits
  response[5] = (M1Speed);  // get the LS 8 bits
  respCRC = crc.XModemCrc(response, 0, (SET_SPEED_RESP-2));
  response[SET_SPEED_RESP-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[SET_SPEED_RESP-1] = (respCRC);  // get the LS 8 bits
  respond(response, SET_SPEED_RESP);
}

void GetCurrents() {
#ifdef FEATHER
  errorReply(0x03);
#else
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
  respCRC = crc.XModemCrc(response, 0, (GET_CURRENT_RESP-2));
  response[GET_CURRENT_RESP-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[GET_CURRENT_RESP-1] = (respCRC);  // get the LS 8 bits
  respond(response, GET_CURRENT_RESP);
#endif
}

void GetErrors() {
#ifdef FEATHER
  errorReply(0x03);
#else
  commandByte = 0;
  errors = qik.getErrors();
  uint8_t response[GET_ERRORS_RESP];
  response[0] = newMessage;
  response[1] = GET_ERRORS;
  response[2] = errors;
  respCRC = crc.XModemCrc(response, 0, (GET_ERRORS_RESP-2));
  response[GET_ERRORS_RESP-2] = ((byte)(respCRC >> 8));  // get the MS 8 bits
  response[GET_ERRORS_RESP-1] = (respCRC);  // get the LS 8 bits
  respond(response, GET_ERRORS_RESP);
#endif
}

int16_t byteToInt(int16_t index) {
  int16_t result = 0;
  int16_t first = (sRead[index]) * 256;  // bit shifting to the left 8 bits
  int16_t second = (sRead[index+1]) & 0x00FF;
  result = (first | second);  // combine two bytes (now ints) into a single int
  return result;
}

uint16_t byteToShort(int16_t index) {
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
    errorReply(0x01);
    return false;
  }
}

void errorReply(uint8_t errorByte) {
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
