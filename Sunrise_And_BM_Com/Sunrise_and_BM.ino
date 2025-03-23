#include <RtcDs1302.h>
#include <Wire.h>
#include <TM1637Display.h>
#include "sunrise.h"

// Pin definitions
const int CLK = 2;                // TM1637 Clock pin
const int DIO = 3;                // TM1637 Data pin
const int HOUR_BUTTON = 4;
const int MINUTE_BUTTON = 5;
const int ALARM_BUTTON = 6;
const int SNOOZE_BUTTON = 7;
//const int BUZZER_PIN = 8;         // Add a buzzer pin
const int RST_PIN = 8;            // Add a reset pin
const int COLD_LED_PIN = 9;           // Add LED 1 pin
const int WARM_LED_PIN = 10;           // Add LED 2 pin
const int DAT_PIN = 12;           // Add a data pin
const int CLK_PIN = 13;           // Add a clock pin


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

// Time and alarm variables
int current_hour = 0; // Start from 12:00
int current_minute = 0;
int current_second = 0;
int alarm_hour = 7;     // Default alarm time
int alarm_minute = 0;
bool alarm_enabled = false;
bool alarm_triggered = false;
unsigned long alarm_trigger_time = 0;
unsigned long last_display_update = 0;
int display_mode = MODE_NORMAL;
bool colon_on = true;
bool display_needs_update = true;
bool showing_alarm_preview = false;
unsigned long alarm_preview_end_time = 0;
unsigned long lastTimeUpdate = 0;

// Global objects
ThreeWire myWire(DAT_PIN, CLK_PIN, RST_PIN);
RtcDS1302<ThreeWire> rtc(myWire);
TM1637Display display(CLK, DIO);

Sunrise sunrise;

uint32_t old_time {millis()};
uint32_t current_time {millis()};
uint32_t timer {0};

RtcDateTime now;
Alarm_Target alarm_target { alarm_hour, alarm_minute }; // Set default alarm to 6:30 AM
Alarm_State alarm_state { true, alarm_target };


// Button state tracking - Note: Using INPUT_PULLUP, so LOW is pressed, HIGH is released
struct ButtonState {
  bool currentState;  // Debounced state - LOW when pressed
  bool lastState;     // Previous raw reading
  unsigned long pressTime;
  unsigned long lastDebounceTime;
};

ButtonState hourButton = {HIGH, HIGH, 0, 0};
ButtonState minuteButton = {HIGH, HIGH, 0, 0};
ButtonState alarmButton = {HIGH, HIGH, 0, 0};
ButtonState snoozeButton = {HIGH, HIGH, 0, 0};



// Function declarations
void initializeHardware();
void initializeRTC();
void handleButtonEvents();
void updateDisplay();
void updateAlarmStatus();
void handleNormalMode();
void handleSetClockMode();
void handleSetAlarmMode();
void turnOffAlarm();
void snoozeAlarm();
void showAlarmTime();
void checkAlarm();
void waitForRelease(int pin, ButtonState *button);
void incrementHour();
void decrementHour();
void incrementMinute();
void decrementMinute();
void toggleAlarm();
void waitForRelease(int pin, ButtonState *button);
void updateCurrentTimeFromRTC();
void incrementRtcDateTime(RtcDateTime& dt, uint32_t seconds);
void setCurrentTime(int hour, int minute, int second);


void setup() {
  Serial.begin(9600);
  
  // Initialize the RTC
  rtc.Begin();
    
  // Check if the RTC is valid
  if (!rtc.GetIsRunning()) {
    Serial.println("RTC is not running! Setting the time.");
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    rtc.SetDateTime(compiled);
  }

  Serial.println("RTC time set");

  // Get current time from RTC
  now = rtc.GetDateTime();
  
  // Set up the sunrise alarm time
  alarm_state.alarm_time = alarm_target.next(now);

  // Initialize the sunrise module
  sunrise.setup(WARM_LED_PIN, COLD_LED_PIN);
  
  // Initialize the TM1637 display and other hardware
  initializeHardware();
  
  // Immediately update time from RTC
  updateCurrentTimeFromRTC();
  
  // Force display update
  display_needs_update = true;
}

void loop() {
  // Update the current time from RTC at regular intervals
  if (millis() - lastTimeUpdate > 1000) {
    now = rtc.GetDateTime();
    updateCurrentTimeFromRTC();
    lastTimeUpdate = millis();
  }
  
  // Update sunrise module
  sunrise(alarm_state, now);
  
  // Now handle the appropriate mode
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
  
  // Then check for button presses after we've handled the current mode
  handleButtonEvents();
  
  // Finally, update the display
  updateDisplay();
}


void incrementRtcDateTime(RtcDateTime& dt, uint32_t seconds) {
  uint32_t total_seconds = dt.TotalSeconds() + seconds;
  dt = RtcDateTime(total_seconds);
}


void setCurrentTime(int hour, int minute, int second) {
  // Create a new RTC time with current date but new time
  RtcDateTime currentTime = rtc.GetDateTime();
  RtcDateTime newTime(currentTime.Year(), currentTime.Month(), currentTime.Day(), 
                      hour, minute, second);
  
  // Set the RTC
  rtc.SetDateTime(newTime);
  
  // Update our now variable
  now = newTime;
  
  Serial.print("RTC time set to: ");
  Serial.print(hour);
  Serial.print(":");
  if (minute < 10) Serial.print("0");
  Serial.print(minute);
  Serial.print(":");
  if (second < 10) Serial.print("0");
  Serial.println(second);
  
  // Update our local time variables
  current_hour = hour;
  current_minute = minute;
  current_second = second;
  
  // Update alarm time based on new current time
  alarm_state.alarm_time = alarm_target.next(now);
}


void updateCurrentTimeFromRTC() {
  // Store previous values to detect changes
  int prev_hour = current_hour;
  int prev_minute = current_minute;
  
  // Update current time
  current_hour = now.Hour();
  current_minute = now.Minute();
  current_second = now.Second();
  
  // If time has changed, update display
  if (prev_hour != current_hour || prev_minute != current_minute) {
    display_needs_update = true;
    
    // Debug output
    Serial.print("Time updated: ");
    Serial.print(current_hour);
    Serial.print(":");
    if (current_minute < 10) Serial.print("0");
    Serial.print(current_minute);
    Serial.print(":");
    if (current_second < 10) Serial.print("0");
    Serial.println(current_second);
  }
}

void initializeHardware() {
  pinMode(HOUR_BUTTON, INPUT_PULLUP);
  pinMode(MINUTE_BUTTON, INPUT_PULLUP);
  pinMode(ALARM_BUTTON, INPUT_PULLUP);
  pinMode(SNOOZE_BUTTON, INPUT_PULLUP);
  pinMode(COLD_LED_PIN, OUTPUT);
  pinMode(WARM_LED_PIN, OUTPUT);
  //pinMode(BUZZER_PIN, OUTPUT);
  
  display.setBrightness(7);  // Set display brightness (0-7)
}


//-----------------


int checkButtonPress(int pin, ButtonState *button) {
  bool reading = digitalRead(pin);
  int result = 0;
  unsigned long currentTime = millis();

  // Check if reading has changed
  if (reading != button->lastState) {
    button->lastDebounceTime = currentTime;
  }
  
  // Check if state has been stable for debounce time
  if ((currentTime - button->lastDebounceTime) > DEBOUNCE_DELAY) {
    // If state has changed from our debounced state
    if (reading != button->currentState) {
      button->currentState = reading;
      // On button press (state changed to LOW)
      if (button->currentState == LOW) {
        button->pressTime = currentTime;
        Serial.println("Button pressed");
        Serial.println(pin);
      } else {
        // On button release (state changed to HIGH)
        unsigned long pressDuration = currentTime - button->pressTime;
        
        // First check if it was a long press (>= 2 seconds)
        if (pressDuration >= LONG_PRESS_TIME) {
          result = 2; // Long press
          Serial.println("Long Button pressed");

        } 
        // Then check if it was a short press (< 2 seconds)
        else if (pressDuration > DEBOUNCE_DELAY) {
          result = 1; // Short press
          Serial.println("Short Button pressed");

        }
      }
    }
  }

  button->lastState = reading;
  return result;
}


//-----------------


void handleButtonEvents() {
  // Hour button
  int hourPress = checkButtonPress(HOUR_BUTTON, &hourButton);
  if (hourPress == 1) {  // Short press
    if (display_mode == MODE_SET_CLOCK || display_mode == MODE_SET_ALARM) {
      incrementHour();
      display_needs_update = true;
    }
  }
  
  // Minute button
  int minutePress = checkButtonPress(MINUTE_BUTTON, &minuteButton);
  if (minutePress == 1) {  // Short press
    if (display_mode == MODE_SET_CLOCK || display_mode == MODE_SET_ALARM) {
      incrementMinute();
      display_needs_update = true;
    }
  }
  
  // Alarm button
  int alarmPress = checkButtonPress(ALARM_BUTTON, &alarmButton);
  if (alarmPress == 1) {  // Short press
    if (display_mode == MODE_NORMAL) {
      toggleAlarm();
      display_needs_update = true;
    } else if (display_mode == MODE_SET_ALARM) {
      // Save the alarm time
      alarm_target.hour = alarm_hour;
      alarm_target.minute = alarm_minute;
      alarm_state.alarm_time = alarm_target.next(now);
      
      display_mode = MODE_NORMAL;
      Serial.println("Alarm set");
      Serial.print("Alarm time: ");
      Serial.print(alarm_hour);
      Serial.print(":");
      Serial.println(alarm_minute);
      display_needs_update = true;
    }
  } else if (alarmPress == 2) {  // Long press
    if (display_mode == MODE_NORMAL) {
      display_mode = MODE_SET_ALARM;
      Serial.println("Entering alarm set mode");
      display_needs_update = true;
    }
  }
  
  // Snooze button
  int snoozePress = checkButtonPress(SNOOZE_BUTTON, &snoozeButton);
  if (snoozePress == 1) {  // Short press
    if (display_mode == MODE_NORMAL) {
      if (alarm_triggered) {
        snoozeAlarm();
        display_needs_update = true;
      } else {
        showing_alarm_preview = true;
        alarm_preview_end_time = millis() + 2000;  // Show for 2 seconds
        display_needs_update = true;
      }
    } else if (display_mode == MODE_SET_CLOCK) {
      // Set the RTC with current values
      setCurrentTime(current_hour, current_minute, 0);
      
      display_mode = MODE_NORMAL;
      Serial.println("Clock set");
      display_needs_update = true;
    }
  } else if (snoozePress == 2) {  // Long press
    if (display_mode == MODE_NORMAL) {
      display_mode = MODE_SET_CLOCK;
      Serial.println("Entering clock set mode");
      display_needs_update = true;
    }
  }
}

void incrementHour() {
  waitForRelease(HOUR_BUTTON, &hourButton);
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
  waitForRelease(MINUTE_BUTTON, &minuteButton);
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
  waitForRelease(ALARM_BUTTON, &alarmButton);
  alarm_enabled = !alarm_enabled;
  alarm_state.active = alarm_enabled;
  
  if (alarm_triggered) {
    turnOffAlarm();
  }
  Serial.print("Alarm: ");
  Serial.println(alarm_enabled ? "ON" : "OFF");
}

void showAlarmTime() {
  waitForRelease(SNOOZE_BUTTON, &snoozeButton);
  showing_alarm_preview = true;
  alarm_preview_end_time = millis() + 2000;  // Show for 2 seconds
  display_needs_update = true;
}

void handleNormalMode() {
  // Check alarm
  checkAlarm();
  
  // Check if alarm auto-snooze is needed
  if (alarm_triggered && millis() - alarm_trigger_time >= ALARM_DURATION) {
    snoozeAlarm();
  }
  
  // Check if alarm preview should end
  if (showing_alarm_preview && millis() >= alarm_preview_end_time) {
    showing_alarm_preview = false;
    display_needs_update = true;
  }
  
  // Set flag to update display to keep colon blinking
  static unsigned long lastBlinkTime = 0;
  unsigned long currentTime = millis();
  if (!alarm_enabled && currentTime - lastBlinkTime >= 500) {
    lastBlinkTime = currentTime;
    colon_on = !colon_on;
    display_needs_update = true;
  }
}

void handleSetClockMode() {
  static unsigned long lastBlinkTime = 0;
  unsigned long currentTime = millis();

  // Blink the display every 500 ms
  if (currentTime - lastBlinkTime >= 500) {
    lastBlinkTime = currentTime;
    colon_on = !colon_on;
    display_needs_update = true;
  }
}

void handleSetAlarmMode() {
  static unsigned long lastBlinkTime = 0;
  unsigned long currentTime = millis();

  // Blink the display every 500 ms
  if (currentTime - lastBlinkTime >= 500) {
    lastBlinkTime = currentTime;
    colon_on = !colon_on;
    display_needs_update = true;
  }
}

void updateDisplay() {
  // Only update if needed to reduce flickering and save power
  if (!display_needs_update) {
    return;
  }
  
  display_needs_update = false;
  
  if (showing_alarm_preview) {
    // Show alarm time
    int alarmValue = alarm_hour * 100 + alarm_minute;
    display.showNumberDecEx(alarmValue, 0b01000000, true);
    return;
  }
  
  switch (display_mode) {
    case MODE_NORMAL:
      {
        int displayValue = current_hour * 100 + current_minute;
        
        // In normal mode, colon is always on when alarm is enabled, blinks otherwise
        display.showNumberDecEx(displayValue,
                               (alarm_enabled ? 0b01000000 : (colon_on ? 0b01000000 : 0b00000000)), 
                               true);
      }
      break;
      
    case MODE_SET_CLOCK:
    case MODE_SET_ALARM:
      {
        int value = (display_mode == MODE_SET_CLOCK) 
                    ? (current_hour * 100 + current_minute)
                    : (alarm_hour * 100 + alarm_minute);
        display.showNumberDecEx(value, colon_on ? 0b01000000 : 0b00000000, true);
      }
      break;
  }
}

void checkAlarm() {
  if (alarm_enabled && !alarm_triggered) {
    if (current_hour == alarm_hour && current_minute == alarm_minute && current_second < 5) {
      alarm_triggered = true;
      alarm_trigger_time = millis();
      Serial.println("ALARM TRIGGERED!");
      
      // Start buzzer
      //digitalWrite(BUZZER_PIN, HIGH);
      display_needs_update = true;
    }
  }
}

void turnOffAlarm() {
  alarm_triggered = false;
  //digitalWrite(BUZZER_PIN, LOW);
  Serial.println("Alarm turned off");
}

void snoozeAlarm() {
  turnOffAlarm();
  
  // Calculate new alarm time for snooze
  int new_minute = alarm_minute + SNOOZE_TIME;
  int new_hour = alarm_hour;
  
  // Handle minute overflow
  if (new_minute >= 60) {
    new_minute %= 60;
    new_hour = (new_hour + 1) % 24;
  }
  
  alarm_minute = new_minute;
  alarm_hour = new_hour;
  
  // Update alarm target and alarm time
  alarm_target.hour = alarm_hour;
  alarm_target.minute = alarm_minute;
  alarm_state.alarm_time = alarm_target.next(now);
  
  Serial.print("Alarm snoozed for ");
  Serial.print(SNOOZE_TIME);
  Serial.println(" minutes");
  Serial.print("New alarm time: ");
  Serial.print(alarm_hour);
  Serial.print(":");
  Serial.println(alarm_minute);
}

void waitForRelease(int pin, ButtonState *button) {
  // Wait for button to be released
  while (digitalRead(pin) == LOW) {
    delay(10);
  }
  
  // Reset button state
  button->currentState = HIGH;
  button->lastState = HIGH;
}