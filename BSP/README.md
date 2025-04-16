Our module is the "ESP32-S3-WROOM-1-N16R8" which has 16MB flash (Quad SPI) and 8MB PSRAM (Octal SPI) on top of 512KB SRAM.

## arduino-esp32 core
- `boards.txt` (only contains the segment relevant for senseBox Eye)
- `partitions-16MB-tinyuf2.csv`
- `pins_arduino.h`
- `variant.cpp`
- `tinyuf2-sensebox-eye/` (only `tinyuf2.bin` and `bootloader.bin` are relevant for arduino-esp32 I think)

## platformio/espressif32
- `sensebox_eye.json`