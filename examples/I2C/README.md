### Pinout
```cpp
#define PIN_QWIIC_SDA 2
#define PIN_QWIIC_SCL 1

Wire.begin(PIN_QWIIC_SDA,PIN_QWIIC_SCL);
```

# Examples
### i2c-scanner
Standard I2C Scanner based on [this](https://playground.arduino.cc/Main/I2cScanner/).

### vl53l8cx
Print closest distance from vl53l8cx in serial monitor (based on code generated from blockly).