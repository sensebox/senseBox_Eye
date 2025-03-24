### Pinout
```cpp
#define SDA 2
#define SCL 1

Wire.begin(SDA,SCL);
```

# Examples
### i2c-scanner
Standard I2C Scanner based on [this](https://playground.arduino.cc/Main/I2cScanner/).

### vl53l8cx
Print closest distance from vl53l8cx in serial monitor (based on code generated from blockly).