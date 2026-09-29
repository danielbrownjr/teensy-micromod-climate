#include "BoardPins.h"
#include "Diagnostics.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
namespace {
constexpr uint8_t address=0x71;
const char* names[]={"A","B","UP","DOWN","LEFT","RIGHT","CENTER"};
bool readRegister(uint8_t reg, uint8_t& value) {
  Wire.beginTransmission(address);
  Wire.write(reg); // Register pointer only; no configuration/EEPROM writes.
  if (Wire.endTransmission()!=0) return false;
  if (Wire.requestFrom(address, uint8_t(1))!=1) return false;
  value=Wire.read();
  return true;
}
}
void scanI2c(Diagnostics& d) {
  d.i2cCount=0;
  Serial.println("I2C scan (7-bit addresses, 100 kHz)");
  for (uint8_t a=8;a<0x78;++a) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission()==0) {
      d.addresses[d.i2cCount++]=a;
      Serial.printf("  ACK 0x%02X\n",a);
    }
  }
  uint8_t id=0, minor=0, major=0;
  d.buttonsOnline=readRegister(0,id) && id==0xAE;
  if (d.buttonsOnline && readRegister(1,minor) && readRegister(2,major))
    Serial.printf("BUTTON controller 0x71 ID=0x%02X firmware=%u.%u\n",id,major,minor);
  else Serial.println("BUTTON controller absent or unexpected ID (expected 0xAE at 0x71)");
}
void probeSd(Diagnostics& d) {
  if(d.logging) { Serial.println("SD probe skipped while logging; stop first"); return; }
  // Carrier socket uses SPI, not BUILTIN_SDCARD/SDIO. No card-detect net.
  digitalWrite(Pins::tftCs,HIGH);
  SD.sdfs.end();
  d.sdMounted=SD.begin(Pins::sdCs);
  if (d.sdMounted) {
    File root=SD.open("/",FILE_READ);
    d.sdMounted=bool(root) && root.isDirectory();
    root.close();
  }
  Serial.printf("SD %s (read-only probe; no physical detect signal)\n",
    d.sdMounted?"filesystem/root readable":"unavailable: absent, unformatted, or SPI error");
}
void ioBegin(Diagnostics& d) {
  pinMode(Pins::tftCs,OUTPUT); digitalWrite(Pins::tftCs,HIGH);
  pinMode(Pins::sdCs,OUTPUT); digitalWrite(Pins::sdCs,HIGH);
  pinMode(Pins::backlight,OUTPUT); digitalWrite(Pins::backlight,HIGH);
  pinMode(Pins::buttonInterrupt,INPUT_PULLUP);
  SPI.begin(); Wire.begin(); Wire.setClock(100000);
  scanI2c(d); probeSd(d);
}
uint8_t pollButtons(Diagnostics& d) {
  if (!d.buttonsOnline) return 0;
  uint8_t raw=0;
  if (!readRegister(3,raw)) {
    ++d.buttonErrors; d.buttonsOnline=false; d.pressed=0;
    Serial.println("BUTTON read failed; rescan with i"); return 0;
  }
  raw &= 0x7F; // Bit 7 is event-available, not an eighth button.
  const uint8_t down=raw & ~d.pressed, changed=raw ^ d.pressed;
  for (uint8_t n=0;n<7;++n) if(down & (1<<n)) d.lastButton=n;
  if (changed) {
    ++d.transitions;
    for (uint8_t n=0;n<7;++n) if (changed & (1<<n))
      Serial.printf("BUTTON %s %s\n",names[n],raw&(1<<n)?"DOWN":"UP");
  }
  d.pressed=raw; d.seen |= raw;
  // Reading 0x04 separately clears the latched click register/interrupt.
  uint8_t clicked=0;
  if (!readRegister(4,clicked)) ++d.buttonErrors;
  return down;
}
void printStatus(const Diagnostics& d) {
  Serial.printf("CLIMATE view=%u history=%u units=%s logging=%u rows=%lu file=%s logError=%u alerts=%u alarm=%u\n",
    d.view,historyCount,d.fahrenheit?"F":"C",d.logging,(unsigned long)d.logRows,d.logName,d.logError,d.alerts,d.alarm);
  Serial.printf("STATUS stage=%u uptime_ms=%lu buttons=%s pressed=0x%02X seen=0x%02X transitions=%lu errors=%lu I2C=%u SD=%s TFT_power=0x%02X format=0x%02X\n",
    PLAYGROUND_STAGE,(unsigned long)millis(),d.buttonsOnline?"online":"offline",d.pressed,d.seen,
    (unsigned long)d.transitions,(unsigned long)d.buttonErrors,d.i2cCount,d.sdMounted?"readable":"unavailable",d.tftPower,d.tftFormat);
}
