# arduino-as-a-component
The esp-idf allows to include the esp32-arduino framework. This allows one to use arduino libraries in the esp-idf. 

In this example the Adafruit Neopixel library is included. Once flashed, the RGB-LED onboard the senseBox Eye should go through a rainbow cycle of colors.

Before building you need to run the following commands in this folder:
```
mkdir -p components && \
cd components && \
git clone https://github.com/espressif/arduino-esp32.git arduino
```

[This wiki page](https://docs.espressif.com/projects/arduino-esp32/en/latest/esp-idf_component.html) gives more details about including arduino as a component.
