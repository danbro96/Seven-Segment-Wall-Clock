/*
  7 segment display clock controller
  Made by Anton Alfonsson and Daniel Broström

  Version 0.0
  2021-03-23
*/

/* -------------------------------------------
  LIBRARIES
  -------------------------------------------*/
#include <Wire.h>
#include <DS3231.h>

/* -------------------------------------------
  HARDWARE CONSTANTS
  -------------------------------------------*/
#define SRCLK 2
#define RCLK 3
#define SRCLR 10
#define OE 11
#define SRL 12

#define DISPLAYS 4
#define SEGMENTS 8

#define BUTTONS 4
int btnPIN[] = {4, 5, 6, 7};

/* -------------------------------------------
  GLOBAL VARIABLES
  -------------------------------------------*/
RTClib myRTC;

byte data[DISPLAYS];
bool btnStates[BUTTONS];

int updateInterval = 1000 * 1; //Milliseconds

unsigned long lastUpdate = 0;

/* -------------------------------------------
  SETUP
  -------------------------------------------*/
void setup() {
  pinMode(RCLK, OUTPUT);
  pinMode(SRCLK, OUTPUT);
  pinMode(SRCLR, OUTPUT);
  pinMode(OE, OUTPUT);
  pinMode(SRL, OUTPUT);

  for (int i = 0; i < BUTTONS; i++) {
    pinMode(btnPIN[i], INPUT_PULLUP);
  }

  digitalWrite(OE, LOW);

  digitalWrite(SRCLR, LOW);
  delay(1);
  digitalWrite(SRCLR, HIGH);

  Serial.begin(57600);
  Wire.begin();

  //testDisplay1();
}

/* -------------------------------------------
  LOOP
  -------------------------------------------*/
void loop() {

  updateButtons();

  if (millis() - lastUpdate > updateInterval) {
    lastUpdate = millis();

    DateTime now = myRTC.now();

    timeToData(now);

    Serial.println("Updating time to: " + timeToStr(now));

    updateShiftRegister(data);
  }


  //testDisplaySeg();

  //checkSerialInp()

  delay(10);

}

/* -------------------------------------------
  FUNCTIONS
  -------------------------------------------*/

void timeToData(DateTime now) {
  int hour = now.hour();
  int minute = now.minute();
  data[3] = intToNum(floor(hour / 10), false);
  data[2] = intToNum(floor(hour % 10), true);
  data[1] = intToNum(floor(minute / 10), false);
  data[0] = intToNum(floor(minute % 10), false);

}

String timeToStr(DateTime now) {
  int hour = now.hour();
  int minute = now.minute();
  String timeStr = "";
  if (hour < 10) {
    timeStr = "0";
  }
  timeStr = timeStr + (String)hour + ":";
  if (minute < 10) {
    timeStr = "0";
  }
  timeStr = timeStr + (String)minute;
  return timeStr;
}
bool updateButtons() {

  for (int i = 0; i < BUTTONS; i++) {
    btnStates[i] = digitalRead(btnPIN[i]);
    //Serial.print("Button " + (String)i + ": " + (String)btnStates[i] + "     ");
  }
  //Serial.println("");

  //Return true if some state has changed?
  return true;
}

void updateShiftRegister(byte data[]) {
  digitalWrite(SRCLK, LOW);

  digitalWrite(RCLK, LOW);
  for (int i = 0; i < DISPLAYS; i++) {
    shiftOut(SRL, SRCLK, MSBFIRST, data[i]);
  }
  digitalWrite(RCLK, HIGH);

}

bool checkSerialInp() {
  if (Serial.available()) {
    byte output;
    String ipt = Serial.readString();
    ipt.trim();
    Serial.print("Input: " + ipt + "       Value to shift: ");
    if (ipt.length() == 1) {
      output = intToNum(ipt.toInt(), true);
      Serial.println(output, BIN);

    }
    for (int i = 0; i < DISPLAYS; i++) {
      data[i] = output;
    }
    updateShiftRegister(data);
    return true;
  }

  return false;
}

void testDisplay1() {
  Serial.println("Running Test 1");

  byte output = intToNum(-1, false);
  Serial.print("Setting all LOW ");
  Serial.println(output, BIN);
  for (int i = 0; i < DISPLAYS; i++) {
    data[i] = output;
  }
  updateShiftRegister(data);
  delay(1000);

  for (int i = -1; i < 10; i++) {
    output = intToNum(i, (i % 2) == 0);
    Serial.print("Setting all to " + (String) i + " ");
    Serial.println(output, BIN);

    for (int i = 0; i < DISPLAYS; i++) {
      data[i] = output;
    }
    updateShiftRegister(data);
    delay(500);
  }
  Serial.println("Done with test");
}

void testDisplaySeg() {
  Serial.println("Running Test 2: Segments");

  digitalWrite(SRCLR, LOW);
  delay(1);
  digitalWrite(SRCLR, HIGH);

  for (int i = 0; i < DISPLAYS * SEGMENTS; i++) {
    digitalWrite(RCLK, LOW);
    digitalWrite(SRCLK, LOW);
    if (i == 0) {
      digitalWrite(SRL, HIGH);
    } else {
      digitalWrite(SRL, LOW);
    }
    delay(1);
    digitalWrite(RCLK, HIGH);
    digitalWrite(SRCLK, HIGH);
    delay(200);
  }

  Serial.println("Done with test");
}

void printtime () {
  DateTime now = myRTC.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  Serial.print(" since midnight 1/1/1970 = ");
  Serial.print(now.unixtime());
  Serial.print("s = ");
  Serial.print(now.unixtime() / 86400L);
  Serial.println("d");
}

int intToNum(int inp, bool dot) {
  int extra = 0;
  if (dot) {
    extra = 128;
  }
  switch (inp) {
    case 0:
      return  0x3F + extra;
    case 1:
      return  0x06 + extra;
    case 2:
      return  0x5B + extra;
    case 3:
      return  0x4F + extra;
    case 4:
      return  0x66 + extra;
    case 5:
      return  0x6D + extra;
    case 6:
      return  0x7D + extra;
    case 7:
      return  0x07 + extra;
    case 8:
      return  0x7F + extra;
    case 9:
      return  0x6F + extra;
    default:
      return  0x00 + extra;
  }
}
