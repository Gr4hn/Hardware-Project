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
  uint32_t fade_out_start_ms {0};
  bool fading_out {false};

  TimeSpan fade_in_duration {20 * 60};  // 20 minutes
  TimeSpan steady_duration {20 * 60};   // 20 minutes
  TimeSpan fade_out_duration {10};      // Updated: 10 seconds for smoother fade-out

  const TimeSpan alarm_duration() const
  {
    return fade_in_duration + steady_duration;
  }

  void setup(uint8_t cold = 3, uint8_t warm = 5)
  {   
    cold_led_pin = cold;
    warm_led_pin = warm;
    pinMode(cold, OUTPUT);
    pinMode(warm, OUTPUT);
  }


  /// Brief: Sets the PWM duty cycle of the LEDs based on our internal logic state.
  /// The cold LED should have maximum duty cycle when warmth ratio is at low.
  void set_brightness()
  {
    analogWrite(cold_led_pin, LED_control::duty_from_ratio(brightness_ratio * (1.0 - warmth_ratio)));
    analogWrite(warm_led_pin, LED_control::duty_from_ratio(brightness_ratio * warmth_ratio));
  }


  /// Brief: Called in the loop function of the arduino. Handles logic for setting the light level.
  /// Based on the set alarm time and the current time, we either fade in, keep steady or fade out.

  void operator()( Alarm_State alarm, DateTime now )
  {
    if (alarm.active && now <= alarm.alarm_time + alarm_duration()) 
    // Alarm is on and within duration
    {
      if (now > alarm.alarm_time - fade_in_duration) 
      // Within fade-in or steady phase
      {
        fading_out = false; // Reset fade-out state
        if (now < alarm.alarm_time) // Fade-in phase
        {
          brightness_ratio = 
            1.0 - float((alarm.alarm_time - now).totalseconds()) / float(fade_in_duration.totalseconds());
          warmth_ratio = 1.0 - ( brightness_ratio * brightness_ratio * brightness_ratio ); // Cubic warmth increase
        }
        else if (now < alarm.alarm_time + steady_duration) // Steady phase
        {
          brightness_ratio = 1.0;
          warmth_ratio = 0.0;
        }
      }
    }
    else 
    // Alarm is off or past duration; handle fade-out
    {
      if (brightness_ratio > 0.0 && !fading_out) 
      // Light is on, start fade-out state
      {
        fading_out = true;
        fade_out_start_ms = millis(); 
      }

      if ( fading_out ) 
      // Perform fade-out
      {
        uint32_t elapsed_ms = millis() - fade_out_start_ms; // Time since fade-out started
        float fade_duration_ms = fade_out_duration.totalseconds() * 1000.0; // Convert to milliseconds
        
        if ( elapsed_ms < fade_duration_ms )
        // Still fading out
        {
          brightness_ratio = 1.0 - ( float( elapsed_ms ) / float( fade_duration_ms ) );
          warmth_ratio = 1.0 - ( brightness_ratio * brightness_ratio * brightness_ratio );
        }
        else 
        // Fade-out complete, reset
        {
          brightness_ratio = 0.0;
          warmth_ratio = 1.0;
          fading_out = false;
        }
      }
      else // Ensure off state if not fading
      {
        brightness_ratio = 0.0;
        warmth_ratio = 1.0;
      }
    }

    set_brightness(); 
  }
};


#endif