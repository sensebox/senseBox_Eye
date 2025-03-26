### Pinout
```cpp
#define PIN_QWIIC_SDA 2
#define PIN_QWIIC_SCL 1

Wire.begin(PIN_QWIIC_SDA,PIN_QWIIC_SCL);
```
These pins are both used by the I2C QWIIC interface and the integrated battery sensor (max17048).

# Examples
### i2c-scanner
Standard I2C Scanner based on [this](https://playground.arduino.cc/Main/I2cScanner/).

### vl53l8cx
Print closest distance from vl53l8cx in serial monitor (based on code generated from blockly).

### display
Print the senseBox-bike loading animation and a QR-Code on the display.

### hdc1080
Print temperature and humidity to serial.

### mpu6050
Print acceleration, gyro and temperature to serial.

### sps30
Print PM1.0, PM2.5, PM4.0 and PM10 to serial.

### i2c-chain
Combines display, vl53l8cx, hdc1080, mpu6050 and sps30. Draws the sensor measurements on the display and prints the loop times to serial.