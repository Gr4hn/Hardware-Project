#include <Ds1302.h>
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
const int RST_PIN = 11;           // Add a reset pin
const int DAT_PIN = 10;           // Add a data pin
const int CLK_PIN = 9;            // Add a clock pin

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
Ds1302 rtc(RST_PIN, DAT_PIN, CLK_PIN);
TM1637Display display(CLK, DIO);

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

// Time and alarm variables
int current_hour = 12; // Start from 12:00
int current_minute = 0;
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
void waitForRelease(int pin, ButtonState *button);
void incrementHour();
void incrementMinute();
void toggleAlarm();
void firstTimeSetup();


uint8_t parseDigits(char* str, uint8_t count)
{
    uint8_t val = 0;
    while(count-- > 0) val = (val * 10) + (*str++ - '0');
    return val;
}

//----------------------------------------------------------------------


void setup() {
  Serial.begin(9600);
  initializeHardware();
  Serial.println("Hardware initialized");
  initializeRTC();
  Serial.println("RTC initialized");
}

//----------------------

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

//----------------------

void initializeHardware() {
  pinMode(HOUR_BUTTON, INPUT_PULLUP);
  pinMode(MINUTE_BUTTON, INPUT_PULLUP);
  pinMode(ALARM_BUTTON, INPUT_PULLUP);
  pinMode(SNOOZE_BUTTON, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  
  display.setBrightness(7);  // Set display brightness (0-7)
}

//----------------------

void initializeRTC() {
  // Initialize the RTC
  rtc.init();
  // Set initial RTC time to 12:00
  Ds1302::DateTime dt = {
    .year = 0,
    .month = 0,
    .day = 0,
    .hour = 12,
    .minute = 0,
    .second = 0,
    .dow = Ds1302::DOW_SUN
  };
  rtc.setDateTime(&dt);
}

//----------------------

bool checkShortButtonPress(int pin, ButtonState *button) {
  if (digitalRead(pin) == LOW) {
    waitForRelease(pin, button);
    return true;
  }
  return false;
}

//----------------------

bool checkButtonLongPress(int pin, ButtonState *button) {
  if (digitalRead(pin) == LOW) {
    waitForRelease(pin, button);
    unsigned long pressDuration = millis() - button->pressTime;
    if (pressDuration >= LONG_PRESS_TIME) {
      return true;
    }
  }
  return false;
}


//----------------------


void handleButtonEvents() {
  // Short press events
  if (checkShortButtonPress(HOUR_BUTTON, &hourButton)) {
    if (display_mode == MODE_SET_CLOCK || display_mode == MODE_SET_ALARM) {
      incrementHour();
      display_needs_update = true;
    }
  }
  
  if (checkShortButtonPress(MINUTE_BUTTON, &minuteButton)) {
    if (display_mode == MODE_SET_CLOCK || display_mode == MODE_SET_ALARM) {
      incrementMinute();
      display_needs_update = true;
    }
  }
  
  if (checkShortButtonPress(ALARM_BUTTON, &alarmButton)) {
    if (display_mode == MODE_NORMAL) {
      toggleAlarm();
      display_needs_update = true;
    } else if (display_mode == MODE_SET_ALARM) {
      display_mode = MODE_NORMAL;
      Serial.println("Alarm set");
      display_needs_update = true;
    }
  }
  
  if (checkShortButtonPress(SNOOZE_BUTTON, &snoozeButton)) {
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
      setCurrentTime();
      display_mode = MODE_NORMAL;
      Serial.println("Clock set");
      display_needs_update = true;
    }
  }
  
  // Long press events
  if (checkButtonLongPress(ALARM_BUTTON, &alarmButton)) {
    if (display_mode == MODE_NORMAL) {
      display_mode = MODE_SET_ALARM;
      Serial.println("Entering alarm set mode");
      display_needs_update = true;
    }
  }
  
  if (checkButtonLongPress(SNOOZE_BUTTON, &snoozeButton)) {
    if (display_mode == MODE_NORMAL) {
      display_mode = MODE_SET_CLOCK;
      Serial.println("Entering clock set mode");
      display_needs_update = true;
    }
  }
}

//----------------------

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

//----------------------

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

//----------------------

void toggleAlarm() {
  waitForRelease(ALARM_BUTTON, &alarmButton);
  alarm_enabled = !alarm_enabled;
  if (alarm_triggered) {
    turnOffAlarm();
  }
  Serial.print("Alarm: ");
  Serial.println(alarm_enabled ? "ON" : "OFF");
}

//----------------------

void showAlarmTime() {
  waitForRelease(SNOOZE_BUTTON, &snoozeButton);
  showing_alarm_preview = true;
  alarm_preview_end_time = millis() + 2000;  // Show for 2 seconds
  display_needs_update = true;
}

//----------------------

void setCurrentTime() {
  // Set RTC time
  Ds1302::DateTime dt;
  rtc.getDateTime(&dt);  // Get current date
  
  // Keep the date, just update the time
  dt.hour = current_hour;
  dt.minute = current_minute;
  dt.second = 0;
  
  rtc.setDateTime(&dt);
  Serial.println("RTC time updated");
}

//----------------------

void handleNormalMode() {
  // Get current time from RTC
  Ds1302::DateTime now;
  rtc.getDateTime(&now);
  
  // Only update if time has changed
  if (current_hour != now.hour || current_minute != now.minute) {
    current_hour = now.hour;
    current_minute = now.minute;
    display_needs_update = true;
  }
  
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
}

//----------------------

void handleSetClockMode() {
  // This mode is primarily handled by button events
  display_mode = MODE_SET_CLOCK;
  
    static unsigned long lastBlinkTime = 0;
    unsigned long currentTime = millis();

      static char buffer[13];
      static uint8_t char_idx = 0;

      if (char_idx == 13)
      {
        while (MODE_SET_CLOCK) {
          // structure to manage date-time
          Ds1302::DateTime dt;

          dt.year = parseDigits(buffer, 2);
          dt.month = parseDigits(buffer + 2, 2);
          dt.day = parseDigits(buffer + 4, 2);
          dt.dow = parseDigits(buffer + 6, 1);
          dt.hour = parseDigits(buffer + 7, 2);
          dt.minute = parseDigits(buffer + 9, 2);
          dt.second = parseDigits(buffer + 11, 2);

          // set the date and time
          rtc.setDateTime(&dt);

          char_idx = 0;
        }
      }
      if (Serial.available())
      {
          buffer[char_idx++] = Serial.read();
      }

    // Blink the display every 500 ms
    if (currentTime - lastBlinkTime >= 500) {
      lastBlinkTime = currentTime;
      colon_on = !colon_on;
      display_needs_update = true;
    }
    
    if (SNOOZE_BUTTON == LOW) {
      display_mode = MODE_NORMAL;
    }
  
}

//----------------------

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

//----------------------

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
        
        // Set flag to update display on next cycle to keep colon blinking
        static unsigned long lastBlinkTime = 0;
        unsigned long currentTime = millis();
        if (!alarm_enabled && currentTime - lastBlinkTime >= 500) {
          lastBlinkTime = currentTime;
          colon_on = !colon_on;
          display_needs_update = true;
        }
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

//----------------------

void checkAlarm() {
  if (alarm_enabled && !alarm_triggered) {
    Ds1302::DateTime now;
    rtc.getDateTime(&now);
    
    if (now.hour == alarm_hour && now.minute == alarm_minute && now.second < 5) {
      alarm_triggered = true;
      alarm_trigger_time = millis();
      Serial.println("ALARM TRIGGERED!");
      
      // Start buzzer
      digitalWrite(BUZZER_PIN, HIGH);
      display_needs_update = true;
    }
  }
}

//----------------------

void turnOffAlarm() {
  alarm_triggered = false;
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("Alarm turned off");
}

//----------------------

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
  
  Serial.print("Alarm snoozed for ");
  Serial.print(SNOOZE_TIME);
  Serial.println(" minutes");
  Serial.print("New alarm time: ");
  Serial.print(alarm_hour);
  Serial.print(":");
  Serial.println(alarm_minute);
}

//----------------------

void waitForRelease(int pin, ButtonState *button) {
  // Wait for button to be released
  while (digitalRead(pin) == LOW) {
    delay(10);
  }
  
  // Reset button state
  button->currentState = HIGH;
  button->lastState = HIGH;
}

//----------------------