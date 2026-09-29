#include <Arduino.h>
#if !defined(ARDUINO_TEENSY_MICROMOD)
#error Select Teensy MicroMod
#endif
void setup() { Serial.begin(115200); }
void loop() {
 static uint32_t last=0;
 if (millis()-last >= 1000) { last=millis(); Serial.printf("BRINGUP Teensy MicroMod cpu=%lu uptime_ms=%lu\n", (unsigned long)F_CPU_ACTUAL,(unsigned long)millis()); }
 while (Serial.available()) { int c=Serial.read(); if(c=='?') Serial.println("BRINGUP serial RX/TX OK"); }
}
