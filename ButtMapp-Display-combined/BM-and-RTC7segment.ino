#include <RTClib.h>
#include <Wire.h>
#include <TM1637Display.h>

#define CLK 2  // TM1637 Clock pin
#define DIO 3  // TM1637 Data pin

RTC_DS1307 rtc;
TM1637Display display(CLK, DIO);

int hourButton = 4;
int minuteButton = 5;
int alarmButton = 6;
int snoozeButton = 7;

bool alarmStatus = false;
int currentAlarm = 0;
int currentMinute = 0;
int currentHour = 0;
int alarmHour = 0;
int alarmMinute = 0;
int pressedAlarm = 0;
int pressedSnooze = 0;
int mode = 0; // 0: Normal, 1: Set Clock, 2: Set Alarm
unsigned long previousMillis = 0;
unsigned long lastTimeUpdate = 0;

void setup() {
  Serial.begin(9600);
  pinMode(hourButton, INPUT);
  pinMode(minuteButton, INPUT);
  pinMode(alarmButton, INPUT);
  pinMode(snoozeButton, INPUT);

  delay(3000);
  Serial.println("RTC is starting");

  Wire.begin();
  rtc.begin();

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC!");
    while (1);
  } else {
    Serial.println("Found RTC");
  }

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    Serial.println("Setting the time from compile time...");
    // Sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("Time set!");
  }

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  display.setBrightness(7);  // Set display brightness (0-7)
}

void setClock() {
  Serial.println("Setting Clock...");
  while (mode == 1) {
    if (digitalRead(hourButton) == HIGH) {
      delay(200); // Debounce delay
      currentHour = (currentHour + 1) % 24;
    }
    else if (digitalRead(minuteButton) == HIGH) {
      delay(200); // Debounce delay
      currentMinute = (currentMinute + 1) % 60;
    }
    else if (digitalRead(snoozeButton) == HIGH) {
      mode = 0;
    }
    Serial.print("Current Hour: ");
    Serial.println(currentHour);
    Serial.print("Current Minute: ");
    Serial.println(currentMinute);
    delay(1000);
  }
}

void setAlarm() {
  Serial.println("Setting Alarm...");
  while (mode == 2) {
    if (digitalRead(hourButton) == HIGH) {
      delay(200); // Debounce delay
      alarmHour = (alarmHour + 1) % 24;
    }
    else if (digitalRead(minuteButton) == HIGH) {
      delay(200); // Debounce delay
      alarmMinute = (alarmMinute + 1) % 60;
    }
    else if (digitalRead(alarmButton) == HIGH) {
      mode = 0;
    }
    int displayValue = alarmHour * 100 + alarmMinute; 
    Serial.print("Alarm Hour: ");
    Serial.println(alarmHour);
    Serial.print("Alarm Minute: ");
    Serial.println(alarmMinute);
    display.showNumberDecEx(displayValue, 0b01000000, true);
    delay(500);
    display.showNumberDecEx(displayValue, 0b00000000, true);
    delay(500);
  }
}

void alarmPress() {
  if (alarmStatus == false) {
    alarmStatus = true;
    Serial.println("Alarm ON");
  } else {
    alarmStatus = false;
    Serial.println("Alarm OFF");
  }
}

void snoozePress() {
  if (alarmStatus == false) {
    return;
  } else {
    alarmMinute = (alarmMinute + 5) % 60;
    // If snooze crosses hour boundary
    if (alarmMinute < 5) {
      alarmHour = (alarmHour + 1) % 24;
    }
    Serial.println("Snooze pressed, adding 5 minutes to alarm...");
    Serial.print("New alarm time: ");
    Serial.print(alarmHour);
    Serial.print(":");
    Serial.println(alarmMinute);
  }
}

void checkAlarm(DateTime now) {
  if (alarmStatus == true && now.hour() == alarmHour && now.minute() == alarmMinute) {
    Serial.println("!!! ALARM TRIGGERED !!!");
    // Here you would add code to trigger your alarm sound/light
  }
}

void updateTime() {
  unsigned long currentMillis = millis();
  // Update time every 60 seconds (60000 milliseconds)
  if (currentMillis - lastTimeUpdate >= 60000) {
    lastTimeUpdate = currentMillis;
    currentMinute = (currentMinute + 1) % 60;
    if (currentMinute == 0) {
      currentHour = (currentHour + 1) % 24;
    }
    Serial.print("Time: ");
    Serial.print(currentHour);
    Serial.print(":");
    if (currentMinute < 10) {
      Serial.print("0");
    }
    Serial.println(currentMinute);
  }
}

void checkButtonHold(int buttonPin, unsigned long &buttonPressStartTime, bool &buttonPressed) {
  bool buttonState = digitalRead(buttonPin);
  
  if (buttonState == HIGH && buttonPressed == false) { 
    Serial.println("Button pressed");
    // Button is pressed down
    buttonPressed = true;
    buttonPressStartTime = millis(); // Start measuring when button is pressed
  } else if (buttonState == LOW && buttonPressed == true) {
    // Button is released
    Serial.println("Button released");
    buttonPressed = false;
    if (millis() - buttonPressStartTime >= 2000) { // If button held for more than 2 seconds
      if (buttonPin == snoozeButton) {
        mode = 1;
        setClock();
      }
      else if (buttonPin == alarmButton) {
        mode = 2;
        setAlarm();
      }
      Serial.println("Button held for 2 seconds, mode changed");
    } else {
      if (buttonPin == alarmButton) {
        alarmPress();
      }
      else if (buttonPin == snoozeButton) {
        snoozePress();
      }
    }
  }
}

void loop() {
  DateTime now = rtc.now();

  // Print the current time to the serial monitor
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  // Display the current time on the 7-segment display in HH:MM format
  int displayValue = now.hour() * 100 + now.minute();

  if (alarmStatus == true) {
    display.showNumberDecEx(displayValue, 0b01000000, true);  // Show HH:MM with colon
    delay(1000);
  }
  else if (alarmStatus == false) {
    display.showNumberDecEx(displayValue, 0b01000000, true);
    delay(500);
    display.showNumberDecEx(displayValue, 0b00000000, true);
    delay(500);
  }

    static unsigned long hourButtonPressStartTime = 0;
  static bool hourButtonPressed = false;
  static unsigned long minuteButtonPressStartTime = 0;
  static bool minuteButtonPressed = false;
  static unsigned long alarmButtonPressStartTime = 0;
  static bool alarmButtonPressed = false;
  static unsigned long snoozeButtonPressStartTime = 0;
  static bool snoozeButtonPressed = false;

  // Check all buttons
  checkButtonHold(hourButton, hourButtonPressStartTime, hourButtonPressed);
  checkButtonHold(minuteButton, minuteButtonPressStartTime, minuteButtonPressed);
  checkButtonHold(alarmButton, alarmButtonPressStartTime, alarmButtonPressed);
  checkButtonHold(snoozeButton, snoozeButtonPressStartTime, snoozeButtonPressed);
  
  // Update time when in normal mode
  if (mode == 0) {
    updateTime();
    checkAlarm(now);
  }

  // Use a shorter delay to ensure button responsiveness
  delay(50);
}

