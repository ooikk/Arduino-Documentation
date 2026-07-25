/*
Example 1: ESP32-S3 BLE Server (Send Data to Smartphone)
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// Unique 128-bit UUIDs generated for custom Service and Characteristic
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Server...");

  // 1. Initialize BLE Device
  BLEDevice::init("ESP32-S3_BLE_Example_1");

  // 2. Create BLE Server
  BLEServer *pServer = BLEDevice::createServer();

  // 3. Create BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 4. Create BLE Characteristic with READ and WRITE properties
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // Set initial value
  pCharacteristic->setValue("Hello from ESP32-S3!");

  // 5. Start Service
  pService->start();

  // 6. Start Advertising so nearby devices can discover it
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions for iPhone connection helpers
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE Server is up and advertising!");
}

void loop() {
  // Update value dynamically every 3 seconds
  static uint32_t counter = 0;
  counter++;

  String payload = "Sensor Reading: " + String(counter);
  pCharacteristic->setValue(payload.c_str());
  pCharacteristic->notify(); // Push notification to connected clients

  delay(3000);
}