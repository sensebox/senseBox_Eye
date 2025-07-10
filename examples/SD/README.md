### Pinout
```cpp
#define VSPI_MISO   40 // DAT
#define VSPI_MOSI   38 // CMD
#define VSPI_SCLK   39 // SCK/CLK
#define VSPI_SS     41 // CS
#define SD_ENABLE   3
#define SD_SW       42 // card detect pin (LOW = card inserted)
```

# Examples
### sd-card-basics
Test basic sd-card functionality and print results to serial.