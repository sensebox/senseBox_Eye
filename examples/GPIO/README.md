### Pinout
```cpp
#define GPIO_PIN 3
```

# QWIIC interface
### of the senseBox-Eye MCU
```
-4--
-3--IO3
-2--3v3
-1--GND
```

### of the LED-Matrix
```
-4--LED
-3--
-2--3v3
-1--GND
```

### of the water temperature sensor
```
-4--
-3--temperature
-2--3v3
-1--GND
```

### of the soil moisture sensor (smt50)
```
-4--moisture
-3--temperature
-2--3v3
-1--GND
```

# Examples
### analog
Read analog input and print it to serial.

### smt50
Print soil temperature to serial.

### led-matrix
Display a guy with a hat on the LED-matrix. The GPIO pins need to be switched in the QWIIC cable for this to work.