
int hourButton = 2;
int minuteButton = 3;
int alarmButton = 4;
int snoozeButton = 5;

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
    else if (digitalRead(snoozeButton) == HIGH) {
      mode = 0;
    }
    Serial.print("Alarm Hour: ");
    Serial.println(alarmHour);
    Serial.print("Alarm Minute: ");
    Serial.println(alarmMinute);
    delay(1000);
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

void checkAlarm() {
  if (alarmStatus && currentHour == alarmHour && currentMinute == alarmMinute) {
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
    checkAlarm();
  }

  // Use a shorter delay to ensure button responsiveness
  delay(50);
}