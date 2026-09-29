#include "BoardPins.h"
#include "Diagnostics.h"
Diagnostics diagnostics;
void setup() {
  Serial.begin(115200);
  const uint32_t start=millis();while(!Serial && millis()-start<2500) {}
  Serial.printf("\nCLIMATE DASHBOARD stage=%u cpu=%lu\n",PLAYGROUND_STAGE,(unsigned long)F_CPU_ACTUAL);
  ioBegin(diagnostics);
  if(PLAYGROUND_STAGE>=2) displayBegin(diagnostics);
  if(PLAYGROUND_STAGE>=3) outputsBegin();
  printStatus(diagnostics);
}
void loop() {
  static uint32_t pollAt=0,screenAt=0,logAt=0;
  const uint32_t now=millis();sensorTick(diagnostics);climateTick(diagnostics);
  if(now-pollAt>=20) {pollAt=now;const uint8_t down=pollButtons(diagnostics);if(down) dashboardButton(diagnostics,down);}
  if(PLAYGROUND_STAGE>=2 && now-screenAt>=250) {screenAt=now;displayUpdate(diagnostics);}
  if(now-logAt>=5000) {logAt=now;printStatus(diagnostics);}
  while(Serial.available()) {
    switch(Serial.read()) {
      case '?': Serial.println("s:status i:scan d:SD u:C/F r:record n/p:next/prev view +/-:up/down c:center v:read last CSV b:beep l:LEDs");break;
      case 's':printStatus(diagnostics);break;
      case 'i':scanI2c(diagnostics);break;
      case 'd':probeSd(diagnostics);break;
      case 'u':dashboardButton(diagnostics,1);break;
      case 'r':dashboardButton(diagnostics,2);break;
      case 'n':dashboardButton(diagnostics,32);break;
      case 'p':dashboardButton(diagnostics,16);break;
      case '+':dashboardButton(diagnostics,4);break;
      case '-':dashboardButton(diagnostics,8);break;
      case 'c':dashboardButton(diagnostics,64);break;
      case 'v':dumpLog(diagnostics);break;
      case 'b':if(PLAYGROUND_STAGE>=3)beep();break;
      case 'l':if(PLAYGROUND_STAGE>=3)toggleLeds();break;
    }
  }
}
