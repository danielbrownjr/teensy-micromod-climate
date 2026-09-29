#include "BoardPins.h"
#include "Diagnostics.h"
#include <ILI9341_t3.h>
#include <math.h>
namespace {
ILI9341_t3 tft(Pins::tftCs,Pins::tftDc,Pins::tftReset);
const uint16_t bg=0x0843, muted=0x94B2, accent=0x4EFD;
float temperature(float c,const Diagnostics& d) {return d.fahrenheit?c*1.8f+32:c;}
void text(int x,int y,uint8_t size,uint16_t color) {tft.setCursor(x,y);tft.setTextSize(size);tft.setTextColor(color);}
void graph(const Diagnostics& d) {
  const bool humidity=d.view==2;
  const uint32_t windows[]={120000,600000,1800000};
  const uint32_t span=windows[d.graphWindow], now=millis();
  float lo=10000,hi=-10000;
  uint16_t count=0;
  for(uint16_t n=0;n<historyCount;++n) {
    const auto& p=history[(historyHead+HISTORY_CAPACITY-historyCount+n)%HISTORY_CAPACITY];
    if(now-p.ms>span) continue;
    const float v=humidity?p.humidity:temperature(p.temperatureC,d);
    lo=fminf(lo,v);hi=fmaxf(hi,v);++count;
  }
  text(12,38,2,ILI9341_WHITE);
  if(d.shtValid) tft.printf("%.1f %s",humidity?d.humidity:temperature(d.temperatureC,d),humidity?"% RH":(d.fahrenheit?"F":"C"));
  else tft.print("Sensor unavailable");
  text(12,62,1,muted); tft.printf("%lu minute window | %u points",(unsigned long)(span/60000),count);
  if(!count) {text(42,112,2,muted);tft.print("Collecting data...");return;}
  const float pad=humidity?1.0f:(d.fahrenheit?0.9f:0.5f);
  lo=floorf(lo-pad);hi=ceilf(hi+pad);
  if(humidity) {lo=fmaxf(0,lo);hi=fminf(100,hi);}
  if(hi<=lo) hi=lo+1;
  const int x0=45,x1=305,y0=83,y1=182;
  for(uint8_t i=0;i<3;++i) {
    const int y=y0+i*(y1-y0)/2;
    tft.drawFastHLine(x0,y,x1-x0,0x2127);
    text(3,y-3,1,muted);tft.printf("%.1f",hi-i*(hi-lo)/2);
  }
  int px=0,py=0;uint32_t previous=0;bool have=false;
  for(uint16_t n=0;n<historyCount;++n) {
    const auto& p=history[(historyHead+HISTORY_CAPACITY-historyCount+n)%HISTORY_CAPACITY];
    const uint32_t age=now-p.ms;if(age>span) continue;
    const float v=humidity?p.humidity:temperature(p.temperatureC,d);
    const int x=x1-int(float(age)*(x1-x0)/span);
    const int y=y1-int((v-lo)*(y1-y0)/(hi-lo));
    if(have && p.ms-previous<=3000) tft.drawLine(px,py,x,y,accent);
    else tft.fillCircle(x,y,2,accent);
    px=x;py=y;previous=p.ms;have=true;
  }
  text(x0,189,1,muted);tft.printf("-%lum",(unsigned long)(span/60000));
  text(287,189,1,muted);tft.print("now");
  text(12,207,1,muted);tft.print("Up/Down: 2 / 10 / 30 minute window");
}
}
void displayBegin(Diagnostics& d) {
  tft.setClock(12000000);tft.begin();tft.setRotation(1);tft.invertDisplay(true);
  digitalWrite(Pins::backlight,LOW);
  d.tftPower=tft.readcommand8(0x0A);d.tftFormat=tft.readcommand8(0x0C);
  for(uint8_t i=0;i<3;++i) d.tftId[i]=tft.readcommand8(0xD3,i+1);
  displayUpdate(d);
}
void displayUpdate(const Diagnostics& d) {
  static uint32_t lastSample=~0u,lastRevision=~0u,lastTime=~0u;
  static bool valid=false,alarm=false;
  if(lastSample==d.shtSamples && lastRevision==d.uiRevision && valid==d.shtValid && alarm==d.alarm && lastTime==millis()/5000) return;
  lastSample=d.shtSamples;lastRevision=d.uiRevision;valid=d.shtValid;alarm=d.alarm;lastTime=millis()/5000;
  tft.fillScreen(bg);
  const char* titles[]={"ROOM CLIMATE","TEMPERATURE","HUMIDITY","DIAGNOSTICS","ALERT SETTINGS"};
  text(12,10,2,accent);tft.print(titles[d.view]);
  text(259,15,1,d.logging?ILI9341_RED:muted);tft.print(d.logging?"REC":"IDLE");
  tft.drawFastHLine(12,30,296,0x2127);
  if(d.view==0) {
    if(d.shtValid) {
      text(12,45,1,muted);tft.print("TEMPERATURE");
      text(12,62,4,ILI9341_WHITE);tft.printf("%.1f %s",temperature(d.temperatureC,d),d.fahrenheit?"F":"C");
      text(12,108,1,muted);tft.print("RELATIVE HUMIDITY");
      text(12,125,4,accent);tft.printf("%.1f%%",d.humidity);
    } else {text(12,70,2,muted);tft.print("Waiting for SHT31");}
    text(12,170,1,d.alarm?ILI9341_RED:muted);
    tft.print(d.alarm?"ALERT: upper threshold exceeded":(d.alerts?"Alerts armed":"Alerts off"));
    text(12,188,1,d.logError?ILI9341_RED:muted);
    if(d.logError) tft.print("SD error - check card, B to retry");
    else if(d.logging) tft.printf("%s | %lu rows",d.logName,(unsigned long)d.logRows);
    else tft.print("B starts a new CSV recording");
    text(12,207,1,muted);tft.printf("%u history points | Left/Right: views",historyCount);
  } else if(d.view==1 || d.view==2) graph(d);
  else if(d.view==3) {
    text(12,43,1,ILI9341_WHITE);tft.printf("SHT31: %s at 0x%02X",d.shtValid?"OK":"OFFLINE",d.shtAddress);
    text(12,63,1,muted);tft.printf("Samples %lu / errors %lu",(unsigned long)d.shtSamples,(unsigned long)d.shtErrors);
    text(12,83,1,muted);tft.printf("Buttons %s / seen %02X / held %02X",d.buttonsOnline?"OK":"OFF",d.seen,d.pressed);
    text(12,103,1,muted);tft.printf("SD %s | TFT %02X/%02X",d.sdMounted?"mounted":"unavailable",d.tftPower,d.tftFormat);
    text(12,123,1,muted);tft.print("I2C: ");for(uint8_t n=0;n<d.i2cCount&&n<10;++n)tft.printf("%02X ",d.addresses[n]);
    text(12,143,1,muted);tft.printf("Log %s / %lu rows",d.logName,(unsigned long)d.logRows);
    text(12,163,1,muted);tft.printf("Uptime %lu s",(unsigned long)(millis()/1000));
    text(12,207,1,muted);tft.print("Center: rescan I2C / check SD");
  } else {
    const uint16_t normal=muted,selected=ILI9341_YELLOW;
    text(12,48,2,d.setting==0?selected:normal);tft.printf("High temp: %.0f C",d.highTempC);
    text(12,85,2,d.setting==1?selected:normal);tft.printf("High RH:   %.0f%%",d.highHumidity);
    text(12,122,2,d.setting==2?selected:normal);tft.printf("Alerts:    %s",d.alerts?"ON":"OFF");
    text(12,164,1,muted);tft.print("Center: select | Up/Down: change");
    text(12,184,1,muted);tft.print("Limits use C. Settings reset at boot.");
    text(12,204,1,muted);tft.print("Buzzer: one short beep every 10s max");
  }
  tft.drawFastHLine(12,220,296,0x2127);
  text(12,228,1,muted);tft.print("<> view   A:C/F   B:record/stop");
}
