#include "Diagnostics.h"
#include <Wire.h>
namespace {
uint8_t crc(const uint8_t* bytes) {
  uint8_t value=0xFF;
  for(uint8_t i=0;i<2;++i) {
    value ^= bytes[i];
    for(uint8_t b=0;b<8;++b) value=(value&0x80)?uint8_t((value<<1)^0x31):uint8_t(value<<1);
  }
  return value;
}
bool command(uint8_t address) {
  Wire.beginTransmission(address);
  Wire.write(0x24); Wire.write(0x00); // High repeatability, no clock stretching.
  return Wire.endTransmission()==0;
}
}
void sensorTick(Diagnostics& d) {
  static uint32_t started=0;
  static bool waiting=false;
  const uint32_t now=millis();
  if(!waiting) {
    if(now-started<2000) return;
    started=now;
    // User-connected SHT31: test its two documented addresses. ACK alone
    // is not a valid measurement; require both CRCs before showing data.
    if(!d.shtAddress) {
      if(command(0x44)) d.shtAddress=0x44;
      else if(command(0x45)) d.shtAddress=0x45;
      else { d.shtValid=false; ++d.shtErrors; return; }
    } else if(!command(d.shtAddress)) {
      d.shtValid=false; d.shtAddress=0; ++d.shtErrors;
      Serial.println("SHT31 disconnected/command failed; retrying"); return;
    }
    waiting=true;
    return;
  }
  if(now-started<20) return; // Datasheet max conversion 15 ms; no blocking delay.
  waiting=false;
  uint8_t data[6];
  if(Wire.requestFrom(d.shtAddress,uint8_t(6))!=6) {
    while(Wire.available()) Wire.read();
    d.shtValid=false; d.shtAddress=0; ++d.shtErrors;
    Serial.println("SHT31 short read; retrying"); return;
  }
  for(uint8_t i=0;i<6;++i) data[i]=Wire.read();
  if(crc(data)!=data[2] || crc(data+3)!=data[5]) {
    d.shtValid=false; ++d.shtErrors;
    Serial.println("SHT31 CRC failure; reading discarded"); return;
  }
  const uint16_t rawT=(uint16_t(data[0])<<8)|data[1];
  const uint16_t rawH=(uint16_t(data[3])<<8)|data[4];
  d.temperatureC=-45.0f+175.0f*rawT/65535.0f;
  d.humidity=100.0f*rawH/65535.0f;
  d.shtValid=true; ++d.shtSamples;
  Serial.printf("SHT31 address=0x%02X T=%.2f C / %.2f F RH=%.2f%% samples=%lu errors=%lu CRC=OK\n",
    d.shtAddress,d.temperatureC,d.temperatureC*1.8f+32,d.humidity,
    (unsigned long)d.shtSamples,(unsigned long)d.shtErrors);
}
