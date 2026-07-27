/*
  Example 3: BLE Peripheral (Server) - Smart Sensor / LED Control
  Converted for NimBLE-Arduino on ESP32-S3
*/

#include <Arduino.h>
#include <NimBLEDevice.h>

// Define UUIDs for Service and Characteristics
#define DEVICE_NAME       "ESP32-S3_Example_3"
#define SERVICE_UUID      "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define SENSOR_CHAR_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define LED_CHAR_UUID     "d5875406-fa50-4bfa-982a-152586b0251b"

NimBLECharacteristic *pSensorCharacteristic = nullptr;
bool deviceConnected = false;

// Server Callbacks to track connection status
class MyServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *pServer, NimBLEConnInfo& connInfo) override {
    deviceConnected = true;
    Serial.printf("Client Connected: %s\n", connInfo.getAddress().toString().c_str());
  }

  void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo& connInfo, int reason) override {
    deviceConnected = false;
    Serial.println("Client Disconnected");
    // Restart advertising immediately
    NimBLEDevice::startAdvertising();
  }
};

// Callback for when the Client writes to the LED Characteristic
class LEDCallback : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo& connInfo) override {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0) {
      Serial.print("LED State Received: ");
      Serial.println(value.c_str());

      // Control your GPIO here based on value
      if (value[0] == '1') {
        Serial.println("Turning LED ON");
      } else if (value[0] == '0') {
        Serial.println("Turning LED OFF");
      }
    }
  }
};

void setup() {
  Serial.begin(115200);

  // 1. Initialize BLE
  NimBLEDevice::init(DEVICE_NAME);

  // 2. Create BLE Server
  NimBLEServer *pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 3. Create BLE Service
  NimBLEService *pService = pServer->createService(SERVICE_UUID);

  // 4. Create Sensor Characteristic (Read & Notify)
  // Note: BLE2902 descriptor is handled automatically by NimBLE!
  pSensorCharacteristic = pService->createCharacteristic(
    SENSOR_CHAR_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );

  // 5. Create LED Characteristic (Write)
  NimBLECharacteristic *pLedCharacteristic = pService->createCharacteristic(
    LED_CHAR_UUID,
    NIMBLE_PROPERTY::WRITE
  );
  pLedCharacteristic->setCallbacks(new LEDCallback());

  // 6. Start the Service
  pService->start();

  // 7. Configure & Start Advertising (Explicit payload split)
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();

  // Primary Advertising Packet: Name & Discoverability Flags
  NimBLEAdvertisementData advData;
  advData.setName(DEVICE_NAME);
  advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
  pAdvertising->setAdvertisementData(advData);

  // Scan Response Packet: 128-bit Service UUID
  NimBLEAdvertisementData scanData;
  scanData.setCompleteServices(NimBLEUUID(SERVICE_UUID));
  pAdvertising->setScanResponseData(scanData);

  pAdvertising->start();

  Serial.println("BLE Server is running. Waiting for connections...");
}

void loop() {
  if (deviceConnected) {
    // Simulate reading a sensor and updating the characteristic
    int sensorValue = random(0, 255);
    String payload = String(sensorValue);
    
    pSensorCharacteristic->setValue(payload.c_str());  // Sends ASCII text
    pSensorCharacteristic->notify();
    
    Serial.print("Notified Sensor Value: ");
    Serial.println(sensorValue);

    delay(2000);  // Update every 2 seconds
  }
  delay(10);
}