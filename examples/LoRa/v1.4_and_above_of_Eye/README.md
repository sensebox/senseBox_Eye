# for versions v1.4 and upwards of the senseBox Eye

# Pinout
```cpp
#define LORA_TX 43
#define LORA_RX 44
```

# Examples

### mcci-otaa
Connect to a ttn-application through otaa and continuously send "Hello world!". Replace the DEVEUI, APPKEY and APPEUI placeholders ("XXX") in the code with your own.
This example uses the [MCCI LoRaWAN LMIC library](https://github.com/mcci-catena/arduino-lmic).

### RadioLib-otaa
Connect to a ttn-application through otaa and send the number 42. Then go to sleep for 30 seconds and repeat.
This example uses v7.0.0 of the [RadioLib library](https://github.com/jgromes/RadioLib).