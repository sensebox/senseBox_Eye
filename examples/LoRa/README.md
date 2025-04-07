# Pinout
```cpp
#define LORA_TX 43
#define LORA_RX 44
```

# Examples
### basic-at-command
Send AT-command to LoRa-modem and print response to serial. The appropriate AT-commands can be found in the command specification ("XXX" is a placeholder).

### ttn-example
Connect to a ttn-application and continuously send a demo message to it. Replace the Appkey and Appeui placeholders ("XXX") in the code with your own.