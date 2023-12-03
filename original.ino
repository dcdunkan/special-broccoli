#include <LiquidCrystal.h>
#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 RTC;
LiquidCrystal lcd(12,11,10,9,8,7);

// VARIABLES
int secs;
int mins;
int hrs;
int minsAl = 30; //default alarm time
int hrsAl = 6;
const int hrsBut = 6; //pins for buttons
const int minsBut = 5;
const int modeBut = 4;
const int alBut = 3;
const int snzBut = 2;
const int alarmOut = 13;
int modeButIn;
int modeButInPre;
int modeButInCount = 0;
int minsButIn;
int hrsButIn;
int alButIn;
int alButInPre;
int alButInCount = 0;
int snzButIn;
int snzButInPre;
int snzButInCount = 0;

int ans;
int z;
int x;
int y;
int oneProb = 0;
int timeCheck;
int snz = 0;
int mathDiff = 0;

long stopAl;
long snzTime;
long solveTime;
long snzTimeSet = 600;  // set time allowed for snooze in seconds (10 min)
long solveTimeSet = 30; //set time allowed to solve the math problem in seconds (30 sec)
long stopAlSet = 3600;  //set time for when alarm shuts off automatically (1 hr)
int h = 0; //alarm entry variable.  h is set when alarm starts
int j = 0; //solving entry variable. j is set when the solving starts
int k = 0; //snooze entry variable.  k is set when snooze starts
int soundAlarm = 0;

int LED_LCD = 14; //LCD backlight
unsigned long LEDtime = 0;

long debounceDelay = 200;
long hrsButWhen = 0;
long minsButWhen = 0;
long modeButWhen = 0;
long alButWhen = 0;
long snzButWhen = 0;
long nowmillis = 0;

void setup()   {
  pinMode(alarmOut, OUTPUT);  //alarm output
  pinMode(LED_LCD, OUTPUT); //LCD backlight output
  
  pinMode(hrsBut, INPUT);  //set input buttons
  pinMode(minsBut, INPUT);
  pinMode(modeBut, INPUT);
  pinMode(alBut, INPUT);
  pinMode(snzBut, INPUT);
  
  digitalWrite(hrsBut, HIGH); //set pullup resistors on
  digitalWrite(minsBut, HIGH);
  digitalWrite(modeBut, HIGH);
  digitalWrite(alBut, HIGH);
  digitalWrite(snzBut, HIGH);
 
  lcd.begin(16,2);
  lcd.clear();
  
  Wire.begin(); //need to talk to RTC
  RTC.begin();
  
  /*  Following lines useful for testing
  //analog pins 4 and 5 used for I2C comm
  
  //Power RTC from analog pins 2 and 3 (16, 17), useful for connecting RTC to main arduino board
  pinMode(16, OUTPUT);  
  pinMode(17, OUTPUT);
  digitalWrite(16, LOW);
  digitalWrite(17, HIGH);
  
  // following line sets the RTC to the date & time this sketch was compiled, usefull for testing
  RTC.adjust(DateTime(_DATE, __TIME_));
  */
}


void loop()   {
  disptime(); //display time
  buttons (); //Read buttons and count mode button
  
  while (modeButInCount == 1) {
    settime();
  }
  while (modeButInCount == 2) {
    setalarm();
  }
  while (modeButInCount == 3) {
    setmath(); //set math alarm difficulty
  }
  
  if (alButInCount == 1) {  //check for alarm
    alarm();
  }
  
  if (soundAlarm == 1) {
    alarmNoise();
  }
  else {
    noTone(alarmOut);
  }
  
  DateTime now = RTC.now(); //get time from RTC
  hrs = now.hour();
  mins = now.minute();
  secs = now.second();
}
  
void disptime() {    //Display Time on LCD
  lcd.setCursor(5,0);
  printDigits(hrs);
  lcd.print(':');
  printDigits(mins);
  lcd.print(':');
  printDigits(secs);
}

void printDigits(int digits){  //Code from Time Library example TimeRTC.  Prints  0 if needed
  if(digits < 10)
    lcd.print('0');
  lcd.print(digits);
}

int buttons() {
  //read buttons  
  int modeButRead = digitalRead(modeBut); //read with debounce
  nowmillis = millis();
  if (modeButRead == LOW  && (nowmillis - modeButWhen) > debounceDelay) {
    modeButWhen = nowmillis;
    modeButIn = LOW;
  }
  else {
    modeButIn = HIGH;
  }
  
  int alButRead = digitalRead(alBut); //read with debounce
  nowmillis = millis();
  if (alButRead == LOW  && (nowmillis - alButWhen) > debounceDelay) {
    alButWhen = nowmillis;
    alButIn = LOW;
  }
  else {
    alButIn = HIGH;
  }
  
  int snzButRead = digitalRead(snzBut); //read with debounce
  nowmillis = millis();
  if (snzButRead == LOW  && (nowmillis - snzButWhen) > debounceDelay) {
    snzButWhen = nowmillis;
    snzButIn = LOW;
  }
  else {
    snzButIn = HIGH;
  }
  
  int hrsButRead = digitalRead(hrsBut); //read with debounce
  nowmillis = millis();
  if (hrsButRead == LOW  && (nowmillis - hrsButWhen) > debounceDelay) {
    hrsButWhen = nowmillis;
    hrsButIn = LOW;
  }
  else {
    hrsButIn = HIGH;
  }
  
  int minsButRead = digitalRead(minsBut);  //read with debounce
  nowmillis = millis();
  if (minsButRead == LOW && (nowmillis - minsButWhen) > debounceDelay) {
    minsButWhen = nowmillis;
    minsButIn = LOW;
  }
  else {
    minsButIn = HIGH;
  }
  
  //'mode button count to cycle between modes.
  if (modeButIn != modeButInPre) {
    if (modeButIn == LOW) {
      modeButInCount++;
      }
    modeButInPre = modeButIn;
  }
  if (modeButInCount == 4) {  //back to clock
    modeButInCount = 0;
  }
  
  //alarm button on/off
  if (alButIn != alButInPre) {
    if (alButIn == LOW) {
      alButInCount++;
    }
    alButInPre = alButIn;
  }
  
  if (alButInCount == 2) {  //turn off alarm, reset alarm/math variables
    soundAlarm = 0;
    alButInCount = 0;
    timeCheck = 0;
    snzButInCount = 0;
    snz = 0;
    oneProb = 0;
    stopAl = 0;
    solveTime = 0;
    snzTime = 0;
    h = 0;
    j = 0;
    k = 0;
    lcd.clear();
  }
  
  //snooze button on/off
  if (snzButIn != snzButInPre) {
    if (snzButIn == LOW) {
      snzButInCount++;
    }
    snzButInPre = snzButIn;
  }
  
  //Use mins and hrs buttons for +/- buttons during math problem
  if (minsButIn == LOW) {
    ans++;
  }
  if (hrsButIn == LOW) {
    ans--;
  }
  
  //LCD backlight on/off and timer, start/turn on everytime any button pressed or Alarm sound
  if (modeButIn == LOW || alButIn == LOW || snzButIn == LOW || minsButIn == LOW || hrsButIn == LOW || soundAlarm == 1) {
    digitalWrite(LED_LCD, HIGH);  //turn on LCD backlight
    LEDtime = millis(); //time LED backlight started
  }
  
  if (millis() >= (LEDtime+8000)) { //turn off LED 8 sec after last button press
    digitalWrite(LED_LCD, LOW);
  }
    
  return modeButInCount;
  return alButInCount;
  return snzButInCount;
}

void settime() {
  buttons();
  int i = 0;
  
  lcd.setCursor(0,1);
  lcd.print("Set Time");
  
  //Set hours if hours button is pressed.  i++ indicates times has changed.
  if (hrsButIn == LOW) {
    hrs++;
    i++;
    if (hrs == 25) {
      hrs = 0;
    }
  }
  
  //Set mins if mins button is pressed.  i++ indicates times has changed.
  if (minsButIn == LOW) {
    mins++;
    i++;
    if (mins == 60) {
      mins = 0;
    }
  } 
  
  //Reset secs so set time starts at a clean 00.  
  secs = 0;
  
  //If a hrs or mins was changed, change on the RTC and display the change.
  if (i != 0) {
    DateTime now = RTC.now();
    RTC.adjust(DateTime(now.year(),now.month(),now.day(),hrs,mins,secs));
    disptime();
  }
}

int setalarm() {
  buttons();
  
  lcd.setCursor(0,1);
  lcd.print("Set Alarm:");
  
  if (hrsButIn == LOW) {
    hrsAl++;
    if (hrsAl == 25) {
      hrsAl = 0;
    }
  }
  if (minsButIn == LOW) {
    minsAl++;
    if (minsAl == 60) {
      minsAl = 0;
    }
  } 
  
  //Display Alarm Time
  lcd.setCursor(10,1);
  printDigits(hrsAl);
  lcd.print(':');
  printDigits(minsAl);
  
  if (modeButInCount == 0) {
    lcd.clear();
  }
  
  secs = 0;
  return hrsAl;
  return minsAl;
}

void alarm() {
  buttons();

  lcd.setCursor(0,1);  //display alarm is on
  lcd.print("AL");
  
  if (mins == minsAl && hrs == hrsAl) {  //check alarm time 
    timeCheck = 1;
    if (h = 0) {  //set snooze to 0 if alarm has not started yet
      snzButInCount = 0;
    }
  }
  
  if (timeCheck == 1) { // do alarm for 1 hour ... the if you havn't got up yet time, I hope your not home
    
    for(h; h<1 ; h++) {  //get time that alarm started only once
      DateTime now = RTC.now(); //get time from RTC
      stopAl = now.unixtime(); //time alarm started
    }
      
    if (snzButInCount == 0) {
      soundAlarm = 1; //sound alarm
    }
    else if (snzButInCount == 1) {
      soundAlarm = 0; //turn off alarm to give time to think
      
      for(j; j<1 ; j++) { // get time that solving started only once
        DateTime now = RTC.now(); //get time from RTC
        solveTime = now.unixtime(); //time solving started
      }
    }
    
    DateTime now = RTC.now();  //get current time to do checks with
    if (now.unixtime() >= (solveTime+solveTimeSet) && snz == 0) { //reset alarm after 30 sec solve time
      snzButInCount = 0;
      j = 0;
    }
    
    if (snz == 0) {
    domath();  //prompt problem
    }
    
    if (snz == 1) {  // if problem correct display count down of awarded snooze time
      lcd.setCursor(3,1);
      lcd.print("Enjoy! ");
      int snzCount = (snzTime + snzTimeSet) - now.unixtime(); //count down secs of snooze
      if(snzCount <100)
        lcd.print('0');
      if(snzCount < 10)
        lcd.print('00');
      lcd.print(snzCount);
    }
    
    if (snzButInCount == 2) { //hit the snooze button to enter answer
      if (ans == z && snz == 0) { //check answer
      
        for(k; k<1; k++) { //get time that snooze started only once
          DateTime now = RTC.now(); //get time from RTC
          snzTime = now.unixtime(); //time snooze started
          snz = 1;
          lcd.clear(); //clear problem
        }
      }
      else if (snz == 0){ //wrong answer
        snzButInCount = 0; //resound alarm to signify wrong answer
        j=0;
        
      }
    }
    
    if (now.unixtime() >= (snzTime+snzTimeSet) && snz == 1) { //reset alarm after 10 min snooze time
      oneProb = 0;
      snzButInCount = 0;
      snz = 0;
      j=0;  //reset solving time
      k=0;  //reset snooze time
      lcd.clear();
    }
  
    
    if (now.unixtime() > (stopAl+stopAlSet)) {  //reset alarm variables for next alarm after 1 hour
      soundAlarm = 0;
      alButInCount = 0;
      timeCheck = 0;
      snzButInCount = 0;
      snz = 0;
      oneProb = 0;
      stopAl = 0;
      snzTime = 0;
      solveTime = 0;
      h = 0;
      j = 0;
      k = 0;
      lcd.clear();
      
    }
  }
}

int domath() {  //display math problem
  buttons();
  randomSeed(analogRead(1)); // give pseudo-random sequence a different place to start, read playground tutorial
  
  if (mathDiff == 0) { // Use addition
    for(oneProb; oneProb<1 ; oneProb++) {  //set problem the first time only
      x = int(random(20));
      y = int(random(20));
      z = x + y;
      ans = int(random(5,15));
      if (ans == z) {
        ans = z-4;
      }
    }
  }
  
  if (mathDiff == 1) { //Use subtraction
    for(oneProb; oneProb<1 ; oneProb++) {  //set problem the first time only
      x = int(random(20));
      y = int(random(20));
      z = x - y;
      ans = int(random(5,15));
      if (ans == z) {
        ans = z-4;
      }
    }
  }

  if (mathDiff == 2) {  //Use multiplication
    for(oneProb; oneProb<1 ; oneProb++) {  //set problem the first time only
      x = int(random(20));
      y = int(random(20));
      z = x * y;
      ans = int(random(5,15));
      if (ans == z) {
        ans = z-4;
      }
    }
  }
  
  lcd.setCursor(3,1);
  lcd.print(x);
  if (mathDiff == 0) {
    lcd.print('+');
  }
  else if ( mathDiff == 1) {
    lcd.print('-');
  }
  else if (mathDiff == 2) {
    lcd.print('*');
  }
  lcd.print(y);
  lcd.print("=?");
  if (ans < 10 && ans > 0) {
    lcd.print('0');
  }
  lcd.print(ans);

  return ans;
  return z;
}

int setmath() {
  buttons();
  
  lcd.setCursor(0,1);
  lcd.print("Set Math Diff: ");
  
  if (hrsButIn == LOW) {
    mathDiff--;
    if (mathDiff < 0) {
      mathDiff = 2;
    }
  }
  if (minsButIn == LOW) {
    mathDiff++;
    if (mathDiff >= 3) {
      mathDiff = 0;
    }
  } 
  
  //Display Math difficulty setting
  lcd.setCursor(14,1);
  lcd.print(mathDiff);
  
  if (modeButInCount == 0) {
    lcd.clear();
  }
  
  return mathDiff;
}  

void alarmNoise() { //make noise for alarm
  tone(alarmOut,262);  //
}
