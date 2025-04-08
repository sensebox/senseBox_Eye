#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>  // Required for notify support
#define ARDUHAL_LOG_LEVEL_LOCAL ESP_LOG_VERBOSE
#include "esp_log.h"

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "772df7ec-8cdc-4ea9-86af-410abe0ba257"

BLEServer* pServer = nullptr;
BLEService* pService = nullptr;
BLECharacteristic* pCharacteristic = nullptr;

float valueToSend = 0.2;

// Optional: receive data from client
class CustomCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) override {
    String rxValue = (pCharacteristic->getValue()).c_str();
    if (rxValue.length() > 0) {
      Serial.print("Received data: ");
      Serial.println(rxValue.c_str());
    }
  }
};

class CustomServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    Serial.println("Client connected");
    pServer->getAdvertising()->start();  // Restart advertising
  }

  void onDisconnect(BLEServer* pServer) override {
    Serial.println("Client disconnected, restarting advertising...");
    delay(500);  // Give the BLE stack some time
    pServer->getAdvertising()->start();  // Restart advertising
  }
};

bool writeBLE(float value) {
  if (!pCharacteristic) return false;
  pCharacteristic->setValue((uint8_t*)&value, sizeof(float));  // Send float (4 bytes)
  pCharacteristic->notify();
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

  BLEDevice::init("senseBox-BLE[000]");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new CustomServerCallbacks());

  // Create BLE service
  pService = pServer->createService(SERVICE_UUID);

  // Create BLE characteristic
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
  );

  // Add Client Characteristic Configuration Descriptor (for notify support)
  pCharacteristic->addDescriptor(new BLE2902());

  // Optional: handle data written from clients
  pCharacteristic->setCallbacks(new CustomCharacteristicCallbacks());

  // Start service and advertising
  pService->start();
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // iOS compatibility
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE server is ready. Connect and write to the characteristic.");
}

void loop() {
  static unsigned long lastSendTime = 0;
  if (millis() - lastSendTime >= 1000) {
    if (writeBLE(valueToSend)) {
      Serial.print("Sent value: ");
      Serial.println(valueToSend);
      valueToSend += 1.0;
    }
    lastSendTime = millis();
  }
}
