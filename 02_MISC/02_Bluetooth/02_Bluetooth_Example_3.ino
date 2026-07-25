/*
Example 3: BLE Peripheral (Server) - Smart Sensor / LED Control
*/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Define UUIDs for Service and Characteristics
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define SENSOR_CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define LED_CHAR_UUID "d5875406-fa50-4bfa-982a-152586b0251b"

BLECharacteristic *pSensorCharacteristic;
bool deviceConnected = false;

// Server Callbacks to track connection status
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("Client Connected");
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("Client Disconnected");
    // Restart advertising
    BLEDevice::startAdvertising();
  }
};

// Callback for when the Client writes to the LED Characteristic
class LEDCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    //std::string value = pCharacteristic->getValue();
    String value = pCharacteristic->getValue();
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
  BLEDevice::init("ESP32-S3_Example_3");

  // 2. Create BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 3. Create BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 4. Create Sensor Characteristic (Read & Notify)
  pSensorCharacteristic = pService->createCharacteristic(
    SENSOR_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pSensorCharacteristic->addDescriptor(new BLE2902());  // Required for Notify

  // 5. Create LED Characteristic (Write)
  BLECharacteristic *pLedCharacteristic = pService->createCharacteristic(
    LED_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE);
  pLedCharacteristic->setCallbacks(new LEDCallback());

  // 6. Start the Service and Advertising
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  BLEDevice::startAdvertising();

  Serial.println("BLE Server is running. Waiting for connections...");
}

void loop() {
  if (deviceConnected) {
    // Simulate reading a sensor and updating the characteristic
    int sensorValue = random(0, 100);
    //pSensorCharacteristic->setValue(sensorValue);
    String payload = String(sensorValue);
    pSensorCharacteristic->setValue(payload.c_str());  // ✅ Sends "42" as ASCII text

    pSensorCharacteristic->notify();
    Serial.print("Notified Sensor Value: ");
    Serial.println(sensorValue);

    delay(2000);  // Update every 2 seconds
  }
  delay(10);
}