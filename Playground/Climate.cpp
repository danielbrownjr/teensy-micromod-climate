#include "Diagnostics.h"
#include "BoardPins.h"
#include <SD.h>
ClimatePoint history[HISTORY_CAPACITY];
uint16_t historyCount=0, historyHead=0;
namespace {
FsFile logFile;
uint32_t lastSample=0, lastAlarm=0;
void logFailure(Diagnostics& d) {
  logFile.close(); d.logging=false; d.logError=true; d.sdMounted=false;
  ++d.uiRevision; Serial.println("LOG ERROR: stopped; check card and retry");
}
}
void stopLogging(Diagnostics& d) {
  if(!d.logging) return;
  const bool okay=logFile.sync() && !logFile.getWriteError();
  logFile.close(); d.logging=false; d.logError=!okay;
  if(!okay) d.sdMounted=false;
  ++d.uiRevision;
  Serial.printf("LOG stopped %s rows=%lu sync=%s\n",d.logName,(unsigned long)d.logRows,okay?"OK":"FAIL");
}
void toggleLogging(Diagnostics& d) {
  if(d.logging) { stopLogging(d); return; }
  probeSd(d);
  if(!d.sdMounted) { d.logError=true; ++d.uiRevision; return; }
  // Exclusive creation prevents modifying any existing file on the card.
  for(unsigned n=0;n<10000;++n) {
    snprintf(d.logName,sizeof(d.logName),"CLIM%04u.CSV",n);
    if(SD.exists(d.logName)) continue;
    logFile=SD.sdfs.open(d.logName,O_WRONLY|O_CREAT|O_EXCL);
    if(!logFile) { logFailure(d); return; }
    const char* header="uptime_ms,temperature_c,temperature_f,relative_humidity_pct\n";
    if(logFile.write(header)!=strlen(header) || !logFile.sync()) {logFailure(d);return;}
    d.logging=true; d.logError=false; d.logRows=0; ++d.uiRevision;
    Serial.printf("LOG started %s (new file; timestamps are uptime)\n",d.logName);
    return;
  }
  d.logError=true; ++d.uiRevision;
  Serial.println("LOG cannot create: all 10000 session names occupied");
}
void dumpLog(const Diagnostics& d) {
  if(d.logging) {Serial.println("Stop logging before readback");return;}
  if(!d.logName[0]) {Serial.println("No session file this boot");return;}
  FsFile file=SD.sdfs.open(d.logName,O_RDONLY);
  if(!file) {Serial.println("LOG readback failed");return;}
  Serial.printf("LOG READBACK %s (up to 4096 bytes)\n",d.logName);
  for(uint16_t n=0;n<4096 && file.available();++n) Serial.write(file.read());
  file.close(); Serial.println("LOG READBACK END");
}
void dashboardButton(Diagnostics& d,uint8_t mask) {
  if(mask&1) d.fahrenheit=!d.fahrenheit;
  if(mask&2) toggleLogging(d);
  if(mask&16) d.view=(d.view+4)%5;
  if(mask&32) d.view=(d.view+1)%5;
  if(d.view==4) {
    if(mask&64) d.setting=(d.setting+1)%3;
    const int change=(mask&4)?1:((mask&8)?-1:0);
    if(change) {
      if(d.setting==0) d.highTempC=constrain(d.highTempC+change,0.0f,60.0f);
      if(d.setting==1) d.highHumidity=constrain(d.highHumidity+change,10.0f,100.0f);
      if(d.setting==2) d.alerts=change>0;
    }
  } else if(d.view==1 || d.view==2) {
    if(mask&4) d.graphWindow=(d.graphWindow+1)%3;
    if(mask&8) d.graphWindow=(d.graphWindow+2)%3;
  } else if(mask&64) { scanI2c(d); probeSd(d); }
  ++d.uiRevision;
  Serial.printf("UI view=%u units=%s window=%u alerts=%u limits=%.1fC/%.0f%%\n",
    d.view,d.fahrenheit?"F":"C",d.graphWindow,d.alerts,d.highTempC,d.highHumidity);
}
void climateTick(Diagnostics& d) {
  if(d.shtValid && d.shtSamples!=lastSample) {
    lastSample=d.shtSamples;
    history[historyHead]={millis(),d.temperatureC,d.humidity};
    historyHead=(historyHead+1)%HISTORY_CAPACITY;
    if(historyCount<HISTORY_CAPACITY) ++historyCount;
    if(d.logging) {
      char row[100];
      const int len=snprintf(row,sizeof(row),"%lu,%.2f,%.2f,%.2f\n",(unsigned long)millis(),
        d.temperatureC,d.temperatureC*1.8f+32,d.humidity);
      if(len<0 || size_t(len)>=sizeof(row) || logFile.write(row,size_t(len))!=size_t(len)
        || !logFile.sync() || logFile.getWriteError()) logFailure(d);
      else {++d.logRows;Serial.printf("LOG row=%lu file=%s sync=OK\n",(unsigned long)d.logRows,d.logName);}
    }
  }
  // Hysteresis prevents repeated toggles around the upper thresholds.
  if(!d.alerts || !d.shtValid) d.alarm=false;
  else if(d.temperatureC>=d.highTempC || d.humidity>=d.highHumidity) d.alarm=true;
  else if(d.temperatureC<d.highTempC-0.5f && d.humidity<d.highHumidity-2) d.alarm=false;
  if(d.alarm && PLAYGROUND_STAGE>=3 && millis()-lastAlarm>=10000) {lastAlarm=millis();beep();}
  if(PLAYGROUND_STAGE>=3) climateLeds(d);
}
