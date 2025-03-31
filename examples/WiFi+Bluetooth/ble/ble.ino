#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>  // Required for notify support
#define ARDUHAL_LOG_LEVEL_LOCAL ESP_LOG_VERBOSE
#include "esp_log.h"

#define SERVICE_UUID        "5fafc201-1fb5-459e-8fcc-c5c9c331914a"
#define CHARACTERISTIC_UUID "7e14e070-84ea-489f-b45a-e1317364b978"

BLEServer* pServer = nullptr;
BLEService* pService = nullptr;
BLECharacteristic* pCharacteristic = nullptr;

class CustomBLECallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) override {
    String value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.println("Received data:");
      for (char c : value) {
        Serial.print(c);
      }
      Serial.println();
    }
  }
};

bool writeBLE(float value) {
  if (!pCharacteristic) return false;
  pCharacteristic->setValue((uint8_t*)&value, sizeof(float));  // Send as float (4 bytes)
  pCharacteristic->notify();
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  BLEDevice::init("senseBox-eye");
  pServer = BLEDevice::createServer();

  // Create the service
  pService = pServer->createService(SERVICE_UUID);

  // Create the characteristic
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
  );

  // Add CCCD (notify descriptor)
  pCharacteristic->addDescriptor(new BLE2902());

  // Set callback
  pCharacteristic->setCallbacks(new CustomBLECallbacks());

  // Start the service
  pService->start();
  delay(100); // Give BLE stack time

  // Start advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // For iOS compatibility
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE server is ready. Connect and write to the characteristic.");

  // Send a test value periodically
  float value = 0.2;
  while(writeBLE(value)) {
    Serial.print("Sent value: ");
    Serial.println(value);
    value += 1.0;
    delay(1000);
  }
}

void loop() {
  // Nothing to do here — BLE runs in background
}
