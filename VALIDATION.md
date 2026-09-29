# Validation

Tested on 2026-09-29 using Teensy MicroMod, SparkFun Input and Display Carrier and SHT31 at 0x44.

- PJRC discovery identified the actual MicroMod target. USB-only, I/O, TFT and full incremental flashes succeeded.
- USB RX/TX and 600 MHz heartbeat passed.
- Onboard controller: 0x71, ID 0xAE, firmware 1.0. All seven physical inputs observed, cumulative mask 0x7F, zero I2C errors.
- TFT communication returned power 0x9C and format 0x05. User confirmed display, graph pages, controls, LEDs and buzzer operation.
- SHT31 returned repeated CRC-valid samples with zero sensor errors. This verifies communication, not calibrated accuracy.
- Two distinct new CSV sessions were written (four and two rows), synced, closed, reopened and read back correctly. Existing files were preserved. SD probing during recording was correctly rejected.
- Unit switching, all five page states, graph window changes and increasing history count were verified via serial.
- Full firmware compiled with all warnings enabled, without warnings or errors. FLASH code 103824 bytes, data 13816, headers 8308; RAM1 variables 26944, code 101688, padding 29384; 366272 bytes available for locals.

Outstanding: full 30-minute history soak, SD-full/removal and power-loss fault injection, alert-threshold hardware testing and calibrated sensor comparison. Settings/history are volatile. SD status reflects a filesystem probe, not an insertion switch.

Local transcripts, toolchains and HEX files are excluded from the repository. Tool.ps1 regenerates firmware locally. The repository packaging does not change the tested firmware source.
