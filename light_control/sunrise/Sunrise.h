#ifndef CHAS_SUNRISE
#define CHAS_SUNRISE

#include <RTClib.h>


namespace LED_control 
{


  float ratio_from_analog ( int analog_input )
  {
    constrain(analog_input, 0, 1023);

    if ( analog_input == 0 ) 
    {
      return 0.0;
    }
    else
    {
      return float(analog_input) / 1023.0;
    }
  }


  /// Given: A ratio 0.0 to 1.0, and a gamma value
  /// Gives: Gamma corrected brightness curve for LED

  float gamma_corrected_ratio ( float ratio, float gamma = 2.2 )
  {
    constrain(ratio, 0.0, 1.0);
    return pow ( ratio, gamma );
  }


  int duty_from_a ( int analog_input, float gamma = 2.2 )
  {    
    constrain(analog_input, 0, 1023);
    return int(255.0 * gamma_corrected_ratio ( ratio_from_analog( analog_input ) ) );
  }


  int duty_from_ratio ( float ratio, float gamma = 2.2 )
  {
    constrain(ratio, 0.0, 1.0);
    return int(255.0 * gamma_corrected_ratio ( ratio ) );
  }

}

struct Alarm_Target {

  uint8_t hour; // 0-23
  uint8_t minute; // 0-59

  const TimeSpan time_until_next ( const DateTime now ) const
  {
    bool tomorrow {false};

    if ( hour < now.hour() )
    {
      tomorrow = true;
    }
    else if ( hour == now.hour() && minute <= now.minute() )
    {
      tomorrow = true;
    }

    DateTime target { now.year(), now.month(), now.day() + tomorrow, hour, minute };

    return target - now;
  }

  const DateTime next ( const DateTime now ) const
  {
    bool tomorrow {false};

    if ( hour < now.hour() )
    {
      tomorrow = true;
    }
    else if ( hour == now.hour() && minute <= now.minute() )
    {
      tomorrow = true;
    }

    DateTime target { now.year(), now.month(), now.day() + tomorrow, hour, minute };

    return target;

  }

};


struct Alarm_State 
{
  bool active;
  Alarm_Target set_time;
  DateTime alarm_time;
};


struct Sunrise 
{
  uint8_t cold_led_pin;
  uint8_t warm_led_pin;

  float brightness_ratio {0.0}; // 0.0 to 1.0
  float warmth_ratio {0.0};     // 0.0 to 1.0


  TimeSpan fade_in_duration { 20*60 };
  TimeSpan steady_duration { 20*60 };
  TimeSpan fade_out_duration { 2 };


  const TimeSpan alarm_duration (  ) const
  {
    return fade_in_duration + steady_duration;
  }


  void setup ( uint8_t cold = 3 , uint8_t warm = 5 )
  {
    
    cold_led_pin = cold;
    warm_led_pin = warm;
    pinMode( cold, OUTPUT );
    pinMode( warm, OUTPUT );
  }


  void set_brightness ()
  {
    analogWrite(cold_led_pin, LED_control::duty_from_ratio( brightness_ratio * warmth_ratio ) );
    analogWrite(warm_led_pin, LED_control::duty_from_ratio( brightness_ratio * ( 1.0 - warmth_ratio ) ) );
  }


  void operator() ( Alarm_State alarm, DateTime now )
  {
    if ( alarm.active && now <= alarm.alarm_time + alarm_duration() ) // alarm is on and we are not past playback
     {
      if ( now > alarm.alarm_time - fade_in_duration ) // we are in a time span when the light should be on
      {
        if ( now < alarm.alarm_time ) // the set alarm time hasn't happened yet, we are fading in
        {
          brightness_ratio = 
          1.0 - 
            float((alarm.alarm_time - now).totalseconds()) / 
            float(fade_in_duration.totalseconds());
          warmth_ratio = brightness_ratio;
        }
        else if ( now < alarm.alarm_time + steady_duration ) // we haven't passed the active light time
        {
          brightness_ratio = 1.0;
          warmth_ratio = 1.0;
        }
      }
     }
    else //we should fade out towards 0.0 brightness;
    {
      brightness_ratio = 0.0;
    }

    set_brightness();
  }
};


#endif