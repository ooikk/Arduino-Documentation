# Bluetooth

## 1. Introduction and Description     
**Hardware Capabilities**     
The ESP32-S3 is a highly integrated, low-power Wi-Fi and Bluetooth System-on-Chip (SoC).
⚠️ CRITICAL DISTINCTION: Unlike the original ESP32, the ESP32-S3 ONLY supports Bluetooth Low Energy (BLE) 5.0. It does not support Bluetooth Classic (BR/EDR).     
- What this means: You cannot use it for Classic Bluetooth Serial (SPP), Classic Audio (A2DP), or connect to older legacy Bluetooth devices.
- What it is great for: IoT sensor networks, wearables, smart home devices, beacons, and low-power wireless communication with smartphones.
**Key Bluetooth 5.0 LE Features on ESP32-S3**     
- Higher Speed: Supports the 2 Mbps PHY (Physical Layer), doubling the data rate compared to BLE 4.2.
- Long Range: Supports the Coded PHY (125 kbps and 500 kbps), significantly increasing the communication range (up to 1km in ideal open environments).
- Advertising Extensions: Allows for larger advertising payloads and better filtering.
- Bluetooth Mesh: Supports BLE Mesh networking for controlling large numbers of devices (e.g., smart lighting).
- Dual-Core Architecture: The Bluetooth radio and baseband are handled by the chip's dedicated hardware, freeing up the dual Xtensa LX7 CPU cores for your application logic.

For detail BLE for ESP32 Arduino Core:     
https://github.com/espressif/arduino-esp32/tree/master/libraries/BLE     


## 2. Arduino IDE Ecosystem & Libraries      
To use Bluetooth in the Arduino IDE, you must have the ESP32 board package by Espressif installed via the Boards Manager.
There are two primary libraries for BLE on the ESP32-S3:      
- ESP32 BLE Arduino (Standard): The original library written by Neil Kolban, now maintained by Espressif. It uses an Object-Oriented C++ wrapper around the ESP-IDF Bluedroid/NimBLE stack. (Used in the examples below).
- [NimBLE-Arduino by h2zero](https://github.com/h2zero/NimBLE-Arduino) (Highly Recommended): A port of the Apache Mynewt NimBLE stack. It uses significantly less RAM and Flash and is much more stable for complex applications. For production projects on the ESP32-S3, NimBLE is the industry standard.      

**Library**     
- ```BLEDevice.h```: Built-in, official Espressif BLE library for creating GATT servers, clients, and advertising packets.
  https://github.com/espressif/arduino-esp32/blob/master/libraries/BLE/src/BLEDevice.h      
- ```NimBLEDevice.h```: NimBLE-Arduino by h2zero. https://github.com/h2zero/NimBLE-Arduino           
  https://github.com/h2zero/NimBLE-Arduino/blob/master/src/NimBLEDevice.h      
  The gold standard for Bluetooth Low Energy development on the ESP32 and ESP32-S3 within the Arduino ecosystem.     
  Here is why it is highly recommended over the default BLEDevice (Bluedroid) library:
  1. Dramatically Lower Resource Usage
     The default ESP32 BLE stack (Bluedroid) is heavy. NimBLE-Arduino uses approximately 50% less flash space and around 100KB less RAM. This is especially critical on the ESP32-S3 if you are also running Wi-Fi, a display, or other memory-intensive tasks.
  2. Drop-in Compatibility
     It was specifically designed to be a fork structured for Arduino compilation while maintaining API compatibility with the original ESP32 BLE library. In most cases, you can switch to it simply by changing your #include statements:
     ```
     // Instead of this:
     // #include <BLEDevice.h>
     // #include <BLEServer.h>

     // You just do this:
     #include <NimBLEDevice.h>
     // (NimBLEServer, NimBLECharacteristic, etc. are used the exact same way)

     ```
  3. Better Stability and Performance
     Because it is based on the Apache Mynewt NimBLE stack (a completely open-source, industry-proven BLE stack), it handles connections, reconnections, and multi-client scenarios much more robustly than the older Bluedroid implementation.
  4. Active Maintenance
     The h2zero repository is actively maintained, frequently updated to support the latest ESP-IDF versions, and widely adopted by the professional ESP32 community.      

## 3. API Overview (Standard BLE Library)     
The standard BLE API follows a strict hierarchical, Object-Oriented structure:
- ```BLEDevice```: The singleton manager. Initializes the Bluetooth hardware and sets the device name.
- ```BLEServer``` / ```BLEClient```: Defines whether your ESP32 is broadcasting data (Peripheral/Server) or reading data from others (Central/Client).
- ```BLEService```: A collection of characteristics. Identified by a UUID (e.g., 0x180A for Device Information).
- ```BLECharacteristic```: The actual data container. Identified by a UUID. Has properties (Read, Write, Notify, Indicate).
- ```BLEDescriptor```: Metadata for a characteristic (e.g., describing the format of the data).     
Core Classes & Methods:
- ```BLEDevice::init("Device_Name")``` - Initializes the BLE controller with a local device name.
- ```BLEDevice::createServer()``` - Creates a BLE Server instance/ Instantiates a GATT Server instance.
- ```BLEServer::createService(UUID)``` - Creates a container group (Service) with a unique 128-bit or 16-bit UUID.
- ```BLEServer::getAdvertising()``` - Accesses the advertising module to make the ESP32 visible to scanners.
- ```BLEService::createCharacteristic(UUID, properties)``` - Adds a data point with specific permissions (PROPERTY_READ, PROPERTY_WRITE, PROPERTY_NOTIFY).
- ```BLECharacteristic::setValue()``` / ```getValue()``` - Reads or sets the underlying data string or byte array.
- ```BLECharacteristic::notify()``` - Pushes data to connected clients/ Sends an immediate update to connected clients without them having to re-read.

## 4. Application Examples     

**Example 1: ESP32-S3 BLE Server (Send Data to Smartphone)**     
This example sets up the ESP32-S3 as a server that advertises a custom service. You can read the characteristic value using a smartphone app like nRF Connect or LightBlue.
```
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
  BLEDevice::init("ESP32-S3_BLE_Demo");

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
```
**Example 2: ESP32-S3 BLE Server Callbacks (Receive Commands)**
To react when a central device (like a smartphone) sends a command to the ESP32-S3, attach a Characteristic Callback:
```
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define CHARACTERISTIC_UUID "19b10001-e8f2-537e-4f6c-d104768a1214"

const int LED_PIN = 2; // Built-in LED on many S3 boards

// Custom Callback Class to handle Incoming Data
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {
        Serial.print("Received Value: ");
        for (int i = 0; i < value.length(); i++) {
          Serial.print(value[i]);
        }
        Serial.println();

        // Control an onboard LED based on incoming character
        if (value[0] == '1') {
          digitalWrite(LED_PIN, HIGH);
          Serial.println("LED turned ON");
        } else if (value[0] == '0') {
          digitalWrite(LED_PIN, LOW);
          Serial.println("LED turned OFF");
        }
      }
    }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  BLEDevice::init("ESP32-S3_Control");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  // Attach callback listener
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Send 1 or 0");

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();

  Serial.println("Waiting for smartphone write commands...");
}

void loop() {
  delay(1000);
}
```

**Example 3: BLE Peripheral (Server) - Smart Sensor / LED Control**     
This example creates a BLE Server that exposes a Service with two Characteristics: one to read a simulated sensor value, and one to write data to control an LED.
```
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Define UUIDs for Service and Characteristics
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define SENSOR_CHAR_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define LED_CHAR_UUID       "d5875406-fa50-4bfa-982a-152586b0251b"

BLECharacteristic *pSensorCharacteristic;
bool deviceConnected = false;

// Server Callbacks to track connection status
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Client Connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Client Disconnected");
      // Restart advertising
      BLEDevice::startAdvertising(); 
    }
};

// Callback for when the Client writes to the LED Characteristic
class LEDCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        Serial.print("LED State Received: ");
        Serial.println(value.c_str());
        // Control your GPIO here based on value
        if (value[0] == '1') {
          Serial.println("Turning LED ON");
        } else {
          Serial.println("Turning LED OFF");
        }
      }
    }
};

void setup() {
  Serial.begin(115200);
  
  // 1. Initialize BLE
  BLEDevice::init("ESP32-S3_SmartDevice");
  
  // 2. Create BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // 3. Create BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // 4. Create Sensor Characteristic (Read & Notify)
  pSensorCharacteristic = pService->createCharacteristic(
                                         SENSOR_CHAR_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );
  pSensorCharacteristic->addDescriptor(new BLE2902()); // Required for Notify
  
  // 5. Create LED Characteristic (Write)
  BLECharacteristic *pLedCharacteristic = pService->createCharacteristic(
                                          LED_CHAR_UUID,
                                          BLECharacteristic::PROPERTY_WRITE
                                        );
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
    pSensorCharacteristic->setValue(sensorValue);
    pSensorCharacteristic->notify();
    Serial.print("Notified Sensor Value: ");
    Serial.println(sensorValue);
    
    delay(2000); // Update every 2 seconds
  }
  delay(10);
}
```

**Example 4: BLE Central (Client) - Scanning and Reading**      
This example turns the ESP32-S3 into a Central device that scans for a specific BLE peripheral, connects to it, and reads a characteristic.
```
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>

// UUID of the device we want to connect to (e.g., a specific heart rate monitor)
#define TARGET_DEVICE_NAME "My_BLE_Sensor" 
#define TARGET_SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define TARGET_CHAR_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEClient* pClient;
bool connected = false;

// Scan Callbacks
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      // Check if the scanned device is our target
      if (advertisedDevice.haveName() && advertisedDevice.getName() == TARGET_DEVICE_NAME) {
        Serial.print("Target found! Address: ");
        Serial.println(advertisedDevice.getAddress().toString().c_str());
        
        // Stop scanning once found
        BLEDevice::getScan()->stop();
        
        // Connect to the device
        connectToServer(advertisedDevice.getAddress());
      }
    }
};

void connectToServer(BLEAddress pAddress) {
  pClient = BLEDevice::createClient();
  Serial.println("Connecting to target...");
  
  if (pClient->connect(pAddress)) {
    Serial.println("Connected!");
    connected = true;
    
    // Obtain the remote service
    BLERemoteService* pRemoteService = pClient->getService(TARGET_SERVICE_UUID);
    if (pRemoteService != nullptr) {
      // Obtain the remote characteristic
      BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(TARGET_CHAR_UUID);
      
      if (pRemoteCharacteristic != nullptr && pRemoteCharacteristic->canRead()) {
        // Read the value
        std::string value = pRemoteCharacteristic->readValue();
        Serial.print("Read Value: ");
        Serial.println(value.c_str());
      }
    }
  } else {
    Serial.println("Failed to connect.");
  }
}

void setup() {
  Serial.begin(115200);
  BLEDevice::init("ESP32-S3_Central");
  
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // Active scan uses more power, but gets results faster
  pBLEScan->start(10, false); // Scan for 10 seconds
  
  Serial.println("Scanning for BLE devices...");
}

void loop() {
  if (connected) {
    // You can periodically read/write to the server here
    delay(2000);
  } else {
    delay(1000);
  }
}
```

**Optimization Tip**: If you encounter SRAM memory limitations or high battery draw, replace the standard #include <BLEDevice.h> with the NimBLE-Arduino library (#include <NimBLEDevice.h>). It shares a virtually identical syntax while cutting RAM usage by ~50% and Flash memory usage by over ~100KB.     


## 5. Pro-Tips for ESP32-S3 Bluetooth Development     
1. Switch to NimBLE for Production:
   The standard BLEDevice library uses the Bluedroid stack, which consumes a lot of RAM. If you are using the ESP32-S3 (which has 512KB SRAM and often PSRAM), it will work fine, but for optimal performance, install the NimBLE-Arduino library via the Library Manager. The API is nearly identical, but it is vastly more memory-efficient.
2. Antenna Considerations:
   The ESP32-S3 has excellent RF performance, but it is highly dependent on the module's antenna design. If using a bare chip, ensure your PCB antenna or external IPEX connector is properly impedance-matched (50 ohms).
3. Use PSRAM:
   If you are running Wi-Fi and BLE simultaneously, or using complex BLE profiles, enable PSRAM in the Arduino IDE Tools menu (Tools -> PSRAM -> "OPI PSRAM" or "QSPI PSRAM" depending on your specific S3 module). This prevents memory allocation failures.
4. Handling UUIDs:
   For custom applications, always use 128-bit UUIDs (like the ones in the examples) to avoid collisions with standard Bluetooth SIG profiles (which use 16-bit UUIDs like 0x180A).
5. Deep Sleep:
   The ESP32-S3 can retain BLE connectivity (specifically for advertising or maintaining a connection) while in Deep Sleep, provided you configure the ULP (Ultra Low Power) coprocessor and RTC memory correctly. This is ideal for battery-powered BLE beacons.

# Reference      

https://randomnerdtutorials.com/esp32-bluetooth-low-energy-ble-arduino-ide/

https://www.oceanlabz.in/esp32-bluetooth-low-energy-ble/

https://www.oceanlabz.in/esp32-esp-now/

https://www.luisllamas.es/esp32-bluetooth-low-energy-ble/

https://randomnerdtutorials.com/esp32-bluetooth-low-energy-ble-arduino-ide/

https://deepbluembedded.com/esp32-bluetooth-classic-with-arduino-complete-guide/
