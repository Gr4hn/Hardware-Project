#ifndef BUTTON_CONTROLER_H
#define BUTTON_CONTROLER_H

class ButtonControler
{
private:
    int pin;
    bool currentState;  // Debounced state - LOW when pressed
    bool lastState;     // Previous raw reading
    unsigned long pressTime;
    unsigned long lastDebounceTime;
public:
    ButtonControler(int button_pin);
};

ButtonControler::ButtonControler(int button_pin) : pin(button_pin), currentState(HIGH), lastState(HIGH), pressTime(0), lastDebounceTime(0)
{
    pinMode(pin, INPUT_PULLUP);
}



#endif
