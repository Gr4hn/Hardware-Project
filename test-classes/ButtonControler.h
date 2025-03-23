#ifndef BUTTON_CONTROLER_H
#define BUTTON_CONTROLER_H
#include "./DisplayControler.h"

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
    ButtonState state;
    static const unsigned int DEBOUNCE_DELAY{ 50 };    // Short debounce delay (ms)... should it be long instead?
    static const unsigned int LONG_PRESS_TIME{ 2000 }; // Long press detection time (ms)...should it be long instead?
public:
    ButtonControler(int button_pin, DisplayControler& display_ref, Mode mode_to_set);
    // function to check button presses
};

ButtonControler::ButtonControler(int button_pin, DisplayControler& display_ref, Mode mode_to_set) : pin(button_pin), display(display_ref), mode(mode_to_set)
{
    pinMode(pin, INPUT_PULLUP);
    state = { HIGH, HIGH, 0, 0 };
}

int ButtonControler::checkButtonPress() {
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



#endif
