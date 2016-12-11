#include <SoftwareSerial.h>
#include "RTClib.h"
#include <avr/pgmspace.h>

#define DEBUG_UBIDOTS

RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

#define initialCRC8() 0x00
#define calculateCRC8(crc,n) table[ crc8 ^ n ]

// RMS server
#define WEB_SERVER "scarlet.tapgo.cc"

const unsigned char crc8table[256] PROGMEM = {
  0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83, 0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41,
  0x9D, 0xC3, 0x21, 0x7F, 0xFC, 0xA2, 0x40, 0x1E, 0x5F, 0x01, 0xE3, 0xBD, 0x3E, 0x60, 0x82, 0xDC,
  0x23, 0x7D, 0x9F, 0xC1, 0x42, 0x1C, 0xFE, 0xA0, 0xE1, 0xBF, 0x5D, 0x03, 0x80, 0xDE, 0x3C, 0x62,
  0xBE, 0xE0, 0x02, 0x5C, 0xDF, 0x81, 0x63, 0x3D, 0x7C, 0x22, 0xC0, 0x9E, 0x1D, 0x43, 0xA1, 0xFF,
  0x46, 0x18, 0xFA, 0xA4, 0x27, 0x79, 0x9B, 0xC5, 0x84, 0xDA, 0x38, 0x66, 0xE5, 0xBB, 0x59, 0x07,
  0xDB, 0x85, 0x67, 0x39, 0xBA, 0xE4, 0x06, 0x58, 0x19, 0x47, 0xA5, 0xFB, 0x78, 0x26, 0xC4, 0x9A,
  0x65, 0x3B, 0xD9, 0x87, 0x04, 0x5A, 0xB8, 0xE6, 0xA7, 0xF9, 0x1B, 0x45, 0xC6, 0x98, 0x7A, 0x24,
  0xF8, 0xA6, 0x44, 0x1A, 0x99, 0xC7, 0x25, 0x7B, 0x3A, 0x64, 0x86, 0xD8, 0x5B, 0x05, 0xE7, 0xB9,
  0x8C, 0xD2, 0x30, 0x6E, 0xED, 0xB3, 0x51, 0x0F, 0x4E, 0x10, 0xF2, 0xAC, 0x2F, 0x71, 0x93, 0xCD,
  0x11, 0x4F, 0xAD, 0xF3, 0x70, 0x2E, 0xCC, 0x92, 0xD3, 0x8D, 0x6F, 0x31, 0xB2, 0xEC, 0x0E, 0x50,
  0xAF, 0xF1, 0x13, 0x4D, 0xCE, 0x90, 0x72, 0x2C, 0x6D, 0x33, 0xD1, 0x8F, 0x0C, 0x52, 0xB0, 0xEE,
  0x32, 0x6C, 0x8E, 0xD0, 0x53, 0x0D, 0xEF, 0xB1, 0xF0, 0xAE, 0x4C, 0x12, 0x91, 0xCF, 0x2D, 0x73,
  0xCA, 0x94, 0x76, 0x28, 0xAB, 0xF5, 0x17, 0x49, 0x08, 0x56, 0xB4, 0xEA, 0x69, 0x37, 0xD5, 0x8B,
  0x57, 0x09, 0xEB, 0xB5, 0x36, 0x68, 0x8A, 0xD4, 0x95, 0xCB, 0x29, 0x77, 0xF4, 0xAA, 0x48, 0x16,
  0xE9, 0xB7, 0x55, 0x0B, 0x88, 0xD6, 0x34, 0x6A, 0x2B, 0x75, 0x97, 0xC9, 0x4A, 0x14, 0xF6, 0xA8,
  0x74, 0x2A, 0xC8, 0x96, 0x15, 0x4B, 0xA9, 0xF7, 0xB6, 0xE8, 0x0A, 0x54, 0xD7, 0x89, 0x6B, 0x35,
};


unsigned char packetCRC( unsigned char *packet, int length ) {
  unsigned char crc8 = initialCRC8();
  unsigned char table[256];
  for (int k = 0; k < 256; k++) {
    table[k] = pgm_read_byte_near(crc8table + k);
    Serial.print(table[k]);
    Serial.print(" ");
  }
  while ( length-- ) {
    crc8 = calculateCRC8( crc8, *packet );
    packet++;
  }
  return crc8;
}

SoftwareSerial SIM900(7, 8);
SoftwareSerial ST109(4, 5);

//char c = 0;

void setup() {
  // Check both serial ports
  Serial.begin(57600);

  // Initialeze ST-109. Turn off verbose mode
  ST109.begin(38400);
  ST109.listen();

  Serial.println(F("<109> verbose off"));
  ST109.write(0x01);
  ST109.write(0x51);
  ST109.write(0xFF);
  ST109.write(0x2D);
  delay(200);
  Serial.print(F("<109> receive: "));
  while (ST109.available() > 0)
  {
    char c = ST109.read();
    Serial.print(c, HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Initialize SIM900 module
  SIM900.begin(19200);
  sim900_send_cmd("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", "OK");
  sim900_send_cmd("AT+SAPBR=3,1,\"APN\",\"internet\"", "OK");
  sim900_send_cmd("AT+SAPBR=1,1", "ERROR");
  sim900_send_cmd("AT+SAPBR=2,1", "+SAPBR: 1,1");

  // Check hwclock
  if (!rtc.begin()) {
    Serial.println(F("Couldn't find RTC"));
    while (1);
  }
}

char* readData(uint16_t timeout) {
  uint16_t replyidx = 0;
  char replybuffer[500];
  while (timeout--) {
    if (replyidx >= 500) {
      break;
    }
    while (SIM900.available()) {
      char c =  SIM900.read();
      if (c == '\r') continue;
      if (c == 0xA) {
        if (replyidx == 0)   // the first 0x0A is ignored
          continue;
      }
      replybuffer[replyidx] = c;
      replyidx++;
    }

    if (timeout == 0) {
      break;
    }
    delay(1);
  }
  replybuffer[replyidx] = '\0';  // null term
#ifdef DEBUG_UBIDOTS
  Serial.println("Response of GPRS:");
  Serial.println(replybuffer);
#endif
  while (SIM900.available()) {
    SIM900.read();
  }
  if (strstr(replybuffer, "NORMAL POWER DOWN") != NULL) {
    powerUpOrDown();
  }
  return replybuffer;
}

void sendDataToServer(float dB, long epoch) {
  Serial.print(F("received dB: "));
  Serial.println(dB);
  SIM900.listen();
  char data[80];
  char str_spl[8];
  dtostrf(dB, 4, 1, str_spl);

  Serial.println("1");
  sprintf(data, "{\"datetime\":\"%ld\",\"spl\":\"%s\",\"deviceId\":\"160503605\"}", epoch, str_spl);
  Serial.println("2");
  Serial.println(data);
  Serial.println("3");
  delay(1000);
  Serial.println("4");
  SIM900.listen();
  delay(1000);
  Serial.println("5");
  SIM900.println(F("AT+SAPBR=2,1"));
  if (strstr(readData(4000), "+SAPBR:") == NULL) {
#ifdef DEBUG_UBIDOTS
    Serial.println(F("Error with AT+SAPBR=2,1 no IP to show"));
#endif
  }

  SIM900.println(F("AT+HTTPINIT"));
  if (strstr(readData(1000), "OK") == NULL) {
#ifdef DEBUG_UBIDOTS
    Serial.println(F("Error with AT+HTTPINIT. Reset the Arduino, will you?"));
#endif
  }

  sim900_send_cmd("AT+HTTPPARA=\"CID\",1", "OK");
  sim900_send_cmd("AT+HTTPPARA=\"URL\",\"scarlet.tapgo.cc/log\"", "OK");
  sim900_send_cmd("AT+HTTPPARA=\"CONTENT\",\"application/json\"", "OK");

  char httpdata1[32];
  sprintf(httpdata1, "AT+HTTPDATA=%d,%ld", strlen(data), 120000);
  //  Serial.println();
  //  Serial.println(httpdata1);
  sim900_send_cmd(httpdata1, "DOWNLOAD");
  SIM900.write(data, strlen(data));
  delay(1000);

  sim900_send_cmd("AT+HTTPACTION=1", "+HTTPACTION=1,200");
  delay(3000);
  sim900_send_cmd("AT+HTTPREAD", "+HTTPREAD:");

  SIM900.println("AT+HTTPTERM");
  if (strstr(readData(1000), "OK") == NULL) {
#ifdef DEBUG_UBIDOTS
    Serial.println(F("No HTTP to close"));
#endif
  }
}

void sim900_send_cmd(const char* cmd, const char* resp)
{
  char c = 0;
  int len = strlen(resp);
  int sum = 0;

  SIM900.print(cmd);
  SIM900.print("\r");
  delay(100);

  while (SIM900.available() > 0) {
    c = SIM900.read();
    Serial.print(c);
  }
}

void sim900_flush_serial()
{
  while (SIM900.available() > 0) {
    SIM900.read();
  }
}

void powerUpOrDown()
{
  Serial.println(F("Power on GPRS shield."));
  digitalWrite(9, HIGH);
  delay(1000);
  digitalWrite(9, LOW);
  delay(5000);
}

void loop() {
  float dB1;
  if (millis() % 1000 < 10)
  {
    // Display system time
    DateTime now = rtc.now();
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    int h = now.hour();
    int m = now.minute();
    int s = now.second();
    long epoch = now.unixtime();

    if ( (m % 1) == 0 && s == 4 )
    {
      pressStartPause();
      Serial.println("Start Leq5");
    }

    if ( (m % 1) == 0 && s == 2 )
    {
      pressStop();
      Serial.println("End Leq5");
    }

    if (m % 1 == 0 && s == 40)
    {
      dB1 = getLeq5Data();
      //      float dB2 = random(380, 520) / 10.0;
      sendDataToServer(dB1, epoch);
    }
    delay(100);
  }
}

/*
   ST-109 functions
   TODO: compile ST-109 functions into library
*/

float getLeq5Data()
{
  Serial.println(F("getLeq5Data+"));
  ST109.listen();
  ST109.write(0x01);
  ST109.write(0x49);
  ST109.write(0x1E);
  delay(200);
  Serial.println("cmd1");

  // Read 9 bytes per ST-109 spec
  unsigned char data[9];
  int i = 0;
  Serial.print(F("<109> receive: "));
  while (ST109.available() > 0)
  {
    data[i] = ST109.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
    i++;
  }
  ST109.flush();
  if (data[0] != 0x06 || data[1] != 0x49 || i != 9 ) // discard data
  {
    Serial.println(F("discard data"));
    return -1;
  }

  // Send command to retrieve the index of last data in ST-109
  unsigned char indexL = data[6];
  unsigned char indexH = data[7];
  unsigned char p[4] = { 0x01, 0x52, indexL, indexH };
  unsigned char crc = packetCRC(p, 4); // Hard code magical num 4

  ST109.write(0x01);
  ST109.write(0x52);
  ST109.write(indexL);
  ST109.write(indexH);
  ST109.write(crc);
  delay(200);


  unsigned char data2[10];
  int j = 0;
  // Read 10 bytes per ST-109 spec
  Serial.print(F("<109> receive: "));
  while (ST109.available() > 0)
  {
    data2[j] = ST109.read();
    Serial.print(data2[j], HEX);
    Serial.print(" ");
    j++;
  }
  ST109.flush();

  float dB = 0.1 * ( data2[2] | data2[3] << 8 );
  Serial.print("dB: ");
  Serial.println(dB);
  Serial.println(F("getLeq5Data-"));
  return dB;
}

void pressStartPause()
{
  Serial.println("pressStartPause+");
  ST109.listen();
  // Key down
  Serial.println("<109> start key down");
  ST109.write(0x01);
  ST109.write(0x4B);
  ST109.write((uint8_t)0);
  ST109.write(0x10);
  ST109.write(0xE2);
  delay(200);
  while (ST109.available() > 0) {
    Serial.print(ST109.read(), HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Key up
  Serial.println("<109> start key up");
  ST109.write(0x01);
  ST109.write(0x4B);
  ST109.write((uint8_t)0);
  ST109.write((uint8_t)0);
  ST109.write(0x7F);
  delay(200);
  while (ST109.available() > 0) {
    Serial.print(ST109.read(), HEX);
    Serial.print(" ");
  }
  Serial.println();
  ST109.flush();
  Serial.println("pressStartPause-");
}

void pressStop()
{
  Serial.println("pressStop+");
  ST109.listen();
  // Key down
  Serial.println("<109> stop key down");
  ST109.write(0x01);
  ST109.write(0x4B);
  ST109.write(0x80);
  ST109.write((uint8_t)0);
  ST109.write(0x50);
  delay(200);
  while (ST109.available() > 0) {
    Serial.print(ST109.read(), HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Key up
  Serial.println("<109> stop key up.");
  ST109.write(0x01);
  ST109.write(0x4B);
  ST109.write((uint8_t)0);
  ST109.write((uint8_t)0);
  ST109.write(0x7F);
  delay(200);
  while (ST109.available() > 0) {
    Serial.print(ST109.read(), HEX);
    Serial.print(" ");
  }
  Serial.println();
  ST109.flush();
}
