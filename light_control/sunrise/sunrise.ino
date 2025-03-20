#include "Sunrise.h"

Sunrise sunrise;

uint32_t old_time {millis()};
uint32_t current_time {millis()};
uint32_t timer {0};


DateTime now { 2001, 1, 1, 8 };
Alarm_State alarm { true, { 8, 30 }, {2001, 1, 1, 8, 30} };

void setup() 
{
  sunrise.setup();   //sunrise.setup(10, 11);
}


void loop() 
{
  sunrise( alarm, now );

  old_time = current_time;
  current_time = millis();
  timer += current_time - old_time;
  if ( timer > 1000 )
  {
    timer = 0;
    now = now + TimeSpan {1};
  }
}