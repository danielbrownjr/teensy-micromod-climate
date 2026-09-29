#pragma once
#include <Arduino.h>
#if !defined(ARDUINO_TEENSY_MICROMOD)
#error This project requires the Teensy MicroMod target.
#endif
// Arduino pin numbers, traced through both SparkFun schematics and PJRC core.
namespace Pins {
constexpr uint8_t tftCs=4, tftDc=5, tftReset=42, backlight=3;
constexpr uint8_t sdCs=10, mosi=11, miso=12, sck=13;
constexpr uint8_t ledClock=40, ledData=41, buzzer=2;
constexpr uint8_t sda=18, scl=19, buttonInterrupt=29;
}
#ifndef PLAYGROUND_STAGE
#define PLAYGROUND_STAGE 3
#endif
