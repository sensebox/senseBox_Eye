### Pinout
```cpp
#define IO14 14
#define IO48 48
```

# QWIIC interface
### of the senseBox Eye
```
-4--IO14
-3--IO48
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
Display a guy with a hat on the LED-matrix.