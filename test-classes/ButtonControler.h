#ifndef BUTTON_CONTROLER_H
#define BUTTON_CONTROLER_H
#include "./DisplayControler.h"
#include "./RTCControler.h"
#include "./AlarmControler.h"

// Button state tracking - Note: Using INPUT_PULLUP, so LOW is pressed, HIGH is released
struct ButtonState
{
    bool currentState;  // Debounced state - LOW when pressed
    bool lastState;     // Previous raw reading
    unsigned long pressTime;
    unsigned long lastDebounceTime;
};

class ButtonControler
{
private:
    int pin;
    DisplayControler& display;
    Mode mode;
    RTCControler& rtc;
    AlarmControler& alarm;
    ButtonState state;
    static const unsigned int DEBOUNCE_DELAY{ 50 };    // Short debounce delay (ms)... should it be long instead?
    static const unsigned int LONG_PRESS_TIME{ 2000 }; // Long press detection time (ms)...should it be long instead?
public:
    ButtonControler(int button_pin, DisplayControler& display_ref, Mode mode_to_set);
    // function to check button presses
    void checkButtonPress();
    void handleButtonEvents();
};

ButtonControler::ButtonControler(int button_pin, DisplayControler& display_ref, Mode mode_to_set) : pin(button_pin), display(display_ref), mode(mode_to_set)
{
    pinMode(pin, INPUT_PULLUP);
    state = { HIGH, HIGH, 0, 0 };
}

void ButtonControler::checkButtonPress() {
    bool reading = digitalRead(pin);
    int result = 0;
    unsigned long currentTime = millis();

    // Check if reading has changed
    if (reading != state.) {
        state.lastDebounceTime = currentTime;
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
            }
            else {
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

void ButtonControler::handleButtonEvents() {
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
        }
        else if (display_mode == MODE_SET_ALARM) {
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
    }
    else if (alarmPress == 2) {  // Long press
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
            }
            else {
                showing_alarm_preview = true;
                alarm_preview_end_time = millis() + 2000;  // Show for 2 seconds
                display_needs_update = true;
            }
        }
        else if (display_mode == MODE_SET_CLOCK) {
            // Set the RTC with current values
            setCurrentTime(current_hour, current_minute, 0);

            display_mode = MODE_NORMAL;
            Serial.println("Clock set");
            display_needs_update = true;
        }
    }
    else if (snoozePress == 2) {  // Long press
        if (display_mode == MODE_NORMAL) {
            display_mode = MODE_SET_CLOCK;
            Serial.println("Entering clock set mode");
            display_needs_update = true;
        }
    }
}



#endif
