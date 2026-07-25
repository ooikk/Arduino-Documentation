/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/

/*************************************************************
To start the server:
02_Bluetooth_BLE_Server.ino

Use another ESP32 to:
Scan the server:
02_Bluetooth_BLE_Scanner.ino
Read the sensorValue from Server:
02_Bluetooth_BLE_Client_Read.ino

use mobile apps to scan and display sensorValue:
nRF Connect 

NOTE: Reset the Server after starting the clients

*************************************************************/
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>  // REQUIRED for notifications to work

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define SERVER_NAME "ESP32 BLE Server Example"

// 1. DECLARE THE POINTER GLOBALLY so both setup() and loop() can see it
BLECharacteristic *pCharacteristic;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  if (!BLEDevice::init(SERVER_NAME)) {
    Serial.println("BLE initialization failed!");
    return;
  }

  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Note: advertiseOnDisconnect is not a standard method in the base library.
  // We handle re-advertising via ServerCallbacks (shown in best practices),
  // but we will leave it out here to ensure it compiles cleanly.

  // 2. ASSIGN IT INSIDE setup() (Notice the added PROPERTY_NOTIFY)
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY  // REQUIRED for notify()
  );

  // 3. ADD THE CCCD DESCRIPTOR (Required by Bluetooth spec for notifications)
  pCharacteristic->addDescriptor(new BLE2902());

  pCharacteristic->setValue("Hello World");
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
  // Simulate reading a sensor and updating the characteristic
  int sensorValue = random(0, 100);

  // Your String concatenation is fine here, though char arrays are more memory-efficient
  String msg = "Sensor Value: " + String(sensorValue);

  // Now loop() can see pCharacteristic because it is global
  pCharacteristic->setValue(msg);
  pCharacteristic->notify();

  Serial.print("Notified Sensor Value: ");
  Serial.println(msg);

  delay(5000);  // Update every 5 seconds
}
