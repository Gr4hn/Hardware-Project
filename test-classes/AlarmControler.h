#ifndef ALARM_CONTROLER_H
#define ALARM_CONTROLER_H

#include <RtcDs1302.h>

class AlarmControler
{
private:
    int alarm_hour{ 0 };     // Default alarm time
    int alarm_minute{ 0 };
    bool alarm_enabled;
    bool alarm_triggered;
    unsigned long alarm_trigger_time{ 0 };
    unsigned long alarm_preview_end_time{ 0 };
    bool showing_alarm_preview{ false };
    const int ALARM_DURATION{ 300000 }; // How long alarm sounds before auto-snooze (5 minutes)
    const int SNOOZE_TIME{ 5 };        // Snooze time in minutes


public:
    AlarmControler();
    void init();
    int get_current_alarm_hour() const { return alarm_hour; }
    int get_current_alarm_minute() const { return alarm_minute; }
    int get_current_alarm_second() const { return alarm_second; }
    void set_current_alarm_hour();
    void set_current_alarm_minute();
    void set_current_alarm_second();

};

AlarmControler::AlarmControler() : alarm_hour(7), alarm_minute(0), alarm_enabled(false), alarm_triggered(false), alarm_trigger_time(0), alarm_preview_end_time(0), showing_alarm_preview(false)
{
}

void AlarmControler::init()
{
}

void AlarmControler::set_current_alarm_hour(int time_value)
{
    current_hour = time_value;
}
void AlarmControler::set_current_alarm_minute(int time_value)
{
    current_minute = time_value;
}
void AlarmControler::set_current_alarm_second(int time_value)
{
    current_second = time_value;
}



#endif
