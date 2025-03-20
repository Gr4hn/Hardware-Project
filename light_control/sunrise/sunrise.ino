#include "Sunrise.h"

Sunrise sunrise;

uint32_t old_time {millis()};
uint32_t current_time {millis()};
uint32_t timer {0};

//fake time state
DateTime now { 2001, 1, 1, 8 };
Alarm_State alarm { true, { 8, 30 }, {2001, 1, 1, 8, 30} };

void setup() 
{
  sunrise.setup();   //can use other pins, like sunrise.setup(10, 11);
}


void loop() 
{
  // sunrise update function: takes the state of the alarm and current time
  sunrise( alarm, now );

  // fake clock function using millis, will be inaccurate
  old_time = current_time;
  current_time = millis();
  timer += current_time - old_time;
  if ( timer > 100 )
  {
    timer = 0;
    now = now + TimeSpan {1};
  }
}