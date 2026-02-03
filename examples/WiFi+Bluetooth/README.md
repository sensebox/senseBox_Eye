The ESP32S3 chip includes both WiFi and Bluetooth functionality. Attach the included antenna to use it. Without the antenna the board might get damaged and the signal strength is drastically reduced.

# Examples
### mac-address
Example script from the ESP32 BSP to print the mac addresses of the MCU to serial.

### wifi-hotspot
Create an access point and host a small website there.

### wifi-connect
Connect to a WiFi access point and continuously print the signal strength. Values can range between 0 (good) and -120 (bad). If the signal is lost try to reconnect.

### ble
Create a service and characteristic and then send and receive values through that characteristics. Can be tested with the "nRF Connect" app for example. 

### openSenseMap
Send data from an HDC1080 sensor to a device on the openSenseMap.