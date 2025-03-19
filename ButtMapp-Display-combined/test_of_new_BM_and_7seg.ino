#include <RTClib.h>
#include <Wire.h>
#include <TM1637Display.h>

// Pin definitions
const int CLK = 2;                // TM1637 Clock pin
const int DIO = 3;                // TM1637 Data pin
const int HOUR_BUTTON = 4;
const int MINUTE_BUTTON = 5;
const int ALARM_BUTTON = 6;
const int SNOOZE_BUTTON = 7;
const int BUZZER_PIN = 8;         // Add a buzzer pin

// Display modes
const int MODE_NORMAL = 0;
const int MODE_SET_CLOCK = 1;
const int MODE_SET_ALARM = 2;

// Time constants
const int DEBOUNCE_DELAY = 50;    // Short debounce delay (ms)
const int LONG_PRESS_TIME = 2000; // Long press detection time (ms)
const int SNOOZE_TIME = 5;        // Snooze time in minutes
const int ALARM_DURATION = 300000; // How long alarm sounds before auto-snooze (5 minutes)
const int DISPLAY_UPDATE_INTERVAL = 500; // Display refresh rate (ms)

// Global objects
RTC_DS1307 rtc;
TM1637Display display(CLK, DIO);

// Button state tracking
struct ButtonState{
  bool currentState;
  bool lastState;
  unsigned long pressTime;
  unsigned long lastDebounceTime;
};

ButtonState hourButton = {false, false, 0, 0};
ButtonState minuteButton = {false, false, 0, 0};
ButtonState alarmButton = {false, false, 0, 0};
ButtonState snoozeButton = {false, false, 0, 0};

// Time and alarm variables
int current_hour = 0;
int current_minute = 0;
int alarm_hour = 0;
int alarm_minute = 0;
bool alarm_enabled = false;
bool alarm_triggered = false;
unsigned long alarm_trigger_time = 0;
unsigned long last_display_update = 0;
int display_mode = MODE_NORMAL;
bool colon_on = true;

// Function declarations
void initializeHardware();
void initializeRTC();
bool checkShortButtonPress(int pin, ButtonState *button);
bool checkButtonLongPress(int pin, ButtonState *button);
void handleButtonEvents();
void updateDisplay();
void updateAlarmStatus();
void handleNormalMode();
void handleSetClockMode();
void handleSetAlarmMode();
void turnOffAlarm();
void snoozeAlarm();
void showAlarmTime();
void setCurrentTime();
void checkAlarm();
void blinkColon();
void waitForRelease();

void setup() {
  Serial.begin(9600);
  initializeHardware();
  Serial.println("Hardware initialized");
  initializeRTC();
  Serial.println("RTC initialized");
}

void loop() {
  handleButtonEvents();
  
  switch(display_mode) {
    case MODE_NORMAL:
      handleNormalMode();
      break;
    case MODE_SET_CLOCK:
      handleSetClockMode();
      break;
    case MODE_SET_ALARM:
      handleSetAlarmMode();
      break;
  }
  
  updateDisplay();
}

void initializeHardware() {
  
  pinMode(HOUR_BUTTON, INPUT_PULLUP);
  pinMode(MINUTE_BUTTON, INPUT_PULLUP);
  pinMode(ALARM_BUTTON, INPUT_PULLUP);
  pinMode(SNOOZE_BUTTON, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  
  display.setBrightness(7);  // Set display brightness (0-7)
  
  //Serial.println("Hardware initialized");
}

void initializeRTC() {
  Wire.begin();
  
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC!");
    while (1);
  }
  
  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("Time set from compile time");
  }
  
  // Synchronize current time from RTC
  DateTime now = rtc.now();
  current_hour = now.hour();
  current_minute = now.minute();
  
  //Serial.println("RTC initialized");
}

bool checkShortButtonPress(int pin, ButtonState *button) {
  bool reading = digitalRead(pin);
  bool result = false;
  unsigned long currentTime = millis();
  
  // Check if reading has changed
  if (reading != button->lastState) {
    button->lastDebounceTime = currentTime;
  }
  
  // Check if state has been stable
  if ((currentTime - button->lastDebounceTime) > DEBOUNCE_DELAY) {
    // If state has changed
    if (reading != button->currentState) {
      button->currentState = reading;
      
      // Record press time on button press
      if (button->currentState == HIGH) {
        button->pressTime = currentTime;
      } else {
        // On button release, check if it was a short press
        if ((currentTime - button->pressTime) < LONG_PRESS_TIME) {
          result = true;
        }
      }
    }
  }
  
  button->lastState = reading;
  return result;
}

bool checkButtonLongPress(int pin, ButtonState *button) {
  bool reading = digitalRead(pin);
  bool result = false;
  unsigned long currentTime = millis();
  
  // Check if reading has changed
  if (reading != button->lastState) {
    button->lastDebounceTime = currentTime;
  }
  
  // Check if state has been stable
  if ((currentTime - button->lastDebounceTime) > DEBOUNCE_DELAY) {
    // If state has changed
    if (reading != button->currentState) {
      button->currentState = reading;
      
      // Record press time on button press
      if (button->currentState == HIGH) {
        button->pressTime = currentTime;
      } else {
        // On button release, check if it was a long press
        if ((currentTime - button->pressTime) >= LONG_PRESS_TIME) {
          result = true;
        }
      }
    }
  }
  
  button->lastState = reading;
  return result;
}

void handleButtonEvents() {
  // Short press events
  if (checkShortButtonPress(HOUR_BUTTON, &hourButton)) {
    if (display_mode == MODE_SET_CLOCK || display_mode == MODE_SET_ALARM) {
      incrementHour();
      waitForRealese(HOUR_BUTTON, &hourButton);
    }
  }
  
  if (checkShortButtonPress(MINUTE_BUTTON, &minuteButton)) {
    if (display_mode == MODE_SET_CLOCK || display_mode == MODE_SET_ALARM) {
      incrementMinute();
      waitForRealese(MINUTE_BUTTON, &minuteButton);
    }
  }
  
  if (checkShortButtonPress(ALARM_BUTTON, &alarmButton)) {
    if (display_mode == MODE_NORMAL) {
      toggleAlarm();
      waitForRealese(ALARM_BUTTON, &alarmButton);
    } else if (display_mode == MODE_SET_ALARM) {
      display_mode = MODE_NORMAL;
      Serial.println("Alarm set");
    }
  }
  
  if (checkShortButtonPress(SNOOZE_BUTTON, &snoozeButton)) {
    if (display_mode == MODE_NORMAL) {
      if (alarm_triggered) {
        snoozeAlarm();
        waitForRealese(SNOOZE_BUTTON, &snoozeButton);
      } else {
        showAlarmTime();
        waitForRealese(SNOOZE_BUTTON, &snoozeButton);
      }
    } else if (display_mode == MODE_SET_CLOCK) {
      display_mode = MODE_NORMAL;
      setCurrentTime();
      waitForRealese(SNOOZE_BUTTON, &snoozeButton);
      Serial.println("Clock set");
    }
  }
  
  // Long press events
  if (checkButtonLongPress(ALARM_BUTTON, &alarmButton)) {
    if (display_mode == MODE_NORMAL) {
      display_mode = MODE_SET_ALARM;
      Serial.println("Entering alarm set mode");
    }
  }
  
  if (checkButtonLongPress(SNOOZE_BUTTON, &snoozeButton)) {
    if (display_mode == MODE_NORMAL) {
      display_mode = MODE_SET_CLOCK;
      Serial.println("Entering clock set mode");
    }
  }
}

void incrementHour() {
  if (display_mode == MODE_SET_CLOCK) {
    current_hour = (current_hour + 1) % 24;
    Serial.print("Current hour: ");
    Serial.println(current_hour);
  } else if (display_mode == MODE_SET_ALARM) {
    alarm_hour = (alarm_hour + 1) % 24;
    Serial.print("Alarm hour: ");
    Serial.println(alarm_hour);
  }
}

void incrementMinute() {
  if (display_mode == MODE_SET_CLOCK) {
    current_minute = (current_minute + 1) % 60;
    Serial.print("Current minute: ");
    Serial.println(current_minute);
  } else if (display_mode == MODE_SET_ALARM) {
    alarm_minute = (alarm_minute + 1) % 60;
    Serial.print("Alarm minute: ");
    Serial.println(alarm_minute);
  }
}

void toggleAlarm() {
  alarm_enabled = !alarm_enabled;
  if (alarm_triggered) {
    turnOffAlarm();
  }
  Serial.print("Alarm: ");
  Serial.println(alarm_enabled ? "ON" : "OFF");
}

void showAlarmTime() {
  int alarmValue = alarm_hour * 100 + alarm_minute;
  display.showNumberDecEx(alarmValue, 0b01000000, true);
  delay(2000);
}

void setCurrentTime() {
  // Set RTC time
  rtc.adjust(DateTime(2023, 1, 1, current_hour, current_minute, 0));
  Serial.println("RTC time updated");
}

void handleNormalMode() {
  // Get current time from RTC
  DateTime now = rtc.now();
  current_hour = now.hour();
  current_minute = now.minute();
  
  // Check alarm
  checkAlarm();
  
  // Check if alarm auto-snooze is needed
  if (alarm_triggered && millis() - alarm_trigger_time >= ALARM_DURATION) {
    snoozeAlarm();
  }
}

void handleSetClockMode() {
  // Nothing to do here - button events handle the time setting
}

void handleSetAlarmMode() {
  // Nothing to do here - button events handle the alarm setting
}

void updateDisplay() {
  unsigned long currentTime = millis();
  
  // Update display every DISPLAY_UPDATE_INTERVAL ms
  if (currentTime - last_display_update >= DISPLAY_UPDATE_INTERVAL) {
    last_display_update = currentTime;
    
    if (display_mode == MODE_NORMAL) {
      int displayValue = current_hour * 100 + current_minute;
      
      // Blink colon depending on alarm status
      if (alarm_enabled) {
        display.showNumberDecEx(displayValue, 0b01000000, true);
      } else {
        if (colon_on) {
          display.showNumberDecEx(displayValue, 0b01000000, true);
        } else {
          display.showNumberDecEx(displayValue, 0b00000000, true);
        }
        colon_on = !colon_on;
      }
    } else if (display_mode == MODE_SET_CLOCK) {
      int displayValue = current_hour * 100 + current_minute;
      colon_on = !colon_on;
      display.showNumberDecEx(displayValue, colon_on ? 0b01000000 : 0b00000000, true);
    } else if (display_mode == MODE_SET_ALARM) {
      int alarmValue = alarm_hour * 100 + alarm_minute;
      colon_on = !colon_on;
      display.showNumberDecEx(alarmValue, colon_on ? 0b01000000 : 0b00000000, true);
    }
  }
}

void checkAlarm() {
  if (alarm_enabled && !alarm_triggered) {
    DateTime now = rtc.now();
    if (now.hour() == alarm_hour && now.minute() == alarm_minute) {
      alarm_triggered = true;
      alarm_trigger_time = millis();
      Serial.println("ALARM TRIGGERED!");
      
      // Code to start buzzer or other alarm indicator
      digitalWrite(BUZZER_PIN, HIGH);
    }
  }
}

void turnOffAlarm() {
  alarm_triggered = false;
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("Alarm turned off");
}

void snoozeAlarm() {
  turnOffAlarm();
  
  // Add snooze time to alarm
  alarm_minute = (alarm_minute + SNOOZE_TIME) % 60;
  if (alarm_minute < SNOOZE_TIME) {
    alarm_hour = (alarm_hour + 1) % 24;
  }
  
  Serial.print("Alarm snoozed for ");
  Serial.print(SNOOZE_TIME);
  Serial.println(" minutes");
  Serial.print("New alarm time: ");
  Serial.print(alarm_hour);
  Serial.print(":");
  Serial.println(alarm_minute);
}


void waitForRealese (int pin, ButtonState *button) {
  while(button->currentState == 1)
  {
    delay(15);
    button->currentState = digitalRead(pin);
  }
}
