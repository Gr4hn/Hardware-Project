#include <RTClib.h>
#include <Wire.h>
#include <TM1637Display.h>

#define CLK 2  // TM1637 Clock pin
#define DIO 3  // TM1637 Data pin

RTC_DS1307 rtc;
TM1637Display display(CLK, DIO);

void setup() {
  Serial.begin(9600);
  delay(3000);
  Serial.println("RTC is starting");

  Wire.begin();
  rtc.begin();

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC!");
    while (1);
  } else {
    Serial.println("Found RTC");
  }

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    Serial.println("Setting the time from compile time...");
    // Sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("Time set!");
  }

  display.setBrightness(7);  // Set display brightness (0-7)
}

void loop() {
  DateTime now = rtc.now();

  // Print the current time to the serial monitor
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  // Display the current time on the 7-segment display in HH:MM format
  int displayValue = now.hour() * 100 + now.minute();
  display.showNumberDecEx(displayValue, 0b01000000, true);  // Show HH:MM with colon

  delay(1000);  // Wait for 1 second before updating the display
}