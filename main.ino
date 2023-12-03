#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <RTClib.h>

const int LCD_DISPLAY_ADDRESS = 0x27;
const int TOGGLE_ALARM_BUTTON = 3;
const int SNOOZE_BUTTON = 2;
const int SWITCH_MODE_BUTTON = 4;
const int MINUTES_BUTTON = 5;
const int HOURS_BUTTON = 6;
const int BUZZER = 12;
const unsigned long DEBOUNCE_DELAY = 200;

// There are 5 modes.
// 0. Normal clock view
// 1. Reset time
// 2. Set alarm
// 3. Choose maths difficulty
// 4. Configure snooze
const int TOTAL_MODES = 5;

int switchModePrevState, toggleAlarmPrevState;

unsigned long toggleAlarmButtonLastDebounce,
  switchModeButtonLastDebounce,
  snoozeButtonLastDebounce,
  minutesButtonLastDebounce,
  hoursButtonLastDebounce;

RTC_DS1307 rtc;
LiquidCrystal_I2C display(LCD_DISPLAY_ADDRESS, 16, 2);

// Variables which affects the visual behavior.
int mode = 0;
bool alarmSet = false;

unsigned long int lastBacklightTrigger = millis();
const int BACKLIGHT_TIMEOUT = 10 * 1000;

bool changedTime = false;

struct Time {
  int hour;
  int minute;
};

Time setTime;
Time alarmTime;

void setup() {
  Serial.begin(9600);
  Wire.begin();  // Scan the module for devices.

  // Setup pins.
  pinMode(TOGGLE_ALARM_BUTTON, INPUT);
  pinMode(SNOOZE_BUTTON, INPUT);
  pinMode(SWITCH_MODE_BUTTON, INPUT);
  pinMode(MINUTES_BUTTON, INPUT);
  pinMode(HOURS_BUTTON, INPUT);
  pinMode(BUZZER, OUTPUT);

  // Reset button inputs.
  digitalWrite(TOGGLE_ALARM_BUTTON, HIGH);
  digitalWrite(SNOOZE_BUTTON, HIGH);
  digitalWrite(SWITCH_MODE_BUTTON, HIGH);
  digitalWrite(HOURS_BUTTON, HIGH);
  digitalWrite(MINUTES_BUTTON, HIGH);

  // Setup RTC Module and set the time to current local time.
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // Configure the display.
  display.init();
  display.clear();
  display.backlight();
}

// An algorithm implementation to find the next probable alarm time.
Time getNextProperAlarmTime() {
  DateTime time = rtc.now();
  Time suggestion = {time.hour(), time.minute()};
  if ((60 - time.minute()) < 15) {
    suggestion.hour = (suggestion.hour + 1 == 24) ? 0 : suggestion.hour + 1;
    suggestion.minute = 0;
  } else {
    while (suggestion.minute % 15 != 0) suggestion.minute++;
    if (suggestion.minute == 60) {
      suggestion.hour = (suggestion.hour + 1 == 24) ? 0 : suggestion.hour + 1;
      suggestion.minute = 0;
    }
  }
  return suggestion;
}

const int MODE_CLOCK_VIEW = 0,
          MODE_SET_TIME = 1,
          MODE_SET_ALARM = 2,
          MODE_SET_DIFFICULTY = 3,
          MODE_SET_SNOOZE = 4;

void loop() {
  DateTime time = rtc.now();
  setTime.hour = time.hour(), setTime.minute = time.minute();
  updateButtonStates();


  // updateButtonStates();
  displayTime(time);
  if (alarmSet) {
    display.setCursor(14, 0);
    display.print("AL");
  } else {
    display.setCursor(14, 0);
    display.print("  ");
  }

  if (mode == MODE_SET_TIME) {
    display.clear();
    while (mode == MODE_SET_TIME) {
      updateButtonStates();
      display.setCursor(0, 0);
      display.print("Change time");
      display.setCursor(0, 1);
      printDigit(setTime.hour);
      display.print(":");
      printDigit(setTime.minute);
      display.print(":");
      printDigit(0);
    }
    if (changedTime) {
      rtc.adjust(DateTime(time.year(), time.month(), time.day(), setTime.hour, setTime.minute, 0));
      changedTime = false;
    }
    display.clear();
  }

  if (mode == MODE_SET_ALARM) {
    display.clear();
    alarmTime = getNextProperAlarmTime();
    while (mode == MODE_SET_ALARM) {
      updateButtonStates();
      display.setCursor(0, 0);
      display.print("Set alarm");
      display.setCursor(0, 1);
      printDigit(alarmTime.hour);
      display.print(":");
      printDigit(alarmTime.minute);
    }
    display.clear();
  }

  if (mode == MODE_SET_DIFFICULTY) {
    display.clear();
    while (mode == MODE_SET_DIFFICULTY) {
      updateButtonStates();
      display.setCursor(0, 0);
      display.print("Math difficulty");
      display.setCursor(0, 1);
      display.print("INCOMPLETE");
    }
    display.clear();
  }


  if (mode == MODE_SET_SNOOZE) {
    display.clear();
    while (mode == MODE_SET_SNOOZE) {
      updateButtonStates();
      display.setCursor(0, 0);
      display.print("Snooze interval");
      display.setCursor(0, 1);
      display.print("INCOMPLETE");
    }
    display.clear();
  }

  // Edge-case: invalid mode.
  if (mode >= TOTAL_MODES) {
    display.clear();
    display.setCursor(0, 0);
    display.print("ERROR:");
    display.setCursor(0, 1);
    display.print("No such mode.");
    delay(3 * 1000);
    display.clear();
    mode = 0;
  }

  // Backlight timeout.
  if (millis() >= lastBacklightTrigger + BACKLIGHT_TIMEOUT) {
    display.noBacklight();
  }
}

int debounceButtonInput(int buttonPin, unsigned long* lastDebounceTime) {
  int reading = digitalRead(buttonPin);  // Reading with debounce
  unsigned long int uptime = millis();
  if (reading == LOW && (uptime - *lastDebounceTime) > DEBOUNCE_DELAY) {
    *lastDebounceTime = uptime;
    return LOW;
  } else return HIGH;
}

void updateButtonStates() {
  // Debounce the hold event by the specified delay and return the state.
  int toggleBtnState = debounceButtonInput(TOGGLE_ALARM_BUTTON, &toggleAlarmButtonLastDebounce);
  int snoozeBtnState = debounceButtonInput(SNOOZE_BUTTON, &snoozeButtonLastDebounce);
  int modeBtnState = debounceButtonInput(SWITCH_MODE_BUTTON, &switchModeButtonLastDebounce);
  int hourBtnState = debounceButtonInput(HOURS_BUTTON, &hoursButtonLastDebounce);
  int minuteBtnState = debounceButtonInput(MINUTES_BUTTON, &minutesButtonLastDebounce);

  // Cycling between different modes.
  if (modeBtnState != switchModePrevState) {
    if (modeBtnState == LOW) mode = (mode + 2 > TOTAL_MODES) ? 0 : mode + 1;
    switchModePrevState = modeBtnState;
  }

  // Toggle alarm
  if (mode == MODE_CLOCK_VIEW) {
    if (toggleBtnState != toggleAlarmPrevState) {
      if (toggleBtnState == LOW) alarmSet = !alarmSet;
      toggleAlarmPrevState = toggleBtnState;
    }

    // Snooze alarm
  }

  if (mode == MODE_SET_TIME) {
    if (hourBtnState == LOW) setTime.hour = (setTime.hour + 1 == 24) ? 0 : setTime.hour + 1;
    if (minuteBtnState == LOW) setTime.minute = (setTime.minute + 1 == 60) ? 0 : setTime.minute + 1;
    if (hourBtnState == LOW || minuteBtnState == LOW) changedTime = true;
  }

  if (mode == MODE_SET_ALARM) {
    if (hourBtnState == LOW) alarmTime.hour = (alarmTime.hour + 1 == 24) ? 0 : alarmTime.hour + 1;
    if (minuteBtnState == LOW) alarmTime.minute = (alarmTime.minute + 1 == 60) ? 0 : alarmTime.minute + 1;
  }

  if (mode == MODE_SET_DIFFICULTY) {}
  if (mode == MODE_SET_SNOOZE) {}

  // Timeout backlight after some amount of time as specified.
  if (modeBtnState == LOW || toggleBtnState == LOW || snoozeBtnState == LOW || hourBtnState == LOW || minuteBtnState == LOW) {
    display.backlight();
    lastBacklightTrigger = millis();
  }
}

void displayTime(DateTime time) {
  display.setCursor(4, 0);
  printDigit(time.hour());
  display.print(":");
  printDigit(time.minute());
  display.print(":");
  printDigit(time.second());
}

void printDigit(int digit) {
  if (digit < 10) display.print("0");
  display.print(digit);
}
