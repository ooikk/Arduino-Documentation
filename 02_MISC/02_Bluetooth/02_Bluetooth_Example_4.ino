/*
  Example 4: BLE Central (Client) - Heart Rate Monitor Receiver (Fixed)
  UPDATED for ESP32 Core v3.x continuous scanning and reconnection.
  Run Example 3 in another ESP32 to simulate heart beat monitor
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLEScan.h>             
#include <BLEAdvertisedDevice.h> 

#define TARGET_DEVICE_NAME  "ESP32-S3_Example_3" 
#define TARGET_SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define TARGET_CHAR_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEClient* pClient = nullptr;
BLERemoteCharacteristic* pRemoteCharacteristic = nullptr;
BLEScan* pBLEScan = nullptr;

bool connected = false;
bool doConnect = false;
BLEAdvertisedDevice* myDevice = nullptr;

// Notification Callback
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  
  String value = "";
  for (size_t i = 0; i < length; i++) {
    value += (char)pData[i];
  }
  
  Serial.print("❤️ Heart Rate Received (Notify): ");
  Serial.println(value);
}

// Client Callbacks (Only updates flags)
class MyClientCallbacks : public BLEClientCallbacks {
  void onConnect(BLEClient* pClient) override {
    Serial.println("✅ Connected to Server!");
  }

  void onDisconnect(BLEClient* pClient) override {
    connected = false;
    Serial.println("⚠️ Server disconnected! Will resume scanning...");
  }
};

// Scan Callbacks
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    // Match by Name OR Service UUID for robust detection
    if ((advertisedDevice.haveName() && advertisedDevice.getName() == TARGET_DEVICE_NAME) ||
        (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(TARGET_SERVICE_UUID)))) {
        
      Serial.print("🎯 Target found! Address: ");
      Serial.println(advertisedDevice.getAddress().toString().c_str());
      
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
  pClient->setClientCallbacks(new MyClientCallbacks());
  
  if (!pClient->connect(myDevice)) {
    Serial.println("❌ Failed to connect.");
    return false;
  }
  
  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(TARGET_SERVICE_UUID));
  if (pRemoteService == nullptr) {
    Serial.println("❌ Failed to find service UUID.");
    pClient->disconnect();
    return false;
  }

  pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(TARGET_CHAR_UUID));
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("❌ Failed to find characteristic UUID.");
    pClient->disconnect();
    return false;
  }

  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
    Serial.println("✅ Registered for Notifications! Waiting for data...");
  }

  connected = true;
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Heart Rate Client...");
  
  BLEDevice::init("ESP32-S3_Example_4");
  
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  
  // Set timing parameters in 0.625ms units
  // 160 * 0.625ms = 100ms interval, 128 * 0.625ms = 80ms window
  pBLEScan->setInterval(160);
  pBLEScan->setWindow(128);
  pBLEScan->setActiveScan(true);
  
  Serial.println("Setup complete.");
}

void loop() {
  // 1. Found server -> Connect
  if (doConnect) {
    if (!connectToServer()) {
      Serial.println("❌ Connection attempt failed. Retrying scan...");
    }
    doConnect = false;
  }

  // 2. Disconnected -> Keep Scanning continuously
  if (!connected && !doConnect) {
    Serial.println("🔍 Scanning for BLE Server...");
    pBLEScan->start(5, false); // Scan for 5 seconds per cycle
    pBLEScan->clearResults();
    delay(1000);
  } else {
    delay(1000); // Connected: Notifications handled automatically by callback
  }
}