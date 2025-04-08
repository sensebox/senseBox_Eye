The ESP32S3 chip includes both WiFi and Bluetooth functionality. For WiFi attach the antenna.

# Examples
### mac-address
Example script from the ESP32 BSP to print the mac addresses of the MCU to serial.

### wifi-hotspot
Create an access point and host a small website there.

### wifi-connect
Connect to a WiFi access point and continuously get the current time from the ntp server. If the signal is lost or the current time was not received try to reconnect.

### ble
Create a service and characteristic and then send and receive values through that characteristics. Can be tested with the "nRF Connect" app for example. 