#ifndef CHAS_SUNRISE
#define CHAS_SUNRISE

#include <RtcDs1302.h>
#include <Wire.h>
#include <math.h>


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

  const uint32_t time_until_next ( const RtcDateTime& now ) const
  {
    bool tomorrow {false};

    if ( hour < now.Hour() )
    {
      tomorrow = true;
    }
    else if ( hour == now.Hour() && minute <= now.Minute() )
    {
      tomorrow = true;
    }

    RtcDateTime target(now.Year(), now.Month(), now.Day() + (tomorrow ? 1 : 0), hour, minute, 0);

    uint32_t target_seconds = target.TotalSeconds();
    uint32_t now_seconds = now.TotalSeconds();
  }

  const RtcDateTime next ( const RtcDateTime& now ) const
  {
    bool tomorrow {false};

    if ( hour < now.Hour() )
    {
      tomorrow = true;
    }
    else if ( hour == now.Hour() && minute <= now.Minute() )
    {
      tomorrow = true;
    }

    RtcDateTime target ( now.Year(), now.Month(), now.Day() + tomorrow, hour, minute, 0 );

    return target;

  }

};


struct Alarm_State 
{
  bool active;
  Alarm_Target set_time;
  RtcDateTime alarm_time;
};


struct Sunrise 
{
  uint8_t cold_led_pin;
  uint8_t warm_led_pin;

  float brightness_ratio {0.0}; // 0.0 to 1.0
  float warmth_ratio {0.0};     // 0.0 to 1.0
  uint32_t fade_out_start_ms {0};
  bool fading_out {false};

  uint32_t fade_in_duration {20 * 60};  // 20 minutes
  uint32_t steady_duration {20 * 60};   // 20 minutes
  uint32_t fade_out_duration {10};      // Updated: 10 seconds for smoother fade-out

  const uint32_t alarm_duration() const
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

  void operator()(Alarm_State alarm, RtcDateTime now) {
    uint32_t now_seconds = now.TotalSeconds();
    uint32_t alarm_seconds = alarm.alarm_time.TotalSeconds();
    uint32_t fade_in_seconds = fade_in_duration;
    uint32_t steady_seconds = steady_duration;
    uint32_t fade_out_seconds = fade_out_duration;

    if (alarm.active && now_seconds <= alarm_seconds + alarm_duration()) {
      if (now_seconds > alarm_seconds - fade_in_seconds) {
        fading_out = false;
        if (now_seconds < alarm_seconds) {
          brightness_ratio = 1.0 - float(alarm_seconds - now_seconds) / float(fade_in_seconds);
          warmth_ratio = 1.0 - (brightness_ratio * brightness_ratio * brightness_ratio);
        } else if (now_seconds < alarm_seconds + steady_seconds) {
          brightness_ratio = 1.0;
          warmth_ratio = 0.0;
        }
      }
    } else {
      if (brightness_ratio > 0.0 && !fading_out) {
        fading_out = true;
        fade_out_start_ms = millis();
      }

      if (fading_out) {
        uint32_t elapsed_ms = millis() - fade_out_start_ms;
        float fade_duration_ms = fade_out_seconds * 1000.0;

        if (elapsed_ms < fade_duration_ms) {
          brightness_ratio = 1.0 - (float(elapsed_ms) / float(fade_duration_ms));
          warmth_ratio = 1.0 - (brightness_ratio * brightness_ratio * brightness_ratio);
        } else {
          brightness_ratio = 0.0;
          warmth_ratio = 1.0;
          fading_out = false;
        }
      } else {
        brightness_ratio = 0.0;
        warmth_ratio = 1.0;
      }
    }

    set_brightness();
  }
};

#endif