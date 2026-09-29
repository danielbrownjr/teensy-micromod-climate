# Teensy MicroMod Climate Dashboard

A room-climate dashboard for the SparkFun Teensy MicroMod processor, SparkFun Input and Display Carrier (DEV-16985), and an SHT31 connected through Qwiic/I2C.

## Features

- Live temperature and humidity, with Celsius/Fahrenheit switching.
- Temperature and humidity graphs with 2-, 10-, and 30-minute windows; 900 samples retained in RAM.
- CSV logging to microSD, creating a new file per session and syncing every row.
- Six APA102 LEDs showing temperature or humidity level.
- Optional high-temperature/humidity buzzer alerts, disabled by default.
- Diagnostics for buttons, I2C, sensor errors, TFT communication and SD status.

## Hardware

Connect a 3.3 V-compatible SHT31 breakout to the carrier's Qwiic port: black GND, red 3.3 V, blue SDA, yellow SCL. Addresses 0x44 and 0x45 are supported. The onboard button controller uses 0x71.

| Signal | Teensy Arduino pin |
|---|---:|
| TFT CS / DC / reset | 4 / 5 / 42 |
| TFT backlight (active low) | 3 |
| SD CS | 10 |
| Shared SPI MOSI / MISO / clock | 11 / 12 / 13 |
| APA102 clock / data | 40 / 41 |
| Buzzer | 2 |
| Qwiic SDA / SCL | 18 / 19 |
| Button interrupt | 29 |

These are Arduino pin numbers, not M.2 pads or GPIO bit positions. Mapping was verified against both SparkFun schematics and PJRC's MicroMod core. The carrier SD slot uses SPI, not BUILTIN_SDCARD/SDIO, and has no physical card-detect signal.

## Build and flash on Windows

Requirements: PowerShell, internet for initial setup, USB data cable. Setup downloads checksum-verified Arduino CLI 1.5.1 and Teensy core 1.62.0 into ignored `.work/`. No global Arduino installation is required.

```powershell
.\Tool.ps1 Setup
.\Tool.ps1 Build -Stage full
.\Tool.ps1 Flash -Stage full
# The command above lists boards; repeat with the actual Teensy usb: path:
.\Tool.ps1 Flash -Stage full -Port usb:C0000/0/0/8
# Wait about seven seconds for Windows USB enumeration.
.\Tool.ps1 Monitor -Seconds 30 -Commands '?s'
```

The example USB path is machine-dependent. Monitoring selects a sole Teensy serial port automatically; specify `-Port COMn` if needed. Close other serial monitors first. If software reboot fails, press the carrier BOOT/program button and retry upload.

The script maps this checkout to `U:` to avoid ARM GCC's Windows long-path header lookup failure. It refuses to replace an occupied volume. Use `-Drive V` or another unused letter consistently if necessary. The mapping is temporary until Windows restarts. Do not run simultaneous builds with different drive choices in one checkout.

Target: `teensy:avr:teensyMM:usb=serial,speed=600,opt=o2std`. Other Teensy targets are rejected. All compiler warnings are enabled. HEX files are generated in ignored `firmware/<stage>/`. Build first after fresh Setup.

| Stage | Behavior |
|---|---|
| bringup | USB heartbeat and RX/TX test only |
| io | Sensor, buttons, SD and serial dashboard controls |
| display | Adds TFT |
| full | Adds LEDs and buzzer |

## Controls

| Control | Action |
|---|---|
| Left / Right | Cycle live readings, temperature graph, humidity graph, diagnostics, alert settings |
| A | Toggle C/F |
| B | Start/stop CSV recording |
| Up / Down on graphs | Change time window |
| Center on live/diagnostics | Scan I2C and probe SD; SD probe is skipped while recording |
| Center on alert settings | Select temperature, humidity, or enabled state |
| Up / Down on alert settings | Change limit or enable/disable alerts |

Samples arrive about every two seconds with both CRC checks verified. Graphs use sample age and break lines across missing samples. History and settings reset on boot. Invalid readings show as unavailable rather than displaying stale values as current.

LEDs display a blue-to-red bar across 10–40 C, or 0–100% RH on the humidity page. These are visualization scales, not comfort recommendations. Alerts start disabled, with editable initial upper limits of 30 C and 70% RH. Either threshold can trigger a short beep at most every ten seconds. Clearing uses 0.5 C / 2% RH hysteresis. Threshold editing always uses Celsius; sensor failure clears the active alarm.

## Recording

B creates the first unused `CLIM0000.CSV` through `CLIM9999.CSV` in the card root. Exclusive creation prevents overwriting existing files. Each valid new sample adds:

```text
uptime_ms,temperature_c,temperature_f,relative_humidity_pct
19388,22.37,72.26,65.23
```

Time is milliseconds since boot, not a calendar timestamp, and wraps after about 49.7 days. Logging starts off. Stop recording before removing the card. Write/sync errors stop logging and display an error. Hot-removal and power-loss recovery have not been validated.

Serial commands: `?` help; `s` status; `i` scan; `d` SD probe; `u` units; `r` record; `n`/`p` next/previous page; `+`/`-` Up/Down; `c` Center; `v` read up to 4096 bytes of this boot's most recent stopped recording; `b` beep; `l` LED toggle.

## Dependencies and source layout

Teensy 1.62.0 bundles ARM GCC 15.2.1, ILI9341_t3 1.0, SPI/Wire 1.0, SD 2.0.0 and SdFat 2.1.2. No separately installed libraries are required. The upstream board definition sets TEENSYDUINO=160; the script preserves that value. These are bundled versions, not claims about latest standalone library releases.

- `Bringup/`: USB-only sketch.
- `Playground/BoardPins.h`: verified wiring and stage guard.
- `Playground/Sht31.cpp`: sensor conversion, CRC validation and retries.
- `Playground/Inputs.cpp`: buttons, scanning and SD status.
- `Playground/Climate.cpp`: history, controls, logging and alerts.
- `Playground/Display.cpp`: dashboard pages.
- `Playground/Outputs.cpp`: LEDs and buzzer.
- `Playground/Playground.ino`: scheduling and serial commands.

See [VALIDATION.md](VALIDATION.md) for test evidence and limitations.

## Primary references

- [PJRC installation](https://www.pjrc.com/teensy/td_download.html) and [package index](https://www.pjrc.com/teensy/package_teensy_index.json).
- [Carrier hardware and button firmware](https://github.com/sparkfun/MicroMod_Input_and_Display_Carrier/tree/584dcf56417b0107d3fab93dfbe4c9b66cd8819a).
- [Processor hardware](https://github.com/sparkfun/MicroMod_Teensy_Processor/tree/51ca16ddcabd38d134c051391ffedf16dafc19a0).
- [SHT3x-DIS datasheet](https://sensirion.com/media/documents/213E6A3B/63A5A569/Datasheet_SHT3x_DIS.pdf).

No license has been selected for this repository yet. Third-party dependencies retain their licenses and are downloaded separately.
