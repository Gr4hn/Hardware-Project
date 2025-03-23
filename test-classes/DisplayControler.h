#ifndef DISPLAY_CONTROLER_H
#define DISPLAY_CONTROLER_H
#include <TM1637Display.h>
#include "./AlarmControler.h"
#include "./RTCControler.h"

class DisplayControler
{
private:
    TM1637 display;
    unsigned long last_display_update{ 0 };
    int display_mode;
    bool colon_on{ true };
    bool display_needs_update{ true };
    bool showing_alarm_preview{ false };
    unsigned long alarm_preview_end_time{ 0 };
    unsigned long lastTimeUpdate{ 0 };
    const int DISPLAY_UPDATE_INTERVAL{ 500 }; // Display refresh rate (ms)
public:
    DisplayControler(int clk_pin, int dio_pin);
    void init();
    void updateDisplay();
    void set_display_needs_update(bool update);
};


DisplayControler::DisplayControler(int clk_pin, int dio_pin) : display(clk, dio)
{
}

void DisplayControler::init()
{
    display.setBrightness(7);
}

void DisplayControler::updateDisplay() {
    // Only update if needed to reduce flickering and save power
    if (!display_needs_update) {
        return;
    }

    display_needs_update = false;

    if (showing_alarm_preview == true) {
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

void DisplayControler::set_display_needs_update(bool update)
{
    display_needs_update = update;
}



#endif
