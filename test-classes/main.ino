#include "./RTCControler.h"
#include "./DisplayControler.h"
#include "./ButtonControler.h"
#include "./AlarmControler.h"

RTCControler rtc(10, 11, 9); // dat_pin, clk_pin, rst_pin
DisplayControler display(2, 3, rtc);
ButtonControler hour_plus_button(4);
ButtonControler hour_minus_button(12);
ButtonControler minute_plus_button(5);
ButtonControler minute_minus_button(13);
ButtonControler alarm_button(6);
ButtonControler snooze_button(7);
AlarmControler alarm;

void setup() {
    Serial.begin(9600);

    rtc.init();
    display.init();

    rtc.updateCurrentTimeFromRTC(display);

    display.set_display_needs_update(true);
}

void loop() {

    /*
    loop flow(the whole program flow):
    1. check every second and update the RTC time
    2. default/normal mode:
        - time keeps on updating
        - display keeps on displaying current time
        - alarm time and state in the background
        - the current mode of the system in the background
    3. Buttons change the modes
    4. display is updated based on the current mode
    5. program loops back, continues to check mode
    */

    rtc.updateClock();

    display.handleMode();

    // Then check for button presses after we've handled the current mode
    // moved handleButtonEvents() into the ButtonControler class
    // the checkButtonPress calls the handleButtonPress
    hour_plus_button.checkButtonPress();
    minute_plus_button.checkButtonPress();
    hour_minus_button.checkButtonPress();
    minute_plus_button.checkButtonPress();
    alarm_button.checkButtonPress();
    snooze_button.checkButtonPress();


    // Finally, update the display
    display.updateDisplay();
}
