# arduino-as-a-component
The esp-idf allows to include the esp32-arduino framework. This allows one to use arduino libraries in the esp-idf. 

In this example the Adafruit Neopixel library is included. Once flashed, the RGB-LED onboard the senseBox Eye should go through a rainbow cycle of colors.

Before building you need to run the following commands in this folder:
```
mkdir -p components && \
cd components && \
git clone https://github.com/espressif/arduino-esp32.git arduino
```

The Adafruit_Neopixel library then has to be copied into `components/arduino/libraries`. The `components/arduino/CMakeLists.txt` now has to be adjusted. After the line `set(ARDUINO_ALL_LIBRARIES` add `Adafruit_NeoPixel`. Furthermore the following lines have to be added:
```
set(ARDUINO_LIBRARY_Adafruit_NeoPixel_SRCS libraries/Adafruit_NeoPixel/Adafruit_NeoPixel.cpp
  libraries/Adafruit_NeoPixel/esp.c 
  libraries/Adafruit_NeoPixel/esp8266.c 
  libraries/Adafruit_NeoPixel/kendyte_k210.c)
```
And lastly the includedirs have to be adjusted like so: `set(includedirs variants/${CONFIG_ARDUINO_VARIANT}/ cores/esp32/ ${ARDUINO_LIBRARIES_INCLUDEDIRS} libraries/Adafruit_NeoPixel)`.

When building I had problems with the i2c-slave (I fixed one line and commented another one out). I also had to make a tiny adjustment to the neopixel library code (the error message will tell you what to do).

[This wiki page](https://docs.espressif.com/projects/arduino-esp32/en/latest/esp-idf_component.html) gives more details about including arduino as a component.
