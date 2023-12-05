#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <RTClib.h>

struct Time {
  int hour;
  int minute;
};

const int LCD_DISPLAY_ADDRESS = 0x27;
const int TOGGLE_ALARM_BUTTON = 3;
const int SNOOZE_BUTTON = 2;
const int SWITCH_MODE_BUTTON = 4;
const int MINUTES_BUTTON = 5;
const int HOURS_BUTTON = 6;
const int BUZZER = 12;
const int DEBOUNCE_DELAY = 200;
const int BACKLIGHT_TIMEOUT = 10 * 1000;
const int TOTAL_MODES = 5;
const int MODE_CLOCK_VIEW = 0,
          MODE_SET_TIME = 1,
          MODE_SET_ALARM = 2,
          MODE_SET_DIFFICULTY = 3,
          MODE_SET_SNOOZE = 4;
const int TOTAL_DIFFICULTIES = 3;
char difficulties[TOTAL_DIFFICULTIES] = { '+', '-', '*' };
const int DIFFICULTY_EASY = 0,
          DIFFICULTY_MEDIUM = 1,
          DIFFICULTY_HARD = 2;
const int MINUTE = 1000;
const int MIN_SNOOZE_LENGTH = 1;
const int MAX_SNOOZE_LENGTH = 60;
const int DEFAULT_SNOOZE_LENGTH = 10;

// State management.
int switchModePrevState, toggleAlarmPrevState, snoozePrevState;
unsigned long toggleAlarmButtonLastDebounce,
  switchModeButtonLastDebounce,
  snoozeButtonLastDebounce,
  minutesButtonLastDebounce,
  hoursButtonLastDebounce;

// Initial variables.
int mode = MODE_CLOCK_VIEW,
    difficulty = DIFFICULTY_EASY,
    snoozeLength = DEFAULT_SNOOZE_LENGTH;
bool haveAlarmSet = false,
     haveChangedTime = false,
     isAlarmRinging = false,
     showingQuestion = false;
unsigned long lastBacklightTrigger = millis();
int x, y, expectedAnswer, enteredAnswer;

Time setTime, alarmTime;

RTC_DS1307 rtc;
LiquidCrystal_I2C display(LCD_DISPLAY_ADDRESS, 16, 2);

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
  Time suggestion = { time.hour(), time.minute() };
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

void loop() {
  DateTime time = rtc.now();
  setTime.hour = time.hour(), setTime.minute = time.minute();
  processUpdates();

  if (showingQuestion) {
    display.setCursor(0, 1);
    display.print(x);
    display.print(" ");
    display.print(difficulties[difficulty]);
    display.print(" ");
    display.print(y);
    display.print(" = ");
    display.print(enteredAnswer);
    display.print("       ");
  }
  displayTime(time);
  Serial.println(haveAlarmSet);
  if (haveAlarmSet) {
    display.setCursor(14, 0);
    display.print("AL");
  } else {
    display.setCursor(14, 0);
    display.print("  ");
  }

  if (mode == MODE_SET_TIME) {
    display.clear();
    while (mode == MODE_SET_TIME) {
      processUpdates();
      display.setCursor(0, 0);
      display.print("Change time");
      display.setCursor(0, 1);
      printDigit(setTime.hour);
      display.print(":");
      printDigit(setTime.minute);
      display.print(":");
      printDigit(0);
    }
    if (haveChangedTime) {
      rtc.adjust(DateTime(time.year(), time.month(), time.day(), setTime.hour, setTime.minute, 0));
      haveChangedTime = false;
    }
    display.clear();
  }

  if (mode == MODE_SET_ALARM) {
    display.clear();
    alarmTime = getNextProperAlarmTime();
    while (mode == MODE_SET_ALARM) {
      processUpdates();
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
      processUpdates();
      display.setCursor(0, 0);
      display.print("Math difficulty");
      for (int i = 0; i < 16; i += 6) {
        display.setCursor(i, 1);
        bool isCurrentDifficulty = (i / 6) == difficulty;
        display.print(isCurrentDifficulty ? "[" : " ");
        display.print(difficulties[i / 6]);
        display.print(isCurrentDifficulty ? "]" : " ");
      }
    }
    display.clear();
  }

  if (mode == MODE_SET_SNOOZE) {
    display.clear();
    while (mode == MODE_SET_SNOOZE) {
      processUpdates();
      display.setCursor(0, 0);
      display.print("Snooze length");
      display.setCursor(0, 1);
      if (snoozeLength < 10) display.print(" ");
      display.print(snoozeLength);
      display.print(" minute");
      display.print(snoozeLength != 1 ? "s" : " ");
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

// Debounce flickering button clicks and dedupe updates.
int debounceButtonInput(int buttonPin, unsigned long* lastDebounceTime) {
  int reading = digitalRead(buttonPin); // Reading with debounce
  unsigned long int uptime = millis();
  if (reading == LOW && (uptime - *lastDebounceTime) > DEBOUNCE_DELAY) {
    *lastDebounceTime = uptime;
    return LOW;
  } else return HIGH;
}

// Generate random digits including +/- numbers and create
// preset answer error range.
void generateEquation() {
  randomSeed(analogRead(1)); // Refresh the random seed.

  int limit = difficulty == DIFFICULTY_EASY
    ? 20 // easy (-20, 20) +
    : DIFFICULTY_MEDIUM
    ? 30 // medium (-30, 30) -
    : 12; // hard (-12, 12) *

  do x = int(random(-limit, limit)); while (x == 0);
  do y = int(random(-limit, limit)); while (y == 0);

  int deltaRange = 0;

  switch (difficulty) {
    case DIFFICULTY_EASY:
      expectedAnswer = x + y;
      deltaRange = int(random(10, 20));
      break;
    case DIFFICULTY_MEDIUM:
      expectedAnswer = x - y;
      deltaRange = int(random(15, 25));
      break;
    case DIFFICULTY_HARD:
      expectedAnswer = x * y;
      deltaRange = int(random(20, 30));
      break;
  }

  int delta = 0;
  while (delta == 0) delta = int(random(-deltaRange, deltaRange));
  enteredAnswer = expectedAnswer - delta;
}

void processUpdates() {
  if (haveAlarmSet && !showingQuestion) {
    DateTime now = rtc.now();
    if (now.hour() == alarmTime.hour && now.minute() == alarmTime.minute) {
      // Ensure that we're in clock view and make sure to revert
      // the time changing action.
      if (haveChangedTime) {
        haveChangedTime = false;
        setTime.hour = now.hour();
        setTime.minute = now.minute();
      }
      generateEquation();
      digitalWrite(BUZZER, HIGH);
      showingQuestion = true;
      mode = 0;
      return;
    }
  }

  // Debounce the hold event by the specified delay and return the state.
  int toggleBtnState = debounceButtonInput(TOGGLE_ALARM_BUTTON, &toggleAlarmButtonLastDebounce);
  int snoozeBtnState = debounceButtonInput(SNOOZE_BUTTON, &snoozeButtonLastDebounce);
  int modeBtnState = debounceButtonInput(SWITCH_MODE_BUTTON, &switchModeButtonLastDebounce);
  int hourBtnState = debounceButtonInput(HOURS_BUTTON, &hoursButtonLastDebounce);
  int minuteBtnState = debounceButtonInput(MINUTES_BUTTON, &minutesButtonLastDebounce);

  if (mode == MODE_CLOCK_VIEW) {
    if (showingQuestion) {
      // If we're currently prompting user to solve a question,
      // set actions of each buttons and don't listen to other buttons.
      // Shouldn't set backlight timeout.
      if (hourBtnState == LOW) enteredAnswer--;
      if (minuteBtnState == LOW) enteredAnswer++;
      if (snoozeBtnState == LOW) {
        if (expectedAnswer == enteredAnswer) {
          isAlarmRinging = false;
          showingQuestion = false;
          digitalWrite(BUZZER, LOW);
          alarmTime.minute = alarmTime.minute + snoozeLength;
          while (alarmTime.minute >= 60) {
            alarmTime.hour = (alarmTime.hour + 1) == 24 ? 0 : alarmTime.hour + 1;
            alarmTime.minute -= 60;
          }
        }
      }
      if (toggleBtnState == LOW) {
        if (expectedAnswer == enteredAnswer) {
          isAlarmRinging = false;
          haveAlarmSet = false;
          showingQuestion = false;
          digitalWrite(BUZZER, LOW);
        } else {
          generateEquation();
        }
      }
    } else if (toggleBtnState != toggleAlarmPrevState) {
      // We're not showing the equation, which means we're in pure
      // clock mode. So, the default behavior of button.
      if (toggleBtnState == LOW) {
        if (!haveAlarmSet) {
          haveAlarmSet = true;
        } else {
          mode = 0; // Ensure that we're in clock view.
          showingQuestion = true;
          generateEquation();
          return;
        }
      }
      toggleAlarmPrevState = toggleBtnState;
    }
  }

  // Cycling between different modes. MODE button works everywhere.
  if (showingQuestion) {
    if (modeBtnState != switchModePrevState) {
      if (modeBtnState == LOW) mode = (mode + 2 > TOTAL_MODES) ? 0 : mode + 1;
      switchModePrevState = modeBtnState;
    }
  }

  // HOME button
  if (mode != MODE_CLOCK_VIEW && toggleBtnState == LOW) mode = 0;

  if (mode == MODE_SET_TIME) {
    if (hourBtnState == LOW) setTime.hour = (setTime.hour + 1 == 24) ? 0 : setTime.hour + 1;
    if (minuteBtnState == LOW) setTime.minute = (setTime.minute + 1 == 60) ? 0 : setTime.minute + 1;
    if (hourBtnState == LOW || minuteBtnState == LOW) haveChangedTime = true;
  }

  if (mode == MODE_SET_ALARM) {
    if (hourBtnState == LOW) alarmTime.hour = (alarmTime.hour + 1 == 24) ? 0 : alarmTime.hour + 1;
    if (minuteBtnState == LOW) alarmTime.minute = (alarmTime.minute + 1 == 60) ? 0 : alarmTime.minute + 1;
  }

  if (mode == MODE_SET_DIFFICULTY) {
    if (hourBtnState == LOW)
      difficulty = (difficulty - 1) < DIFFICULTY_EASY ? DIFFICULTY_HARD : difficulty - 1;
    if (minuteBtnState == LOW)
      difficulty = (difficulty + 1) > DIFFICULTY_HARD ? DIFFICULTY_EASY : difficulty + 1;
    if (minuteBtnState == LOW || hourBtnState == LOW) generateEquation();
  }

  if (mode == MODE_SET_SNOOZE) {
    if (hourBtnState == LOW)
      snoozeLength = (snoozeLength - 1) < MIN_SNOOZE_LENGTH ? MAX_SNOOZE_LENGTH : snoozeLength - 1;
    if (minuteBtnState == LOW)
      snoozeLength = (snoozeLength + 1) > MAX_SNOOZE_LENGTH ? MIN_SNOOZE_LENGTH : snoozeLength + 1;
  }

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
