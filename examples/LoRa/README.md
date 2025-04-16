# Pinout
```cpp
#define LORA_TX 43
#define LORA_RX 44
```

# Examples
### basic-at-command
Send AT-command to LoRa-modem and print response to serial. The appropriate AT-commands can be found in the command specification ("XXX" is a placeholder).

### ttn-example-otaa
Connect to a ttn-application through otaa and continuously send confirmed and unconfirmed demo messages to it. Replace the APPKEY and APPEUI placeholders ("XXX") in the code with your own.

### ttn-example-abp
Connect to a ttn-application through abp and continuously send confirmed and unconfirmed demo messages to it. Replace the DEVADDR, APPSKEY and NWKSKEY placeholders ("XXX") in the code with your own.