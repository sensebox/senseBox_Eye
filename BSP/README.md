Our module is the "ESP32-S3-WROOM-1-N16R8" which has 16MB flash (Quad SPI) and 8MB PSRAM (Octal SPI) on top of 512KB SRAM.

## [arduino-esp32 core](https://github.com/espressif/arduino-esp32)
- `boards.txt` (only contains the segment relevant for senseBox Eye)
- `partitions-16MB-tinyuf2.csv`
- `pins_arduino.h`
- `variant.cpp`
- `tinyuf2/` (only `tinyuf2.bin` and `bootloader.bin` are relevant for arduino-esp32 I think)
- If we want to add this to the official [arduino-esp32 core](https://github.com/espressif/arduino-esp32) we need to add this [fix](https://github.com/espressif/arduino-esp32/issues/11245#issuecomment-2842952196) in the corresponding pull-request (or wait for it to be fixed beforehand)

## platformio/espressif32
- `sensebox_eye.json`