/*
  Example 4 (NimBLE v2.x): BLE Central (Client) - Receiver & Controller
  Updated for NimBLE-Arduino 2.0+ API
  Converted for ESP32-S3 using NimBLE-Arduino.
  Run the server sketch on another ESP32-S3 to test.
*/
/* 
*** Key Changes Made for NimBLE:
1. NimBLEAddress Handling: Instead of keeping a direct pointer to the BLEAdvertisedDevice (which gets recycled after a scan), 
   we capture and allocate a NimBLEAddress copy to ensure a reliable connection target.
2. Subscription Method: Replaced the stock registerForNotify() with NimBLE's native subscribe(true, notifyCallback) method.
3. Write Command: Used pRemoteLedCharacteristic->writeValue(data, length, response) which gives explicit control over write-with-response vs. write-without-response flags.
*** Specifically, in NimBLE v2.0+:
1. NimBLEAdvertisedDeviceCallbacks was renamed to NimBLEScanCallbacks.
2. onResult() now requires a const pointer: (const NimBLEAdvertisedDevice* advertisedDevice).
3. pBLEScan->setAdvertisedDeviceCallbacks() was replaced by pBLEScan->setScanCallbacks().

*/


/*
  Example 4 (NimBLE v2.x): Stable BLE Central (Client)
  Fixes callback stack crash and heap allocation leaks.
*/

#include <NimBLEDevice.h>

#define TARGET_DEVICE_NAME   "ESP32-S3_Example_3"

// UUIDs must match the Server sketch
#define TARGET_SERVICE_UUID  "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define TARGET_CHAR_UUID     "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define TARGET_LED_CHAR_UUID "d5875406-fa50-4bfa-982a-152586b0251b"

NimBLEClient* pClient = nullptr;
NimBLERemoteCharacteristic* pRemoteCharacteristic = nullptr;     // Notify characteristic
NimBLERemoteCharacteristic* pRemoteLedCharacteristic = nullptr; // Write characteristic
NimBLEScan* pBLEScan = nullptr;

bool connected = false;
bool foundTarget = false;
NimBLEAddress targetAddress; // Stored as a direct object (no dynamic heap allocation)
unsigned long lastCmdTime = 0;

// Notification Callback
static void notifyCallback(
  NimBLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  
  String value = "";
  for (size_t i = 0; i < length; i++) {
    value += (char)pData[i];
  }
  
  Serial.print("❤️ Sensor Data Received (Notify): ");
  Serial.println(value);
}

// Client Callbacks
class MyClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient) override {
    Serial.println("✅ Connected to Server!");
  }

  void onDisconnect(NimBLEClient* pClient, int reason) override {
    connected = false;
    Serial.println("⚠️ Server disconnected! Resuming scan...");
  }
};

// Scan Callbacks
class MyScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
    if (foundTarget || connected) return;

    if ((advertisedDevice->haveName() && advertisedDevice->getName() == TARGET_DEVICE_NAME) ||
        (advertisedDevice->haveServiceUUID() && advertisedDevice->isAdvertisingService(NimBLEUUID(TARGET_SERVICE_UUID)))) {
        
        Serial.print("🎯 Target found! Address: ");
        Serial.println(advertisedDevice->getAddress().toString().c_str());
        
        // Save address copy and flag for loop() processing
        targetAddress = advertisedDevice->getAddress();
        foundTarget = true;
        // Note: Do NOT call getScan()->stop() here to avoid stack panics!
    }
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(targetAddress.toString().c_str());
  
  if (pClient == nullptr) {
    pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallbacks());
  }
  
  if (!pClient->connect(targetAddress)) {
    Serial.println("❌ Failed to connect.");
    return false;
  }
  
  // 1. Get Service
  NimBLERemoteService* pRemoteService = pClient->getService(NimBLEUUID(TARGET_SERVICE_UUID));
  if (pRemoteService == nullptr) {
    Serial.println("❌ Failed to find service UUID.");
    pClient->disconnect();
    return false;
  }

  // 2. Get Sensor Notify Characteristic
  pRemoteCharacteristic = pRemoteService->getCharacteristic(NimBLEUUID(TARGET_CHAR_UUID));
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("❌ Failed to find sensor characteristic UUID.");
    pClient->disconnect();
    return false;
  }

  // Subscribe to Notifications
  if (pRemoteCharacteristic->canNotify()) {
    if (pRemoteCharacteristic->subscribe(true, notifyCallback)) {
      Serial.println("✅ Subscribed to Notifications!");
    } else {
      Serial.println("❌ Failed to subscribe to notifications!");
    }
  }

  // 3. Get LED Write Characteristic
  pRemoteLedCharacteristic = pRemoteService->getCharacteristic(NimBLEUUID(TARGET_LED_CHAR_UUID));
  if (pRemoteLedCharacteristic == nullptr) {
    Serial.println("⚠️ Warning: Failed to find LED write characteristic UUID.");
  } else {
    Serial.println("✅ Found LED write characteristic!");
  }

  connected = true;
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting NimBLE BLE Client...");
  
  NimBLEDevice::init("ESP32-S3_Example_4");
  
  pBLEScan = NimBLEDevice::getScan();
  pBLEScan->setScanCallbacks(new MyScanCallbacks());
  pBLEScan->setInterval(160);
  pBLEScan->setWindow(128);
  pBLEScan->setActiveScan(true);
  
  Serial.println("Setup complete. Ready to scan.");
}

void loop() {
  // 1. Safely handle found targets in main loop context
  if (foundTarget && !connected) {
    pBLEScan->stop(); // Safe to stop scan here!
    foundTarget = false;
    
    if (!connectToServer()) {
      Serial.println("❌ Connection failed. Retrying scan...");
    }
  }

  // 2. Scan vs Connected management
  if (!connected && !foundTarget) {
    if (!pBLEScan->isScanning()) {
      Serial.println("🔍 Scanning for BLE Server...");
      pBLEScan->start(5000, false); // 5-second scan window
      delay(1000);
    }
  } else if (connected) {
    // Connected: Send write commands every 5 seconds
    if (pRemoteLedCharacteristic != nullptr && pRemoteLedCharacteristic->canWrite() && ((millis() - lastCmdTime) > 5000)) {
      int cmdValue = random(0, 5); // Send '0' to '5'
      String cmdStr = String(cmdValue);
      
      pRemoteLedCharacteristic->writeValue(cmdStr.c_str(), cmdStr.length(), false);
      Serial.print("📤 Sent command to Server: ");
      Serial.println(cmdStr);
      
      lastCmdTime = millis();
    }
  }
  
  delay(10);
}