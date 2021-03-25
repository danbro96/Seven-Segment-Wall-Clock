/*
  7 segment display clock controller
  Made by Anton Alfonsson and Daniel Broström

  Version 0.0
  2021-03-23
*/

/* -------------------------------------------
  LIBRARIES
  -------------------------------------------*/
#include <RTClib.h>

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
RTC_DS3231 rtc;

byte data[DISPLAYS];
bool btnState[BUTTONS];
bool btnStateChange[BUTTONS];

int updateInterval = 500; //Milliseconds

#define NORMAL 0
#define SET_TIME 1
int mode = NORMAL;

DateTime newTime;

#define HOUR 0
#define MINUTE 1
int menuItem = HOUR;

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
  while (!Serial); // wait for serial port to connect. Needed for native USB

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    //  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  testDisplaySeg();
  Serial.println("Finished initialising.");
}

/* -------------------------------------------
  MAIN LOOP
  -------------------------------------------*/
void loop() {

  checkButtons();

  if (checkSerialInp()) {
    Serial.println("Serial input success!");
  }
  if (rtc.lostPower()) {
    digitalWrite(OE, ((millis() / 1000) % 2) == 0);
    Serial.println("Set new time!");
  } else if (mode == NORMAL) {
    if (millis() - lastUpdate > updateInterval) {
      lastUpdate = millis();
      //Serial.println("Temp: " + (String)rtc.getTemperature());
      displayCurrentTime();
    }
  } else if (mode == SET_TIME) {
    switchMenu();
    if (millis() - lastUpdate > updateInterval / 2 || btnStateChange[0] || btnStateChange[3]) {
      lastUpdate = millis();
      changeNewTime();
    }
    displayNewTime();
  }
  delay(10);
}

/* -------------------------------------------
  BUTTON FUNCTIONS
  -------------------------------------------*/
void checkButtons() {
  for (int i = 0; i < BUTTONS; i++) {
    bool newState = !digitalRead(btnPIN[i]);
    btnStateChange[i] = newState != btnState[i];
    btnState[i] = newState;
  }
  for (int i = 0; i < BUTTONS; i++) {
    if (btnStateChange[i]) {
      Serial.println("Button " + (String)i + " changed state to: " + (String)btnState[i]);

      //If middle buttons are pressed while in NORMAL mode, enter SET_TIME mode.
      if (!btnState[0] && btnState[1] && btnState[2] && !btnState[3] && (mode == NORMAL || rtc.lostPower())) {
        Serial.println("Entering buttonbased time-setting!");
        mode = SET_TIME;
        newTime = rtc.now();
        menuItem = HOUR;
        blinkDisplay(5, 200);
        break;

        //If middle buttons are pressed while in SET_TIME mode, enter NORMAL mode and adjust RTC time.
      }  else if (!btnState[0] && btnState[1] && btnState[2] && !btnState[3] && mode == SET_TIME) {
        Serial.println("Leaving buttonbased time-setting. Sending new time to RTC!");
        mode = NORMAL;
        rtc.adjust(newTime);
        fancyBlink();
        break;
      }
    }
  }
}

/* -------------------------------------------
  SET TIME FUNCTIONS
  -------------------------------------------*/
void switchMenu() {
  if (!btnState[0] && !btnState[1] && btnState[2] && !btnState[3] && btnStateChange[2]) {
    menuItem++;
        if (menuItem > 1){
      menuItem = 0;
    }
  } else if (!btnState[0] && btnState[1] && !btnState[2] && !btnState[3] && btnStateChange[1]) {
    menuItem--;
    if (menuItem < 0){
      menuItem = 1;
    }
  }
}
void changeNewTime() {

  int hour = newTime.hour();
  int minute = newTime.minute();

  //Change hour
  if (menuItem == HOUR) {
    if (!btnState[0] && !btnState[1] && !btnState[2] && btnState[3]) {
      hour++;
      if (hour > 23) {
        hour = 0;
      }
      Serial.println("Hour: " + (String)hour);
    } else if (btnState[0] && !btnState[1] && !btnState[2] && !btnState[3]) {
      hour--;
      if (hour < 0) {
        hour = 23;
      }
      Serial.println("Hour: " + (String)hour);
    }

    //Change minute
  } else if (menuItem == MINUTE) {
    if (!btnState[0] && !btnState[1] && !btnState[2] && btnState[3]) {
      minute ++;
      if (minute > 59) {
        minute = 0;
        hour++;
      }
      Serial.println("Minute: " + (String)minute);
    } else if (btnState[0] && !btnState[1] && !btnState[2] && !btnState[3]) {
      minute--;
      if (minute < 0) {
        minute = 59;
        hour--;
      }
      Serial.println("Minute: " + (String)minute);
    }
  }

  newTime = DateTime(2021, 01, 01, hour, minute, 00);
}
void displayNewTime() {
  //digitalWrite(OE, ((millis() / 1000) % 2) == 0);


  bool blinkSeg = ((millis() / 100) % 3) == 0;
  timeToData(newTime, ((millis() / 1000) % 2) == 0, !(blinkSeg && menuItem == HOUR), !(blinkSeg && menuItem == HOUR), !(blinkSeg && menuItem == MINUTE), !(blinkSeg && menuItem == MINUTE));
  updateShiftRegister(data);
}
/* -------------------------------------------
  TIME DISPLAY FUNCTIONS
  -------------------------------------------*/
void displayCurrentTime() {
  DateTime nowRTC = rtc.now();
  Serial.println("Displaying time: " + timeToStr(nowRTC));

  timeToData(nowRTC);
  updateShiftRegister(data);
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

void timeToData(DateTime timeNow) {
  int hour = timeNow.hour();
  int minute = timeNow.minute();
  int second = timeNow.second();

  data[3] = intToNum(floor(hour / 10), false);
  data[2] = intToNum(floor(hour % 10), true);
  data[1] = intToNum(floor(minute / 10), false);
  data[0] = intToNum(floor(minute % 10), false);
}
void timeToData(DateTime timeNow, bool dot) {
  int hour = timeNow.hour();
  int minute = timeNow.minute();
  int second = timeNow.second();

  data[3] = intToNum(floor(hour / 10), dot);
  data[2] = intToNum(floor(hour % 10), dot);
  data[1] = intToNum(floor(minute / 10), dot);
  data[0] = intToNum(floor(minute % 10), dot);
}
void timeToData(DateTime timeNow, bool dot, bool e0, bool e1, bool e2, bool e3) {
  int hour = timeNow.hour();
  int minute = timeNow.minute();
  int second = timeNow.second();

  if (e0) {
    data[3] = intToNum(floor(hour / 10), dot);
  } else {
    data[3] = intToNum(-1, dot);
  }

  if (e1) {
    data[2] = intToNum(floor(hour % 10), dot);
  } else {
    data[2] = intToNum(-1, dot);
  }

  if (e2) {
    data[1] = intToNum(floor(minute / 10), dot);
  } else {
    data[1] = intToNum(-1, dot);
  }

  if (e3) {
    data[0] = intToNum(floor(minute % 10), dot);
  } else {
    data[0] = intToNum(-1, dot);
  }
}
String timeToStr(DateTime timeNow) {
  int hour = timeNow.hour();
  int minute = timeNow.minute();
  return (String)(int)floor(hour / 10) + (String)(int)floor(hour % 10) + ":" + (String)(int)floor(minute / 10) + (String)(int)floor(minute % 10);
}

void updateShiftRegister(byte data[]) {
  digitalWrite(SRCLK, LOW);

  digitalWrite(RCLK, LOW);
  for (int i = 0; i < DISPLAYS; i++) {
    shiftOut(SRL, SRCLK, MSBFIRST, data[i]);
  }
  digitalWrite(RCLK, HIGH);
}

/* -------------------------------------------
  SERIAL FUNCTIONS
  -------------------------------------------*/
bool checkSerialInp() {
  // Checks for serial commands.
  // The inputs must be structured in the form *command*:*variables*x, example:
  // sDate:210101235900x

  if (Serial.available()) {
    String inStr = Serial.readString();
    inStr.trim();
    String cmdStr = inStr.substring(0, inStr.lastIndexOf(":"));
    Serial.print("Command: " + cmdStr);
    cmdStr.toLowerCase();
    String varStr = inStr.substring(inStr.lastIndexOf(":") + 1, inStr.lastIndexOf("x"));
    Serial.print(", with " + (String)varStr.length() + " variables: " + varStr);

    if (cmdStr.equals("sdate") && varStr.length() == 12) {
      Serial.println(", Set new RTC time!");
      mode = NORMAL;

      DateTime current = getSerialDate(varStr);

      rtc.adjust(current);
      fancyBlink();
      delay(100);
      return true;
    } else {
      Serial.println(", Not recognized as a command!");
    }
  }
  return false;
}

DateTime getSerialDate(String InString) {
  // Call this if you notice something coming in on
  // the serial port. The stuff coming in should be in
  // the order of the incoming string must be YYMMDDHHMMSS
  byte Year;
  byte Month;
  byte Day;
  byte Hour;
  byte Minute;
  byte Second;

  byte Temp1, Temp2;

  // Read Year first
  Temp1 = (byte)InString[0] - 48;
  Temp2 = (byte)InString[1] - 48;
  Year = Temp1 * 10 + Temp2;
  // now month
  Temp1 = (byte)InString[2] - 48;
  Temp2 = (byte)InString[3] - 48;
  Month = Temp1 * 10 + Temp2;
  // now date
  Temp1 = (byte)InString[4] - 48;
  Temp2 = (byte)InString[5] - 48;
  Day = Temp1 * 10 + Temp2;
  // now Hour
  Temp1 = (byte)InString[6] - 48;
  Temp2 = (byte)InString[7] - 48;
  Hour = Temp1 * 10 + Temp2;
  // now Minute
  Temp1 = (byte)InString[8] - 48;
  Temp2 = (byte)InString[9] - 48;
  Minute = Temp1 * 10 + Temp2;
  // now Second
  Temp1 = (byte)InString[10] - 48;
  Temp2 = (byte)InString[11] - 48;
  Second = Temp1 * 10 + Temp2;

  return DateTime(Year, Month, Day, Hour, Minute, Second);
}

/* -------------------------------------------
  ANIMATION FUNCTIONS
  -------------------------------------------*/
void fancyBlink() {
  for (int y = 100; 20 < y; y = y - 20) {
    digitalWrite(OE, HIGH);
    delay(y);
    digitalWrite(OE, LOW);
    delay(y);
  }
}
void blinkDisplay(int freq, int duration) {
  //freq is blink frequency in Hz
  //duration is time in ms

  int start = millis();
  while (millis() - start < duration) {
    digitalWrite(OE, HIGH);
    delay(1000 / freq / 2);
    digitalWrite(OE, LOW);
    delay(1000 / freq / 2);
  }
}
void testDisplayNums() {
  //Loop through all numbers shown on all displays simultaneously
  Serial.println("Running Test 1: Numbers");

  for (int i = -1; i < 10; i++) {
    byte output = intToNum(i, (i % 2) == 0);
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
  //Loop through all display segments in order
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
    delay(20);
  }

  Serial.println("Done with test");
}
