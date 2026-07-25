/*
  ESP32 Core v3.x BLE Client Read
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
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>         
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// --- TARGET DEVICE DETAILS ---
#define TARGET_NAME         "ESP32 BLE Server Example" 
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEScan* pBLEScan;
BLEClient* pClient = nullptr;
BLERemoteCharacteristic* pRemoteCharacteristic = nullptr;

bool isConnected = false;
bool doConnect = false;
BLEAdvertisedDevice* myDevice = nullptr;

// Client callback to catch disconnections
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pClient) override {
    Serial.println("✅ Connected to Server!");
  }

  void onDisconnect(BLEClient* pClient) override {
    isConnected = false;
    Serial.println("❌ Disconnected from Server. Will resume scanning...");
  }
};

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    // Simplified Match: Check Name or Service UUID
    if (advertisedDevice.haveName() && advertisedDevice.getName() == TARGET_NAME) {
      Serial.println("✅ Target device found by Name! Stopping scan...");
      
      pBLEScan->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remote BLE Server
  if (!pClient->connect(myDevice)) {
    Serial.println("❌ Failed to connect.");
    return false;
  }

  // Obtain a reference to the service
  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(SERVICE_UUID));
  if (pRemoteService == nullptr) {
    Serial.print("❌ Failed to find service UUID: ");
    Serial.println(SERVICE_UUID);
    pClient->disconnect();
    return false;
  }

  // Obtain a reference to the characteristic
  pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(CHARACTERISTIC_UUID));
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("❌ Failed to find characteristic UUID: ");
    Serial.println(CHARACTERISTIC_UUID);
    pClient->disconnect();
    return false;
  }

  isConnected = true;
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Client...");

  BLEDevice::init("ESP32-S3 Client");
  
  pBLEScan = BLEDevice::getScan(); 
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  
  // FIX: Values converted to units of 0.625ms
  // 100ms / 0.625ms = 160
  // 80ms / 0.625ms = 128
  pBLEScan->setInterval(160);
  pBLEScan->setWindow(128);
  pBLEScan->setActiveScan(true); 

  Serial.println("Setup complete. Starting scan...");
}

void loop() {
  // 1. If we found a target during scanning, connect to it
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("✅ Connection successful!");
    } else {
      Serial.println("❌ Failed to connect to server.");
    }
    doConnect = false;
  }

  // 2. If connected, read data continuously
  if (isConnected) {
    if (pRemoteCharacteristic != nullptr && pRemoteCharacteristic->canRead()) {
      String value = pRemoteCharacteristic->readValue();
      Serial.print("📡 Read value from server: ");
      Serial.println(value.c_str()); 
    } else {
      Serial.println("❌ Characteristic does not support reading.");
    }
    delay(2000); 
  } 
  // 3. Otherwise, keep scanning
  else {
    Serial.println("🔍 Scanning for target device...");
    pBLEScan->start(5, false);
    pBLEScan->clearResults(); 
    delay(1000); 
  }
}