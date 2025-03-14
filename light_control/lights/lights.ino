struct Sunrise 
{

  bool playing {false};

  uint8_t led_pwm_pin {0};
  uint32_t start_play_time_millis { 0 };
  uint32_t fadein_duration_millis { 20000 };   // default 20 seconds
  uint32_t fadeout_duration_millis { 2000 };  // default 2 seconds
  float gamma_factor {2.2};

  int gamma_corrected_pwm ( float ratio, float gamma = gamma_factor )
  {
    if ( ratio > 1 || ratio < 0 )
    {
      return -1;
    }
    else
    {
      return int( 255.0 * ( pow( ratio, gamma ) ) );
    }
  }


  void setup ( uint8_t led_pin_config )
  {
    led_pwm_pin = led_pin_config;
    pinMode(led_pwm_pin, OUTPUT);
  }


  void play ( )
  {
    start_play_time_millis = millis();
    time_for_max_brightness_millis = start_play_time_millis + alarm_duration_millis;
    playing = true;
  }


  void snooze ( )
  {

  }


  void operator()()
  {
    if ( playing ) 
    {
      float progress_ratio { millis()  };
    }

  }

}

Sunrise sunrise;

uint32_t alarm_time { 0 };
bool alarm_active { true };


void setup() {
  // put your setup code here, to run once:
  alarm_time = millis() + ;
  sunrise.setup(3);
}

void loop() {
  // put your main code here, to run repeatedly:
  while ( alarm_active )
  {
    sunrise();
  }

}
