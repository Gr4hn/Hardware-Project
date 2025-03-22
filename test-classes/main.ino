#include "./RTCControler.h"
#include "./DisplayControler.h"
#include "./ButtonControler.h"
#include "./AlarmControler.h"

// Pin definitions
const int CLK = 2;                // TM1637 Clock pin
const int DIO = 3;                // TM1637 Data pin
const int HOURPLUS_BUTTON = 4;
const int MINUTEPLUS_BUTTON = 5;
const int ALARM_BUTTON = 6;
const int SNOOZE_BUTTON = 7;
const int BUZZER_PIN = 8;         // Add a buzzer pin
const int RST_PIN = 9;            // Add a reset pin
const int DAT_PIN = 10;           // Add a data pin
const int CLK_PIN = 11;           // Add a clock pin
const int MINUTEMINUS_BUTTON = 13;
const int HOURMINUS_BUTTON = 12;

// Time constants
const int DEBOUNCE_DELAY = 50;    // Short debounce delay (ms)
const int LONG_PRESS_TIME = 2000; // Long press detection time (ms)
const int SNOOZE_TIME = 5;        // Snooze time in minutes
const int ALARM_DURATION = 300000; // How long alarm sounds before auto-snooze (5 minutes)
const int DISPLAY_UPDATE_INTERVAL = 500; // Display refresh rate (ms)

// Display modes
const int MODE_NORMAL = 0;
const int MODE_SET_CLOCK = 1;
const int MODE_SET_ALARM = 2;

// setup up all global objects
RTCControler rtc(DAT_PIN, CLK_PIN, RST_PIN);
DisplayControler display(CLK, DIO);
ButtonControler hour_plus_button(HOURPLUS_BUTTON);
ButtonControler hour_minus_button(HOURMINUS_BUTTON);
ButtonControler minute_plus_button(MINUTEPLUS_BUTTON);
ButtonControler minute_minus_button(MINUTEMINUS_BUTTON);
ButtonControler alarm_button(ALARM_BUTTON);
ButtonControler snooze_button(SNOOZE_BUTTON);
AlarmControler alarm(BUZZER_PIN);

void setup() {
    /*
    setup flow:
    1. init rtc
    2. init display
    3. immediately update time from RTC
    4. Force the display to update
    */
    Serial.begin(9600);

    rtc.init();
    display.init();

    /*
    note: tried to preserve the original flow from the void setup(), the updateCurrentTimeFromRTC() sets the display_needs_update to true, then it is set to true right after.
    */
    updateCurrentTimeFromRTC(display);

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

    // First, update the current time from RTC at regular intervals
    //
    if (millis() - rtc.get_last_time_update() >= 1000) {
        rtc.updateCurrentTimeFromRTC(display);
        rtc.set_last_time_update(millis());
    }

    // Now handle the appropriate mode
    switch (display_mode) {
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
