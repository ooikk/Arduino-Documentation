/*
  Example 4: BLE Central (Client) - Heart Rate Monitor Receiver 
  UPDATED for ESP32 Core v3.x continuous scanning and reconnection.
  Run Example 3 in another ESP32 to simulate heart beat monitor
*/



#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLEScan.h>             
#include <BLEAdvertisedDevice.h> 

#define TARGET_DEVICE_NAME  "ESP32-S3_Example_3" \

// Below UUID must match Server or Example 3
#define TARGET_SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define TARGET_CHAR_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define TARGET_LED_CHAR_UUID "d5875406-fa50-4bfa-982a-152586b0251b"

BLEClient* pClient = nullptr;
BLERemoteCharacteristic* pRemoteCharacteristic = nullptr;    // Notify characteristic
BLERemoteCharacteristic* pRemoteLedCharacteristic = nullptr; // Write characteristic
BLEScan* pBLEScan = nullptr;

bool connected = false;
bool doConnect = false;
BLEAdvertisedDevice* myDevice = nullptr;
unsigned long lastCmdTime = 0;

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
  
  Serial.print("❤️ Sensor Data Received (Notify): ");
  Serial.println(value);
}

// Client Callbacks
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
  
  // 1. Get Service
  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(TARGET_SERVICE_UUID));
  if (pRemoteService == nullptr) {
    Serial.println("❌ Failed to find service UUID.");
    pClient->disconnect();
    return false;
  }

  // 2. Get Sensor Notify Characteristic
  pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(TARGET_CHAR_UUID));
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("❌ Failed to find sensor characteristic UUID.");
    pClient->disconnect();
    return false;
  }

  // Register for notifications
  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
    Serial.println("✅ Registered for Notifications!");
  }

  // 3. Get LED Write Characteristic
  pRemoteLedCharacteristic = pRemoteService->getCharacteristic(BLEUUID(TARGET_LED_CHAR_UUID));
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
  Serial.println("Starting BLE Client...");
  
  BLEDevice::init("ESP32-S3_Example_4");
  
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  
  pBLEScan->setInterval(160);
  pBLEScan->setWindow(128);
  pBLEScan->setActiveScan(true);
  
  Serial.println("Setup complete.");
}

void loop() {
  // 1. Connect logic
  if (doConnect) {
    if (!connectToServer()) {
      Serial.println("❌ Connection attempt failed. Retrying scan...");
    }
    doConnect = false;
  }

  // 2. Scan vs Connected operations
  if (!connected && !doConnect) {
    Serial.println("🔍 Scanning for BLE Server...");
    pBLEScan->start(5, false);
    pBLEScan->clearResults();
    delay(1000);
  } else {
    // Connected: Send write commands every 5 seconds
    if (connected && pRemoteLedCharacteristic != nullptr && pRemoteLedCharacteristic->canWrite() && ((millis() - lastCmdTime) > 5000)) {
      int cmdValue = random(0, 5); 
      String cmdStr = String(cmdValue);
      
      pRemoteLedCharacteristic->writeValue(cmdStr.c_str());
      Serial.print("📤 Sent command to Server: ");
      Serial.println(cmdStr);
      
      lastCmdTime = millis();
    }
    delay(1000);
  }
}