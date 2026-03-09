#include "config.h"
#include <LoRaWAN_ESP32.h>
#include <SPI.h>

const char* band       = RADIOLIB_LORAWAN_BAND;
const uint8_t subband = RADIOLIB_LORAWAN_SUBBAND;
SX1276 radio = new Module(LORA_CS, LORA_INT, RADIOLIB_NC, LORA_INT);


// create the LoRaWAN node
LoRaWANNode* node;

uint64_t joinEUI =   RADIOLIB_LORAWAN_JOIN_EUI;
uint64_t devEUI  =   RADIOLIB_LORAWAN_DEV_EUI;
uint8_t appKey[] = { RADIOLIB_LORAWAN_APP_KEY };
uint8_t nwkKey[] = { RADIOLIB_LORAWAN_NWK_KEY };

#define LORAWAN_DEV_INFO_SIZE 36
uint8_t deviceInfo[LORAWAN_DEV_INFO_SIZE] = {0};

#define SERIAL_DATA_BUF_LEN  64
uint8_t serialDataBuf[SERIAL_DATA_BUF_LEN] = {0};
uint8_t serialIndex = 0;

#define UPLINK_PAYLOAD_MAX_LEN  256
uint8_t uplinkPayload[UPLINK_PAYLOAD_MAX_LEN] = {0};
uint16_t uplinkPayloadLen = 0;

uint32_t previousMillis = 0;


void setup() {
  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && (millis() - start) < 3000) { delay(10); }

  Serial.println("\n=== Pin Setup ===");
  Serial.println("\n=== SET SD_ENABLE (3) to LOW ===");
  pinMode(SD_ENABLE, OUTPUT);
  digitalWrite(SD_ENABLE, LOW);  // Try HIGH first
  delay(100);  // Let power stabilize

  Serial.println("\n=== SET SD_CS (41) to HIGH ===");
  pinMode(41, OUTPUT);
  digitalWrite(41, HIGH);
  delay(100);  // Let power stabilize

  Serial.println("\n=== SET LORA_CS (43) to LOW ===");
  // Deselect ALL SPI devices
  pinMode(43, OUTPUT);
  digitalWrite(43, LOW);
  delay(100);  // Let power stabilize

  Serial.println("\n=== Pin Setup Done ===");

  persist.provision(band, subband, joinEUI, devEUI, appKey, nwkKey);

  Serial.println("\n----- Setup -----");  
  Serial.println("Initialising the radio..");
  int16_t state = radio.begin();
  if(state!= RADIOLIB_ERR_NONE) {
    Serial.print("State: ");
    Serial.print(state);
    Serial.print(F("\nInitialise radio failed\n"));
    Serial.println("STOPPING HERE AND WAITING FOR USER RESET.");
    while(1);
  } else {
    Serial.println("Radio sucessfully initialised.\n");
  }

  Serial.println("Trying to join network ...");
  // recreate the LoRaWAN node with the radio from the persisted session
  node = persist.manage(&radio);

  if (!node->isActivated()) {
    Serial.printf("\nCould not join network. We'll go to sleep for %d seconds and try again later.\n", (int64_t)SLEEP_DURATION_SECONDS);
    esp_sleep_enable_timer_wakeup((int64_t)SLEEP_DURATION_SECONDS * 1000000);
    esp_deep_sleep_start();
  }
  Serial.println("Join-request successful!\n");

  // Turn of management of uplink intervals to the TTN Fair Use Policy [ONLY FOR TESTING]
  node->setDutyCycle(false);
  
  uint8_t uplinkData = 42;
  uint8_t* uplinkDataPtr = &uplinkData;
  
  Serial.printf("Ready to send data! Sending uplink now... \n");
  
  // send the uplink
  state = node->sendReceive(uplinkDataPtr, sizeof(uplinkData), LORAWAN_UPLINK_USER_PORT);
  
  if (state!= RADIOLIB_LORAWAN_NO_DOWNLINK && state!= RADIOLIB_ERR_NONE) {
    Serial.print("Error in sendReceive:");
    Serial.println(state);
    Serial.println();
  } else {
    Serial.println("Uplink data was successful sent!\n");
  }
  
  Serial.println("Saving LoRa Session...");
  // Save LoRa configuration before sleep
  persist.saveSession(node);
  Serial.printf("Saved session! Going to sleep for %d seconds ...\n", (int64_t)SLEEP_DURATION_SECONDS);
  Serial.println("------------------------");
  esp_sleep_enable_timer_wakeup((int64_t)SLEEP_DURATION_SECONDS * 1000000);
  // and off to bed we go
  esp_deep_sleep_start();
}


void loop() {
  // not used
}