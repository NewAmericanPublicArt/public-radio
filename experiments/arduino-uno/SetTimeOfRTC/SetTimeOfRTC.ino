#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtc;

void setup() {
  Serial.begin(9600);
  rtc.begin();
  rtc.adjust(DateTime(2018, 10, 26, 16, 6, 0));
}

void loop() {
  DateTime now;
  now = rtc.now();
  Serial.print(now.unixtime()); // Epoch AKA seconds since 1/1/1970
  Serial.print(" ");
  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
  Serial.println("");
}
