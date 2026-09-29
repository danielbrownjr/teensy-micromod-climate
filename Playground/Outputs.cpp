#include "BoardPins.h"
#include "Diagnostics.h"
namespace {bool enabled=true;}
void outputsBegin() {
  pinMode(Pins::ledClock,OUTPUT);pinMode(Pins::ledData,OUTPUT);
  digitalWrite(Pins::ledClock,LOW);digitalWrite(Pins::ledData,LOW);
  pinMode(Pins::buzzer,OUTPUT);digitalWrite(Pins::buzzer,LOW);
}
void climateLeds(const Diagnostics& d) {
  static uint32_t last=0;if(millis()-last<250)return;last=millis();
  const bool rh=d.view==2;
  const float value=rh?d.humidity:d.temperatureC;
  const float low=rh?0:10,high=rh?100:40;
  const float fraction=constrain((value-low)/(high-low),0.0f,1.0f);
  const uint8_t count=d.shtValid?uint8_t(1+fraction*5):0;
  const uint8_t red=d.alarm?100:uint8_t(fraction*80);
  const uint8_t blue=d.alarm?0:uint8_t((1-fraction)*80);
  auto out=[](uint8_t b){shiftOut(Pins::ledData,Pins::ledClock,MSBFIRST,b);};
  for(uint8_t i=0;i<4;++i)out(0);
  for(uint8_t i=0;i<6;++i) {const bool on=enabled&&i<count;out(0xE2);out(on?blue:0);out(on&&!d.alarm?25:0);out(on?red:0);}
  for(uint8_t i=0;i<4;++i)out(0xFF);
}
void toggleLeds() {enabled=!enabled;Serial.printf("APA102 enabled=%u\n",enabled);}
void beep() {tone(Pins::buzzer,880,120);Serial.println("BUZZER 880Hz / 120ms");}
