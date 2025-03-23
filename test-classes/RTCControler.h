#ifndef RTC_CONTROLER_H
#define RTC_CONTROLER_H

#include <RtcDs1302.h>
#include <Wire.h>
#include "./DisplayControler.h"

class RTCControler
{
private:
    ThreeWire myWire;
    RtcDs1302<ThreeWire> rtc;
    int current_hour{ 0 }; // Start from 12:00
    int current_minute{ 0 };
    int current_second{ 0 };
    RtcDateTime now;
    unsigned long lastTimeUpdate;
public:
    RTCControler(int data_pin, int clk_pin, int rst_pin);
    void init();
    int get_current_hour() const { return now.Hour(); }
    int get_current_minute() const { return now.Minute(); }
    int get_current_second() const { return now.Second(); }
    unsigned long get_last_time_update() const { return lastTimeUpdate; }
    void set_current_hour(int time_value);
    void set_current_minute(int time_value);
    void set_current_second(int time_value);
    void set_last_time_update(unsigned long time_value);
    void updateCurrentTimeFromRTC(Display& display);
    void updateClock();
    RtcDateTime getDateTime();

};

RTCControler::RTCControler(int data_pin, int clk_pin, int rst_pin) : myWire(data_pin, clk_pin, rst_pin), rtc(myWire), current_hour(12), current_minute(0), current_second(0), lastTimeUpdate(0) {}

void RTCControler::init()
{
    // Initialize the RTC
    rtc.Begin();

    // Check if the RTC is valid
    if (!rtc.GetIsRunning()) {
        Serial.println("RTC is not running! Setting the time.");
        RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
        rtc.SetDateTime(compiled);
    }

    Serial.println("RTC time set");
}

void RTCControler::updateCurrentTimeFromRTC(Display& display) {
    // Get current time from RTC once
    now = rtc.GetDateTime();

    // Store previous values to detect changes
    int prev_hour = current_hour;
    int prev_minute = current_minute;

    // Update current time
    current_hour = now.Hour();
    current_minute = now.Minute();
    current_second = now.Second();

    // If time has changed, update display
    if (prev_hour != current_hour || prev_minute != current_minute) {
        display.set_display_needs_update(true);

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

void RTCControler::updateClock()
{
    if (millis() - lastTimeUpdate > 1000) {
        now = rtc.GetDateTime();
        updateCurrentTimeFromRTC();
        lastTimeUpdate = millis();
    }
}


void RTCControler::set_current_hour(int time_value)
{
    current_hour = time_value;
}
void RTCControler::set_current_minute(int time_value)
{
    current_minute = time_value;
}
void RTCControler::set_current_second(int time_value)
{
    current_second = time_value;
}

void RTCControler::set_last_time_update(unsigned long time_value)
{
    lastTimeUpdate = time_value;
}

RtcDateTime RTCControler::getDateTime()
{
    return now;
}



#endif
