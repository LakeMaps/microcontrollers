#include <SoftwareSerial.h> //specific to Propulsion Module
#include <PololuQik.h>  //specific to Propulsion Module
#include <Crc16.h>

#define RESET_CMD (byte) 0x10
#define SET_CONFIG (byte) 0x11
#define GET_CONFIG (byte) 0x12
#define SET_SPEEDS (byte) 0x13
#define GET_CURRENTS (byte) 0x14
#define GET_ERRORS (byte) 0x15
#define ERROR_CMD (byte) 0x1F
#define SERIAL_BAUD 115200

/*****GENERIC PARAMETERS COMMON TO ALL CONTROL MODULES*****************************/
const int maxMsgLength = 10; //varies depending on module but still required
char sRead[maxMsgLength];
int serialTimeout = 5;  // the timeout in ms of a serial.readBytes commandByte
const byte newMessage = 0xAA;
byte commandByte = 0;
Crc16 crc;
uint16_t respLength;
uint16_t reqLength;
short respCRC;
short reqCRC;
void respond(byte response[]);
bool reqCheck();

/*****Propulsion Module Specific Parameters****************************************/
PololuQik2s12v10 qik(11, 12, 13); //TX, RX, RESET pins
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
void ErrorReply();
int byteToInt(int index);
short byteToShort(int index);

void setup() {
  Serial.begin(SERIAL_BAUD);
  qik.init();
}

void loop() {
  if (Serial.available()) {
    Serial.readBytes(sRead, maxMsgLength);
    if ((sRead[0] == (char) newMessage) && (0x10 <= sRead[0] <= 0x1F)) {
      commandByte = sRead[1];  // the commandByte byte is byte 1 of the request (second byte sent)
    }
    if (commandByte == RESET_CMD) {
      respLength = 5;
      reqLength = 5;
      if (reqCheck()) {
        ResetModule();
      }
      else return;
    }
    
    if (commandByte == GET_CONFIG) {
      respLength = 6;
      reqLength = 5;
      if (reqCheck()) {    
        GetConfig();
      }
      else return;
    }
    if (commandByte == SET_CONFIG) {
      respLength = 6;
      reqLength = 6;
      if (reqCheck()) {
        SetConfig();
      }
      else return;
    }
    if (commandByte == SET_SPEEDS) {
      respLength = 8;
      reqLength = 8;
      if (reqCheck()) {
        SetSpeeds();
      }
      else return;
    }
    if (commandByte == GET_CURRENTS) {
      respLength = 8;
      reqLength = 5;
      if (reqCheck()) {
        GetCurrents();
      }
      else return;
    }
    if (commandByte == GET_ERRORS) {
      respLength = 5;
      reqLength = 5;
      if (reqCheck()) {
        GetErrors();
      }
      else return;
    }
  }
}

void respond(byte response[]) {
  for (int i = 0; i < respLength; i++) {
    Serial.write(response[i]);
  }
}

void ResetModule(){
  commandByte = 0;
  byte response[respLength];
  qik.init();
  response[0] = newMessage;
  response[1] = RESET_CMD;
  response[2] = 0x01; // always write true after reset until we can determine how best to confirm successful reset
  respCRC = crc.XModemCrc(response,0,(respLength-2));
  response[respLength-2] = ((byte)(respCRC >> 8));  // gets the most significant 8 bits (leftmost) of the integer
  response[respLength-1] = (respCRC);  // gets the least signficant 8 bits (rightmost) of the integer
  respond (response);
}

void GetConfig(){
  commandByte = 0;
  byte response[respLength];
  response[0] = newMessage;
  response[1] = GET_CONFIG;
  response[2] = sRead[2]; // always write true after reset until we can determine how best to confirm successful reset
  response[3] = qik.getConfigurationParameter(sRead[2]);
  respCRC = crc.XModemCrc(response,0,(respLength-2));
  response[respLength-2] = ((byte)(respCRC >> 8));  // gets the most significant 8 bits (leftmost) of the integer
  response[respLength-1] = (respCRC);  // gets the least signficant 8 bits (rightmost) of the integer
  respond (response);
}

void SetConfig(){
  commandByte = 0;
  byte response[respLength];
  qik.setConfigurationParameter(sRead[2], sRead[3]);
  delay(4); // User manual specifies waiting 4ms for the execution of this commandByte. https://www.pololu.com/docs/0J29/5.d
  response[0] = newMessage;
  response[1] = SET_CONFIG;
  response[2] = sRead[2]; // always write true after reset until we can determine how best to confirm successful reset
  response[3] = qik.getConfigurationParameter(sRead[2]);
  respCRC = crc.XModemCrc(response,0,(respLength-2));
  response[respLength-2] = ((byte)(respCRC >> 8));  // gets the most significant 8 bits (leftmost) of the integer
  response[respLength-1] = (respCRC);  // gets the least signficant 8 bits (rightmost) of the integer
  respond (response); 
}

void SetSpeeds(){
  commandByte = 0;
  M0Speed = byteToInt(2);
  M1Speed = byteToInt(4);
  qik.setSpeeds(M0Speed,M1Speed);
  byte response[respLength];
  response[0] = newMessage;
  response[1] = SET_SPEEDS;
  response[2] = ((byte)(M0Speed >> 8));  // gets the most significant 8 bits (leftmost) of the integer
  response[3] = (M0Speed);  // gets the least signficant 8 bits (rightmost) of the integer
  response[4] = ((byte)(M1Speed >> 8));  // gets the most significant 8 bits (leftmost) of the integer
  response[5] = (M1Speed);  // gets the least signficant 8 bits (rightmost) of the integer
  respCRC = crc.XModemCrc(response,0,(respLength-2));
  response[respLength-2] = ((byte)(respCRC >> 8));  // gets the most significant 8 bits (leftmost) of the integer
  response[respLength-1] = (respCRC);  // gets the least signficant 8 bits (rightmost) of the integer
  respond (response);
}

void GetCurrents(){
  commandByte = 0;
  M0Current = qik.getM0CurrentMilliamps();
  M1Current = qik.getM1CurrentMilliamps();
  byte response[respLength];
  response[0] = newMessage;
  response[1] = GET_CURRENTS;
  response[2] = ((byte)(M0Current >> 8));  // gets the most significant 8 bits (leftmost) of the integer
  response[3] = (M0Current);  // gets the least signficant 8 bits (rightmost) of the integer
  response[4] = ((byte)(M1Current >> 8));  // gets the most significant 8 bits (leftmost) of the integer
  response[5] = (M1Current);  // gets the least signficant 8 bits (rightmost) of the integer
  respCRC = crc.XModemCrc(response,0,(respLength-2));
  response[respLength-2] = ((byte)(respCRC >> 8));  // gets the most significant 8 bits (leftmost) of the integer
  response[respLength-1] = (respCRC);  // gets the least signficant 8 bits (rightmost) of the integer
  respond (response);
}

void GetErrors(){
  commandByte = 0;
  errors = qik.getErrors();
  byte response[respLength];
  response[0] = newMessage;
  response[1] = GET_ERRORS;
  response[2] = errors;
  respCRC = crc.XModemCrc(response,0,(respLength-2));
  response[respLength-2] = ((byte)(respCRC >> 8));  // gets the most significant 8 bits (leftmost) of the integer
  response[respLength-1] = (respCRC);  // gets the least signficant 8 bits (rightmost) of the integer
  respond (response);
}

int byteToInt(int index) {
  int result = 0;
  int first = (sRead[index]) * 256; //bit shifting to the left 8 bits
  int second = (sRead[index+1]) & 0x00FF;
  result = (first | second);  //combine the two bytes (now ints) into a single integer
  return result;
}

short byteToShort(int index) {
  short result = 0;
  short first = (sRead[index]) * 256; //bit shifting to the left 8 bits
  short second = (sRead[index+1]) & 0x00FF;
  result = (first | second);  //combine the two bytes (now shorts) into a single integer
  return result;
}

bool reqCheck() {
  byte request[reqLength];
  memcpy(request, sRead, reqLength);
  reqCRC = crc.XModemCrc(request, 0, (reqLength-2));
  if (reqCRC == byteToShort(reqLength-2)) {
    return true;
  }
  else {
    ErrorReply();
    return false;
  }
}

void ErrorReply() {
  respLength = 5;
  byte response[respLength];
  response[0] = newMessage;
  response[1] = ERROR_CMD;
  response[2] = 0x01;
  respCRC = crc.XModemCrc(response,0,(respLength-2));
  response[respLength-2] = ((byte)(respCRC >> 8));  // gets the most significant 8 bits (leftmost) of the integer
  response[respLength-1] = (respCRC);  // gets the least signficant 8 bits (rightmost) of the integer
  respond (response);
}
