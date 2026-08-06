# Bluetooth

## 1. Introduction and Description

### Hardware Capabilities

The ESP32-S3 is a highly integrated, low-power Wi-Fi and Bluetooth System-on-Chip (SoC).

⚠️ **CRITICAL DISTINCTION:** Unlike the original ESP32, the ESP32-S3 ONLY supports Bluetooth Low Energy (BLE) 5.0. It does not support Bluetooth Classic (BR/EDR).

- **What this means:** You cannot use it for Classic Bluetooth Serial (SPP), Classic Audio (A2DP), or connect to older legacy Bluetooth devices.
- **What it is great for:** IoT sensor networks, wearables, smart home devices, beacons, and low-power wireless communication with smartphones.

### Key Bluetooth 5.0 LE Features on ESP32-S3

- **Higher Speed:** Supports the 2 Mbps PHY (Physical Layer), doubling the data rate compared to BLE 4.2.
- **Long Range:** Supports the Coded PHY (125 kbps and 500 kbps), significantly increasing the communication range (up to 1km in ideal open environments).
- **Advertising Extensions:** Allows for larger advertising payloads and better filtering.
- **Bluetooth Mesh:** Supports BLE Mesh networking for controlling large numbers of devices (e.g., smart lighting).
- **Dual-Core Architecture:** The Bluetooth radio and baseband are handled by the chip's dedicated hardware, freeing up the dual Xtensa LX7 CPU cores for your application logic.

### BLE Server and Client

With Bluetooth Low Energy, there are two types of devices: the server and the client. The ESP32 can act either as a client or as a server.

The server advertises its existence, so it can be found by other devices, and contains the data that the client can read. The client scans the nearby devices, and when it finds the server it is looking for, it establishes a connection and listens for incoming data. This is called point-to-point communication.


<img width="50%" height="auto" alt="image" src="https://github.com/user-attachments/assets/b39454a3-c17a-4935-b0b2-9d8fe458f945" />


As mentioned previously, BLE also supports broadcast mode and mesh network:

- **Broadcast mode:** the server transmits data to many clients that are connected.
- **Mesh network:** all the devices are connected, this is a many to many connection.

Even though the broadcast and mesh network setups are possible to implement, they were developed very recently, so there aren't many examples implemented for the ESP32 at this moment.

### GATT

GATT stands for Generic Attributes and it defines a hierarchical data structure that is exposed to connected BLE devices. This means that GATT defines the way that two BLE devices send and receive standard messages. Understanding this hierarchy is important because it will make it easier to understand how to use BLE with the ESP32.


<img width="75%" height="auto" alt="image" src="https://github.com/user-attachments/assets/a7d17de1-37f9-4052-a8a2-8ab96932db45" />


- **Profile:** standard collection of services for a specific use case.
- **Service:** collection of related information, like sensor readings, battery level, heart rate, etc.
- **Characteristic:** it is where the actual data is saved on the hierarchy (value).
- **Descriptor:** metadata about the data.
- **Properties:** describe how the characteristic value can be interacted with. For example: read, write, notify, broadcast, indicate, etc.

In our example, we'll create a service with two characteristics. One for the temperature and another for the humidity. The actual temperature and humidity readings are saved on the value under their characteristics. Each characteristic has the notify property, so that it notifies the client whenever the values change.

### UUID

Each service, characteristic, and descriptor have a UUID (Universally Unique Identifier). A UUID is a unique 128-bit (16 bytes) number. For example:

```cpp
55072829-bc9e-4c53-938a-74a6d4c78776
```

See **What is a UUID in Bluetooth Low Energy (BLE)** for details.

### BLE Service

The top level of the hierarchy is a profile, which is composed of one or more services. Usually, a BLE device contains more than one service.

Every service contains at least one characteristic, or can also reference other services. A service is simply a collection of information, like sensor readings, for example.

There are predefined services for several types of data defined by the SIG (Bluetooth Special Interest Group) like: Battery Level, Blood Pressure, Heart Rate, Weight Scale, etc. [You can check here other defined services](https://www.bluetooth.com/specifications/assigned-numbers/).

### BLE Characteristic

The characteristic is always owned by a service, and it is where the actual data is contained in the hierarchy (value). The characteristic always has two attributes: characteristic declaration (that provides metadata about the data) and the characteristic value.

Additionally, the characteristic value can be followed by descriptors, which further expand on the metadata contained in the characteristic declaration.

The properties describe how the characteristic value can be interacted with. Basically, it contains the operations and procedures that can be used with the characteristic:

- Broadcast
- Read
- Write without response
- Write
- Notify
- Indicate
- Authenticated Signed Writes
- Extended Properties

For detail BLE for ESP32 Arduino Core:  
https://github.com/espressif/arduino-esp32/tree/master/libraries/BLE

---

## 2. Arduino IDE Ecosystem & Libraries

To use Bluetooth in the Arduino IDE, you must have the ESP32 board package by Espressif installed via the Boards Manager.

There are two primary libraries for BLE on the ESP32-S3:

- **ESP32 BLE Arduino (Standard):** The original library written by Neil Kolban, now maintained by Espressif. It uses an Object-Oriented C++ wrapper around the ESP-IDF Bluedroid/NimBLE stack. (Used in the examples below).
- **[NimBLE-Arduino by h2zero](https://github.com/h2zero/NimBLE-Arduino) (Highly Recommended):** A port of the Apache Mynewt NimBLE stack. It uses significantly less RAM and Flash and is much more stable for complex applications. For production projects on the ESP32-S3, NimBLE is the industry standard.

### Library

- **`BLEDevice.h`:** Built-in, official Espressif BLE library for creating GATT servers, clients, and advertising packets.  
  https://github.com/espressif/arduino-esp32/blob/master/libraries/BLE/src/BLEDevice.h
- **`NimBLEDevice.h`:** NimBLE-Arduino by h2zero.  
  https://github.com/h2zero/NimBLE-Arduino/blob/master/src/NimBLEDevice.h

The gold standard for Bluetooth Low Energy development on the ESP32 and ESP32-S3 within the Arduino ecosystem.

Here is why it is highly recommended over the default BLEDevice (Bluedroid) library:

1. **Dramatically Lower Resource Usage**  
   The default ESP32 BLE stack (Bluedroid) is heavy. NimBLE-Arduino uses approximately 50% less flash space and around 100KB less RAM. This is especially critical on the ESP32-S3 if you are also running Wi-Fi, a display, or other memory-intensive tasks.
2. **Drop-in Compatibility**  
   It was specifically designed to be a fork structured for Arduino compilation while maintaining API compatibility with the original ESP32 BLE library. In most cases, you can switch to it simply by changing your `#include` statements:

   ```cpp
   // Instead of this:
   // #include <BLEDevice.h>
   // #include <BLEServer.h>

   // You just do this:
   #include <NimBLEDevice.h>
   // (NimBLEServer, NimBLECharacteristic, etc. are used the exact same way)
   ```
3. **Better Stability and Performance**  
   Because it is based on the Apache Mynewt NimBLE stack (a completely open-source, industry-proven BLE stack), it handles connections, reconnections, and multi-client scenarios much more robustly than the older Bluedroid implementation.
4. **Active Maintenance**  
   The h2zero repository is actively maintained, frequently updated to support the latest ESP-IDF versions, and widely adopted by the professional ESP32 community.

---

## 3. API Overview (Standard BLE Library)

The standard BLE API follows a strict hierarchical, Object-Oriented structure:

- **`BLEDevice`:** The singleton manager. Initializes the Bluetooth hardware and sets the device name.
- **`BLEServer` / `BLEClient`:** Defines whether your ESP32 is broadcasting data (Peripheral/Server) or reading data from others (Central/Client).
- **`BLEService`:** A collection of characteristics. Identified by a UUID (e.g., 0x180A for Device Information).
- **`BLECharacteristic`:** The actual data container. Identified by a UUID. Has properties (Read, Write, Notify, Indicate).
- **`BLEDescriptor`:** Metadata for a characteristic (e.g., describing the format of the data).

### Core Classes & Methods

- `BLEDevice::init("Device_Name")` - Initializes the BLE controller with a local device name.
- `BLEDevice::createServer()` - Creates a BLE Server instance/ Instantiates a GATT Server instance.
- `BLEServer::createService(UUID)` - Creates a container group (Service) with a unique 128-bit or 16-bit UUID.
- `BLEServer::getAdvertising()` - Accesses the advertising module to make the ESP32 visible to scanners.
- `BLEService::createCharacteristic(UUID, properties)` - Adds a data point with specific permissions (PROPERTY_READ, PROPERTY_WRITE, PROPERTY_NOTIFY).
- `BLECharacteristic::setValue()` / `getValue()` - Reads or sets the underlying data string or byte array.
- `BLECharacteristic::notify()` - Pushes data to connected clients/ Sends an immediate update to connected clients without them having to re-read.

---

## 4. What is a UUID in Bluetooth Low Energy (BLE)?

UUID stands for **Universally Unique Identifier**.

In BLE, a UUID is a 128-bit (16-byte) number used to uniquely identify information. Think of it like a URL for a webpage or a file path on your computer. When a smartphone connects to your ESP32-S3, it doesn't know what your data is just by looking at raw bytes. It uses UUIDs to ask: "What kind of service is this?" and "What kind of data is inside this characteristic?"

In the BLE hierarchy, UUIDs are assigned to three things:

- **Service UUID:** Groups related data together (e.g., a "Heart Rate Service" or a "Custom Sensor Service").
- **Characteristic UUID:** The actual data point (e.g., "Heart Rate Measurement" or "LED Control State").
- **Descriptor UUID:** Metadata about the characteristic (e.g., 0x2902 is the standard UUID for a Client Characteristic Configuration Descriptor, which allows Notifications).

### How Do We Get UUID Values?

There are two ways to get UUIDs, depending on whether you are building a standard device or a custom one.

#### 1. Standard (16-bit) UUIDs: For Common, Official Profiles

If you are building something that already has an official Bluetooth specification (like a Heart Rate Monitor, Battery Level indicator, or Temperature Sensor), you must use the official 16-bit UUIDs assigned by the **Bluetooth Special Interest Group (SIG)**.

The BLE stack automatically expands these 16-bit numbers into the standard 128-bit "base UUID" format behind the scenes.

**Common Standard 16-bit UUIDs:**

- 0x180F : Battery Service
- 0x180D : Heart Rate Service
- 0x180A : Device Information Service
- 0x2A19 : Battery Level Characteristic
- 0x2902 : Client Characteristic Configuration Descriptor (CCCD - used for Notifications)

👉 Where to find them: You can browse the official, complete list here:  
[Bluetooth SIG Assigned Numbers (16-bit UUIDs)](https://bitbucket.org/bluetooth-SIG/public/src/main/assigned_numbers/uuids/service_uuids.yaml)

#### 2. Custom (128-bit) UUIDs: For Your Own Unique Applications

If you are building a custom project (like controlling a specific robot, sending proprietary sensor data, or a custom smart home device), you should generate your own 128-bit UUIDs. This guarantees your device won't accidentally conflict with a standard Bluetooth profile or another developer's device.

A 128-bit UUID looks like this:

```cpp
4fafc201-1fb5-459e-8fcc-c5c9c331914b
```

**👉 How to generate them:**

You can generate a random, statistically unique 128-bit UUID using several methods:

- **Online Generators (Easiest):**  
  Visit a site like https://uuidgenerator.net or https://uuidtools.com or https://www.guidgenerator.com/ and click "Generate".
- **Windows Command Prompt / PowerShell:**  
  Type: `powershell -command "[guid]::NewGuid().ToString()"`
- **Mac / Linux Terminal:**  
  Type: `uuidgen`
- **Python:**
  ```cpp
  import uuid
  print(str(uuid.uuid4()))
  ```

### Pro-Tips for UUIDs

1. **Keep them organized:** If you have multiple characteristics, make them visually similar so you know they belong to the same service. For example:
   - Service: `12345678-1234-1234-1234-123456789abc`
   - Char 1: `12345678-1234-1234-1234-123456789ab1`
   - Char 2: `12345678-1234-1234-1234-123456789ab2`
2. **Use a BLE Scanner App:** To verify your UUIDs are working, download a free BLE scanner app on your phone (like nRF Connect by Nordic Semiconductor or LightBlue). When your ESP32-S3 advertises, the app will display the exact Service and Characteristic UUIDs you defined, allowing you to read/write to them manually for testing.
3. **Case Sensitivity:** UUID strings in Arduino are generally case-insensitive, but it is standard convention to write them in lowercase hex characters.

### Manually Generate UUID

Random UUID generators are actually not helpful since it always return random numbers. The good news is: For custom applications, you do not need a random generator at all. You are the "authority" of your own project's UUID space. The only rule for custom 128-bit UUIDs is that they must be unique to your specific application so they don't clash with other devices.

Manually crafting them to share a common base and only differ at the end is actually a highly recommended best practice. It makes your code infinitely easier to read, debug, and organize.

**Here is easy ways to generate this pattern:**

**The "Copy-Paste & Tweak" (Easiest)**

Since a UUID is just a string of hexadecimal characters (0-9, a-f), you can simply write a base string and manually change the last character(s).

A standard 128-bit UUID has 32 hex characters, formatted as 8-4-4-4-12. The last group has 12 characters. You can lock the first 31 characters and just increment the last one.

```cpp
Base:      12345678-1234-1234-1234-123456789ab_
Service:   12345678-1234-1234-1234-123456789ab0  (or 'c', 'f', etc.)
Char 1:    12345678-1234-1234-1234-123456789ab1
Char 2:    12345678-1234-1234-1234-123456789ab2
Char 3:    12345678-1234-1234-1234-123456789ab3
```

*Note: Hexadecimal goes from 0-9 and then a-f. So after 9 comes a, not 10.*

**💡 Pro-Tip: The "Official" Bluetooth Base UUID Format**

If you want your custom UUIDs to look more "professional" or align with how the Bluetooth SIG structures things, you can use the official Bluetooth Base UUID:

```cpp
xxxxxxxx-0000-1000-8000-00805F9B34FB
```

In this format, the Bluetooth SIG reserves the middle part (0000-1000-8000-00805F9B34FB). You are only supposed to change the first 8 characters (xxxxxxxx).

Many companies generate one random 32-bit (8-character) hex number for their company/product, and then use standard 16-bit numbers at the end. For example:

```cpp
// Company/Product Base: a1b2c3d4
#define SERVICE_UUID       "a1b2c3d4-0000-1000-8000-00805F9B34FB"
#define TEMP_CHAR_UUID     "a1b2c3d4-0000-1000-8000-00805F9B3401" // Ends in 01
#define HUMIDITY_CHAR_UUID "a1b2c3d4-0000-1000-8000-00805F9B3402" // Ends in 02
```

*Note: The NimBLE/Arduino library will actually accept this format perfectly, and it's a very clean way to organize large projects.*

---

## 5. Application Examples

### Basic steps to setup ESP32 BLE Server

Setting up a GATT BLE Server on the ESP32-S3 follows a strict initialization sequence. The core architecture relies on the standard Bluetooth GATT hierarchy: Device → Server → Service → Characteristic → Advertising.

Here are the standard steps using the Arduino ESP32 BLE framework (`BLEDevice.h`):

1. **Initialize the BLE Stack: Device Name Setup.**  
   Assign a broadcast name to your ESP32-S3 and initialize the underlying Bluetooth hardware stack.
   ```cpp
   BLEDevice::init("ESP32-S3_BLE_Server");
   ```
2. **Instantiate the BLE Server: Server Creation.**  
   Create the primary BLE Server object. This acts as the root container for services and handles connection/disconnection event callbacks.
   ```cpp
   BLEServer *pServer = BLEDevice::createServer();
   // Optional: pServer->setCallbacks(new MyServerCallbacks());
   ```
3. **Create the GATT Service: Service Definition.**  
   Define a service using a unique 128-bit UUID to logically group your sensor or control characteristics.
   ```cpp
   BLEService *pService = pServer->createService(SERVICE_UUID);
   ```
4. **Add Characteristics & Descriptors: Data Endpoints.**  
   Instantiate characteristics inside your service with specific access permissions (Read, Write, Notify). If enabling notifications, attach the standard 2902 Client Characteristic Configuration Descriptor.
   ```cpp
   BLECharacteristic *pCharacteristic = pService->createCharacteristic(
     CHARACTERISTIC_UUID,
     BLECharacteristic::PROPERTY_READ   |
     BLECharacteristic::PROPERTY_WRITE  |
     BLECharacteristic::PROPERTY_NOTIFY
   );
   pCharacteristic->addDescriptor(new BLE2902());
   ```
5. **Start the Service: Memory Activation.**  
   Start the service to allocate its characteristics and make them active in memory.
   ```cpp
   pService->start();
   ```
6. **Configure & Start Advertising: Broadcasting.**  
   Attach your Service UUID to the advertising payload so central devices (such as a Web BLE client or smartphone) can discover your ESP32-S3, then start broadcasting.
   ```cpp
   BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
   pAdvertising->addServiceUUID(SERVICE_UUID);
   pAdvertising->setScanResponse(true);
   BLEDevice::startAdvertising();
   ```

### Basic steps to setup ESP32 BLE Client

Setting up a GATT BLE Client (Central) on the ESP32 using the stock `BLEDevice.h` library follows the reverse flow of a Server: Initialize Stack → Scan for Devices → Connect to Server → Discover Services & Characteristics → Read/Write/Subscribe.

Here are the basic steps to implement a BLE Client:

1. **Initialize the BLE Stack: Device Setup.**  
   Initialize the underlying BLE stack as a central client device. Unlike a server, a client does not require a public broadcast name.
   ```cpp
   BLEDevice::init("");
   ```
2. **Configure & Start BLE Scanner: Device Discovery.**  
   Get the scanner instance, attach an advertised device callback class to process incoming broadcasts, set active scanning, and initiate a scan window.
   ```cpp
   BLEScan* pBLEScan = BLEDevice::getScan();
   pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
   pBLEScan->setInterval(1349);
   pBLEScan->setWindow(449);
   pBLEScan->setActiveScan(true); // Active scanning requests scan response packets
   pBLEScan->start(5, false);     // Scan for 5 seconds
   ```
3. **Target Server Identification: Advertised Device Callback.**  
   Inside your `MyAdvertisedDeviceCallbacks::onResult()` function, check incoming advertisement packets for your target Service UUID or Device Name. Once matched, save a pointer to the device and stop scanning.
   ```cpp
   if (advertisedDevice.haveServiceUUID() && 
       advertisedDevice.isAdvertisingService(SERVICE_UUID)) {
     BLEDevice::getScan()->stop();
     myDevice = new BLEAdvertisedDevice(advertisedDevice);
     doConnect = true; // Set flag to connect in loop()
   }
   ```
4. **Connect to GATT Server: Establish Peer Connection.**  
   Instantiate a BLEClient object, attach optional connection state callbacks, and connect directly to the target BLEAdvertisedDevice.
   ```cpp
   BLEClient* pClient = BLEDevice::createClient();
   pClient->connect(myDevice); // Returns true if connection succeeds
   ```
5. **Discover Remote Services & Characteristics: GATT Tree Mapping.**  
   Query the connected server to retrieve handles for your target Service UUID and its associated Remote Characteristics.
   ```cpp
   BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
   BLERemoteCharacteristic* pSensorChar  = pRemoteService->getCharacteristic(SENSOR_CHAR_UUID);
   BLERemoteCharacteristic* pControlChar = pRemoteService->getCharacteristic(CONTROL_CHAR_UUID);
   ```
6. **Read, Write & Subscribe to Notifications: Data Exchange.**  
   Perform GATT operations on the server: write data to control characteristics, read value buffers, or register a callback function to receive asynchronous notifications.
   ```cpp
   // Write command payload to server
   if (pControlChar->canWrite()) {
     pControlChar->writeValue("1");
   }
   // Subscribe to incoming notifications
   if (pSensorChar->canNotify()) {
     pSensorChar->registerForNotify(notifyCallback);
   }
   ```

### Example 1: ESP32-S3 BLE Server (Send Data to Smartphone)

This example sets up the ESP32-S3 as a server that advertises a custom service. You can read the characteristic value using a smartphone app like nRF Connect, Arduino Bluetooth Controller or LightBlue.

```cpp
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
```

### Example 2: ESP32-S3 BLE Server Callbacks (Receive Commands)

To react when a central device (like a smartphone) sends a command to the ESP32-S3, attach a Characteristic Callback:

```cpp
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define CHARACTERISTIC_UUID "19b10001-e8f2-537e-4f6c-d104768a1214"

const int LED_PIN = 2; // Built-in LED on many S3 boards

// Custom Callback Class to handle Incoming Data
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
      String value = pCharacteristic->getValue();
      if (value.length() > 0) {
        Serial.print("Received Value: ");
        for (int i = 0; i < value.length(); i++) {
          Serial.print(value[i]);
        }
        Serial.println();

        // Control an onboard LED based on incoming character
        if (value == '1') {
          digitalWrite(LED_PIN, HIGH);
          Serial.println("LED turned ON");
        } else if (value == '0') {
          digitalWrite(LED_PIN, LOW);
          Serial.println("LED turned OFF");
        }
      }
    }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  BLEDevice::init("ESP32-S3_Example_2");
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

### Example 3: BLE Peripheral (Server) - Smart Sensor / LED Control

This example creates a BLE Server that exposes a Service with two Characteristics: one to read a simulated sensor value, and one to write data to control an LED.

```cpp
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
      String value = pCharacteristic->getValue();
      if (value.length() > 0) {
        Serial.print("LED State Received: ");
        Serial.println(value.c_str());
        // Control your GPIO here based on value
        if (value == '1') {
          Serial.println("Turning LED ON");
        } else if (value == '0') {
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
    String payload = String(sensorValue);
    pSensorCharacteristic->setValue(payload.c_str());  // ✅ Sends "42" as ASCII text
    pSensorCharacteristic->notify();
    Serial.print("Notified Sensor Value: ");
    Serial.println(sensorValue);
    
    delay(2000); // Update every 2 seconds
  }
  delay(10);
}
```

### Example 4: BLE Central (Client) - Scanning and Reading

This example turns the ESP32-S3 into a Central device that scans for a specific BLE peripheral, connects to it, and reads a characteristic.

```cpp
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

#define TARGET_DEVICE_NAME  "ESP32-S3_Example_3"

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
```

**Optimization Tip:** If you encounter SRAM memory limitations or high battery draw, replace the standard `#include <BLEDevice.h>` with the NimBLE-Arduino library (`#include <NimBLEDevice.h>`). It shares a virtually identical syntax while cutting RAM usage by ~50% and Flash memory usage by over ~100KB.

---

## 6. Pro-Tips for ESP32-S3 Bluetooth Development

1. **Switch to NimBLE for Production:**  
   The standard `BLEDevice` library uses the Bluedroid stack, which consumes a lot of RAM. If you are using the ESP32-S3 (which has 512KB SRAM and often PSRAM), it will work fine, but for optimal performance, install the NimBLE-Arduino library via the Library Manager. The API is nearly identical, but it is vastly more memory-efficient.
2. **Antenna Considerations:**  
   The ESP32-S3 has excellent RF performance, but it is highly dependent on the module's antenna design. If using a bare chip, ensure your PCB antenna or external IPEX connector is properly impedance-matched (50 ohms).
3. **Use PSRAM:**  
   If you are running Wi-Fi and BLE simultaneously, or using complex BLE profiles, enable PSRAM in the Arduino IDE Tools menu (Tools → PSRAM → "OPI PSRAM" or "QSPI PSRAM" depending on your specific S3 module). This prevents memory allocation failures.
4. **Handling UUIDs:**  
   For custom applications, always use 128-bit UUIDs (like the ones in the examples) to avoid collisions with standard Bluetooth SIG profiles (which use 16-bit UUIDs like 0x180A).
5. **Deep Sleep:**  
   The ESP32-S3 can retain BLE connectivity (specifically for advertising or maintaining a connection) while in Deep Sleep, provided you configure the ULP (Ultra Low Power) coprocessor and RTC memory correctly. This is ideal for battery-powered BLE beacons.

---

## 7. Sending/ Receiving Multiple Commands

In BLE development, both "Separate Characteristics" and "Single Command Pipe" approaches are widely used, but creating separate BLE characteristics for distinct hardware subsystems is considered the standard GATT best practice.

Here is a breakdown of how the two approaches compare, when to use each, and how to structure them cleanly.

### Approach 1: Separate Characteristics (Recommended for GATT)

In true Bluetooth LE design, Characteristics represent attributes/subsystems (e.g., LED state, Motor speed, Battery level).

**Why this is the best practice:**

- **Clean Code:** Separate callback handlers (or if/else checks per characteristic) keep logic decoupled.
- **Semantic & Tool Friendly:** In debugging apps like nRF Connect, you can inspect or control the LED without affecting the motor.
- **Granular Types & Permissions:** The LED might only need `PROPERTY_WRITE` for a 0/1 boolean, while the motor might need `PROPERTY_WRITE + PROPERTY_READ` for speed control (0–255 PWM).

```cpp
// Separate Characteristics under the SAME Service
#define LED_CHAR_UUID   "d5875406-fa50-4bfa-982a-152586b0251b"
#define MOTOR_CHAR_UUID "e6986517-gb61-5cgb-093b-262697c1362c"

// Unified or separate callbacks
class CommandCallback: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pChar) override {
    String val = pChar->getValue();
    
    // Check which characteristic received the data
    if (pChar->getUUID().equals(BLEUUID(LED_CHAR_UUID))) {
      digitalWrite(LED_PIN, val == '1' ? HIGH : LOW);
    } 
    else if (pChar->getUUID().equals(BLEUUID(MOTOR_CHAR_UUID))) {
      int speed = val.toInt(); // e.g. "255"
      analogWrite(MOTOR_PIN, speed);
    }
  }
};
```

### Approach 2: Single Command Pipe ("Serial/Stream" Style)

If you have dozens of minor commands (or a terminal-like interface), creating 20+ characteristics creates memory overhead and bloats your service definition. In this case, multiplexing over a single Command Characteristic makes sense.

Instead of raw string parsing like "L1" and "M255" (which gets tricky when parsing multi-digit numbers or floats), use one of two structured patterns:


#### Option A: Binary Protocol (Fastest & Lightest)    

Send fixed binary byte arrays rather than ASCII strings:


```
Byte 0 (Device ID)     Byte 1 (Action / Command)    Byte 2+ (Payload / Value)
0x01 (LED)             0x01 (ON) / 0x00 (OFF)       —
0x02 (Motor)           0x01 (Set Speed)             0xFE (Speed = 254)
```
```cpp
class CommandPipeCallback: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pChar) override {
    uint8_t* data = pChar->getData();
    size_t len = pChar->getLength();

    if (len < 2) return; // Invalid packet

    uint8_t targetDevice = data[0]; // 0x01 = LED, 0x02 = Motor
    uint8_t command      = data[1]; // Action

    switch (targetDevice) {
      case 0x01: // LED
        digitalWrite(LED_PIN, command ? HIGH : LOW);
        break;
      case 0x02: // Motor
        if (len >= 3) analogWrite(MOTOR_PIN, data[2]); // Byte 2 is speed
        break;
    }
  }
};
```
#### Option B: JSON Payload (Human-Readable)        
If memory isn't tight and you use ArduinoJson: send ```{"target":"motor", "speed":180}```.      
#### Summary Recommendation     
For a project with 2–6 distinct controls (LEDs, relays, motors, sensors), use Separate Characteristics.

If you plan to build a general-purpose serial terminal or dynamic command processor, use a Single Command Pipe with binary opcodes.       

## 8. JSON Payload    
Using a JSON Payload over a single BLE characteristic gives you a clean, human-readable command interface. It allows you to expand your hardware commands (adding LEDs, motors, buzzers, etc.) without having to redesign your BLE GATT structure every time.

Prerequisite: Install the ArduinoJson library (v7 or later) via the Arduino IDE Library Manager (Tools > Manage Libraries... -> Search [ArduinoJson](https://arduinojson.org/?utm_source=meta&utm_medium=library.properties)).      

### 1. Server Code (Receiver & Parser)     
The server listens on a single "Command Characteristic" (```PROPERTY_WRITE```). When data arrives, it parses the JSON object and routes actions based on the "target" field.      
```cpp
/*
  ESP32-S3 BLE Server - JSON Command Pipe
  Parses incoming JSON objects to route actions to specific hardware modules.
*/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <ArduinoJson.h> // ArduinoJson v7+

#define SERVICE_UUID      "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define COMMAND_CHAR_UUID "d5875406-fa50-4bfa-982a-152586b0251b"

const int LED_PIN   = 2;  // Onboard LED
const int MOTOR_PIN = 4;  // PWM pin for Motor driver

// Custom Callback to parse incoming JSON
class JsonCommandCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
      String jsonPayload = pCharacteristic->getValue(); // ESP32 Core v3.x returns String

      if (jsonPayload.length() == 0) return;

      Serial.print("📥 Raw JSON Received: ");
      Serial.println(jsonPayload);

      // Allocate JSON document (ArduinoJson v7)
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, jsonPayload);

      if (error) {
        Serial.print("❌ JSON Parsing failed: ");
        Serial.println(error.c_str());
        return;
      }

      // Read the "target" string key
      const char* target = doc["target"];
      if (!target) {
        Serial.println("⚠️ Invalid command: Missing 'target' field");
        return;
      }

      // --- ROUTE COMMANDS BASED ON TARGET ---
      
      // 1. LED Command: {"target":"led", "state":1}
      if (strcmp(target, "led") == 0) {
        int state = doc["state"] | 0; // Default to 0 if missing
        digitalWrite(LED_PIN, state ? HIGH : LOW);
        Serial.printf("💡 LED set to %s\n", state ? "ON" : "OFF");
      } 
      
      // 2. Motor Command: {"target":"motor", "speed":180}
      else if (strcmp(target, "motor") == 0) {
        int speed = doc["speed"] | 0; // PWM value: 0 to 255
        speed = constrain(speed, 0, 255);
        analogWrite(MOTOR_PIN, speed);
        Serial.printf("⚙️ Motor speed set to %d / 255\n", speed);
      } 
      
      // 3. Unknown Target
      else {
        Serial.printf("⚠️ Unknown target system: %s\n", target);
      }
    }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);

  BLEDevice::init("ESP32-S3_JSON_Server");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Command Characteristic with Write permissions
  BLECharacteristic *pCmdChar = pService->createCharacteristic(
                                  COMMAND_CHAR_UUID,
                                  BLECharacteristic::PROPERTY_WRITE
                                );

  pCmdChar->setCallbacks(new JsonCommandCallback());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();

  Serial.println("BLE Server is ready! Send JSON write payloads.");
}

void loop() {
  delay(1000);
}
```

### 2. Client Code Snippet (Transmitter)     
On your ESP32-S3 Client (or smartphone app), build the JSON document, serialize it into a string, and send it over the BLE connection.

Example Helper Functions for Client:       
```cpp
// Assuming pRemoteCmdChar is declared globally at top of sketch

void sendLedCommand(bool turnOn) {
  if (pRemoteCmdChar == nullptr || !pRemoteCmdChar->canWrite()) return;

  JsonDocument doc;
  doc["target"] = "led";
  doc["state"]  = turnOn ? 1 : 0;

  String payload;
  serializeJson(doc, payload);

  // Uses global pointer directly
  pRemoteCmdChar->writeValue(payload.c_str());
  Serial.printf("📤 Sent: %s\n", payload.c_str());
}

void sendMotorCommand(int speed) {
  if (pRemoteCmdChar == nullptr || !pRemoteCmdChar->canWrite()) return;

  JsonDocument doc;
  doc["target"] = "motor";
  doc["speed"]  = speed;

  String payload;
  serializeJson(doc, payload);

  // Uses global pointer directly
  pRemoteCmdChar->writeValue(payload.c_str());
  Serial.printf("📤 Sent: %s\n", payload.c_str());
}
```

Then in your loop(), calling them becomes cleaner:    
```
sendLedCommand(true);     // Turns LED on
sendMotorCommand(180);    // Sets motor speed to 180
```


### 3. Example Payloads & Behavior     
```
Action                  Sent JSON Payload              Server Reaction
Turn LED On             {"target":"led","state":1}     Sets LED_PIN to HIGH.
Turn LED Off            {"target":"led","state":0}     Sets LED_PIN to LOW.
Set Motor Speed (Half)  {"target":"motor","speed":128} Drives PWM on MOTOR_PIN at 50% duty cycle.
Stop Motor              {"target":"motor","speed":0}   Drives PWM on MOTOR_PIN to 0.
```

## 9. Web Bluetooth      
Web Bluetooth (also sometimes referred to as Web BLE) is a technology that allows you to connect and control BLE-enabled devices, like the ESP32, directly from your web browser using JavaScript.      
With Web BLE, you can create web applications that interact with your ESP32 devices via Bluetooth, enabling you to control GPIO pins, exchange data, and manage your devices remotely through a web interface (this means any device that supports a web browser like your computer or smartphone).      
It has been implemented in Chrome, Edge, Opera (Android), and it is supported on Android and Windows. However, it is not yet supported on iOS.      
The ESP32 will act as a BLE Peripheral/BLE Server that advertises its existence. Your computer, smartphone, or tablet will act as a BLE Controller/Client that interacts with the ESP32 device.      
Create an HTML file called anyname.html with the following code (it contains both the HTML to build the web page and Javascript to handle Web Bluetooth).     

```cpp
<!--
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-web-bluetooth/

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
-->

<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Web BLE App</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" type="image/png" href="">
</head>
<body>
  <h1>ESP32 Web BLE Application</h1>
  <button id="connectBleButton">Connect to BLE Device</button>
  <button id="disconnectBleButton">Disconnect BLE Device</button>
  <p>BLE state: <strong><span id="bleState" style="color:#d13a30;">Disconnected</span></strong></p>
  <h2>Fetched Value</h2>
  <p><span id="valueContainer">NaN</span></p>
  <p>Last reading: <span id="timestamp"></span></p>
  <h2>Control GPIO 2</h2>
  <button id="onButton">ON</button>
  <button id="offButton">OFF</button>
  <p>Last value sent: <span id="valueSent"></span></p>
  <p><a href="https://randomnerdtutorials.com/">Created by RandomNerdTutorials.com</a></p>
  <p><a href="https://RandomNerdTutorials.com/esp32-web-bluetooth/">Read the full project here.</a></p>
</body>
<script>
    // DOM Elements
    const connectButton = document.getElementById('connectBleButton');
    const disconnectButton = document.getElementById('disconnectBleButton');
    const onButton = document.getElementById('onButton');
    const offButton = document.getElementById('offButton');
    const retrievedValue = document.getElementById('valueContainer');
    const latestValueSent = document.getElementById('valueSent');
    const bleStateContainer = document.getElementById('bleState');
    const timestampContainer = document.getElementById('timestamp');

    // Define BLE Device Specs (Matched to ESP32-S3 Example 3)
    var deviceName = 'ESP32-S3_Example_3';
    var bleService = '4fafc201-1fb5-459e-8fcc-c5c9c331914b';
    var ledCharacteristic = 'd5875406-fa50-4bfa-982a-152586b0251b';
    var sensorCharacteristic = 'beb5483e-36e1-4688-b7f5-ea07361b26a8';

    // Global Variables
    var bleServer;
    var bleServiceFound;
    var sensorCharacteristicFound;

    // Event Listeners
    connectButton.addEventListener('click', () => {
        if (isWebBluetoothEnabled()) {
            connectToDevice();
        }
    });

    disconnectButton.addEventListener('click', disconnectDevice);
    onButton.addEventListener('click', () => writeOnCharacteristic(1));
    offButton.addEventListener('click', () => writeOnCharacteristic(0));

    function isWebBluetoothEnabled() {
        if (!navigator.bluetooth) {
            console.log("Web Bluetooth API is not available in this browser!");
            bleStateContainer.innerHTML = "Web Bluetooth API is not available in this browser!";
            return false;
        }
        return true;
    }

    function connectToDevice() {
        console.log('Initializing Bluetooth...');
        navigator.bluetooth.requestDevice({
            filters: [{ name: deviceName }],
            optionalServices: [bleService]
        })
        .then(device => {
            console.log('Device Selected:', device.name);
            bleStateContainer.innerHTML = 'Connected to device ' + device.name;
            bleStateContainer.style.color = "#24af37";
            device.addEventListener('gattserverdisconnected', onDisconnected);
            return device.gatt.connect();
        })
        .then(gattServer => {
            bleServer = gattServer;
            return bleServer.getPrimaryService(bleService);
        })
        .then(service => {
            bleServiceFound = service;
            return service.getCharacteristic(sensorCharacteristic);
        })
        .then(characteristic => {
            sensorCharacteristicFound = characteristic;
            characteristic.addEventListener('characteristicvaluechanged', handleCharacteristicChange);
            characteristic.startNotifications();
            return characteristic.readValue();
        })
        .then(value => {
            const decodedValue = new TextDecoder().decode(value);
            retrievedValue.innerHTML = decodedValue;
        })
        .catch(error => {
            console.log('Error: ', error);
        });
    }

    function onDisconnected(event) {
        console.log('Device Disconnected:', event.target.device.name);
        bleStateContainer.innerHTML = "Device disconnected";
        bleStateContainer.style.color = "#d13a30";
    }

    function handleCharacteristicChange(event) {
        const newValueReceived = new TextDecoder().decode(event.target.value);
        retrievedValue.innerHTML = newValueReceived;
        timestampContainer.innerHTML = getDateTime();
    }

    function writeOnCharacteristic(value) {
        if (bleServer && bleServer.connected) {
            bleServiceFound.getCharacteristic(ledCharacteristic)
            .then(characteristic => {
                // Fixed: Sends string "1" or "0" as ASCII encoded bytes
                const data = new TextEncoder().encode(value.toString());
                return characteristic.writeValue(data);
            })
            .then(() => {
                latestValueSent.innerHTML = value;
                console.log("Value written to LED characteristic:", value);
            })
            .catch(error => {
                console.error("Error writing to characteristic: ", error);
            });
        } else {
            window.alert("Bluetooth is not connected. Connect to BLE first!");
        }
    }

    function disconnectDevice() {
        if (bleServer && bleServer.connected) {
            if (sensorCharacteristicFound) {
                sensorCharacteristicFound.stopNotifications()
                    .then(() => bleServer.disconnect())
                    .then(() => {
                        bleStateContainer.innerHTML = "Device Disconnected";
                        bleStateContainer.style.color = "#d13a30";
                    })
                    .catch(error => console.log("An error occurred:", error));
            }
        }
    }

    function getDateTime() {
        var currentdate = new Date();
        var day = ("00" + currentdate.getDate()).slice(-2);
        var month = ("00" + (currentdate.getMonth() + 1)).slice(-2);
        var year = currentdate.getFullYear();
        var hours = ("00" + currentdate.getHours()).slice(-2);
        var minutes = ("00" + currentdate.getMinutes()).slice(-2);
        var seconds = ("00" + currentdate.getSeconds()).slice(-2);
        return day + "/" + month + "/" + year + " at " + hours + ":" + minutes + ":" + seconds;
    }
</script>

</html>
```
Change below variable to match the server settings, in this example we use Example 3 Server code for the ESP32.
```cpp
    var deviceName ='ESP32-S3_Example_3';
    var bleService = '4fafc201-1fb5-459e-8fcc-c5c9c331914b';
    var ledCharacteristic = 'd5875406-fa50-4bfa-982a-152586b0251b';
    var sensorCharacteristic= 'beb5483e-36e1-4688-b7f5-ea07361b26a8';
```

## 10. Connectionless Broadcaster/Observer Pattern (Beacons)       
To achieve ultra-low power consumption on both devices without missing broadcasts, avoid traditional BLE connections. Connection handshakes take time, exchange multiple packets, and waste battery on both ends.

Instead, use a Connectionless Broadcaster/Observer Pattern (Beacons) combined with a Sliding Sync Window.      

### Key Architectural Concepts     
**1. Connectionless Broadcasting**     
   - Server: Wakes up, reads sensors, embeds data directly into the BLE Advertisement Payload (Manufacturer Data), advertises continuously for 2–3 seconds, and goes straight back to deep sleep.
   - Client: Wakes up, scans for the payload, extracts the data, and deep sleeps until the next expected window.          


**2. Overcoming RTC Clock Drift**     
   The ESP32’s internal RTC oscillator drifts by roughly 1% to 3% over time due to temperature fluctuations. Over 30 minutes ($1,800\text{ seconds}$), a 1% drift equals 18 seconds of offset.      
   To ensure you never miss a message:
   - The client wakes up 20 seconds early (Safety Margin).
   - The client starts scanning. Once it catches the advertisement, it immediately recalculates the exact time remaining until the next 30-minute mark and goes back to sleep.
   - Self-Correcting Sync: Because the client resynchronizes its sleep timer on every received packet, clock drift never accumulates across cycles.

### 1. Server Code (Broadcaster)     
The server wakes up every 30 minutes, broadcasts its sensor payload in advertisement data for 3 seconds, and sleeps.
```cpp
/* ESP32-S3 BLE Server - Ultra Low Power Broadcaster */
#include <NimBLEDevice.h>

#define TEST_MODE

#ifdef TEST_MODE
#define SLEEP_DURATION_SEC 10 // 10 seconds
#else
#define SLEEP_DURATION_SEC  1800 // 30 mins interval
#endif
#define BROADCAST_TIME_MS   3000 // Broadcast for 3 seconds

struct SensorPayload {
  float temperature;
  float humidity;
  uint32_t bootCount;
};

RTC_DATA_ATTR uint32_t bootCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000); // Give ESP32-S3 USB Serial time to connect

  bootCount++;
  Serial.println("\n=================================");
  Serial.printf("📡 ESP32-S3 Broadcaster Woke Up! Boot #%u\n", bootCount);

  // 1. Prepare Payload Data
  SensorPayload data;
  data.temperature = 24.5 + (random(-10, 10) / 10.0);
  data.humidity = 60.0 + (random(-20, 20) / 10.0);
  data.bootCount = bootCount;

  // 2. Initialize NimBLE
  NimBLEDevice::init("ESP32_Sensor");
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();

  // 3. Construct Manufacturer Data Payload
  NimBLEAdvertisementData advertData;
  std::string payloadStr((char*)&data, sizeof(data));
  advertData.setManufacturerData("\xFF\xFF" + payloadStr);

  pAdvertising->setAdvertisementData(advertData);

  // 4. Start Broadcasting
  pAdvertising->start();
  Serial.printf("📡 Broadcasting sensor data for %d seconds...\n", BROADCAST_TIME_MS / 1000);
  Serial.printf("Temp: %.2f°C | Hum: %.2f%% | Server Boot: %u\n", data.temperature, data.humidity, data.bootCount);
  delay(BROADCAST_TIME_MS);

  // 5. Stop Advertising and Enter Deep Sleep
  pAdvertising->stop();
  Serial.printf("😴 Going to deep sleep for %d seconds...\n", SLEEP_DURATION_SEC);
  Serial.println("=================================\n");

  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_DURATION_SEC * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {
  // Deep sleep resets the CPU, so loop() is never reached
}

```
### 2. Client Code (Observer with Self-Sync)     
The client uses RTC_DATA_ATTR memory to retain its synchronization state across deep sleep resets.     
```cpp
/* ESP32-S3 BLE Client - Synchronized Low Power Observer */

#include <NimBLEDevice.h>

#define TEST_MODE

#ifdef TEST_MODE

#define CYCLE_INTERVAL_SEC  13     // Sleep + Broadcast time. Target: 30 minutes
#define SAFETY_MARGIN_SEC   2      // Wake 20s early to catch drift
#define SCAN_TIMEOUT_MS     6000   // Timeout if server missed

#else
#define CYCLE_INTERVAL_SEC  1800    // Target: 30 minutes
#define SAFETY_MARGIN_SEC   20      // Wake 20s early to catch drift
#define SCAN_TIMEOUT_MS     45000   // Timeout if server missed
#endif

RTC_DATA_ATTR bool isSynced = false;
bool dataReceived = false;

struct SensorPayload {
  float temperature;
  float humidity;
  uint32_t bootCount;
};

class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
    if (advertisedDevice->haveManufacturerData()) {
      std::string mData = advertisedDevice->getManufacturerData();
      
      if (mData.length() == (2 + sizeof(SensorPayload))) {
        if ((uint8_t)mData[0] == 0xFF && (uint8_t)mData[1] == 0xFF) {
          SensorPayload data;
          memcpy(&data, mData.data() + 2, sizeof(SensorPayload));

          Serial.println("\n✅ Broadcast Captured!");
          Serial.printf("Temp: %.2f°C | Hum: %.2f%% | Server Boot: %u\n", 
                        data.temperature, data.humidity, data.bootCount);

          dataReceived = true;
          NimBLEDevice::getScan()->stop();
        }
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(1000);

  NimBLEDevice::init("ESP32_Observer");
  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setScanCallbacks(new ScanCallbacks());
  pScan->setActiveScan(false);

  if (!isSynced) {
    Serial.println("⚠️ Unsynced! Scanning continuously until first broadcast...");
    pScan->start(0, false); // 0 = Scan indefinitely in background
  } else {
    Serial.println("🔍 Synced wake-up! Scanning for server broadcast...");
    pScan->start(SCAN_TIMEOUT_MS, false); // Pass milliseconds!
  }

  // Active wait loop for both continuous (0) and windowed (6000ms) scans
  while (pScan->isScanning() && !dataReceived) {
    delay(10); // Yield CPU to let NimBLE process background packets
  }

  // Handle Scan Outcome
  if (dataReceived) {
    isSynced = true;
    
    int64_t calculatedSleep = (int64_t)CYCLE_INTERVAL_SEC - (int64_t)SAFETY_MARGIN_SEC;
    if (calculatedSleep < 1) calculatedSleep = 1;
    
    uint64_t sleepTimeSec = (uint64_t)calculatedSleep;
    Serial.printf("😴 Sleeping for %llu seconds until next window...\n\n", sleepTimeSec);
    
    NimBLEDevice::deinit(true);
    esp_sleep_enable_timer_wakeup(sleepTimeSec * 1000000ULL);
    esp_deep_sleep_start();

  } else {
    Serial.println("\n❌ Broadcast missed or timed out! Resetting sync...");
    isSynced = false;
    
    NimBLEDevice::deinit(true);
    esp_sleep_enable_timer_wakeup(2 * 1000000ULL);
    esp_deep_sleep_start();
  }
}

void loop() {}

```

### Timing & Energy Budget Comparison      
```
Metric              Continuous Scanning Client     Synchronized Deep-Sleep Client
Active Scan Time    30 minutes (1800s)             ~20 seconds per 30 minutes
Current Draw        ~40mA continuous               ~40mA for 20s, ~10µA for 1780s
Power Reduction     Baseline                       ~98.8% energy savings
```
### Safety Recovery Strategy    
If a broadcast is missed due to wireless interference:
- The client scan times out after 45 seconds.
- It sets isSynced = false and goes to sleep for 5 seconds.
- On the next boot, it stays awake scanning continuously until it catches the next 30-minute transmission, instantly restoring lock-step synchronization.

### BLE Advertising (Beaconing)     
The low-power example omitted UUIDs because it isn't using GATT (Generic Attribute Profile) at all. Instead, it uses raw connectionless BLE Advertising (Beaconing).     

Here is why that batters for code structure and battery life:     

**1. UUIDs belong to GATT Databases**     
In standard BLE, UUIDs are used to label services and characteristics inside a GATT database (like pServer->createService(SERVICE_UUID)).     
When a client connects, it queries the GATT server using those UUIDs to figure out where to read or write data. In the low-power beacon example, the server never creates a GATT server or establishes a connection, so there is no GATT database to attach a Service UUID to.     
**2. The 31-Byte Advertising Limit**     
BLE advertisement packets have a strict maximum size of 31 bytes.
```
Field Type            Header Overhead    Payload ID Size       Space Remaining for Data
128-bit Service UUID  2 bytes            16 bytes (UUID)       13 bytes left
Manufacturer Data     2 bytes            2 bytes (Company ID)  27 bytes left
```

A standard 128-bit UUID consumes over half of your entire broadcast budget ($18\text{ bytes}$ total). By skipping the 128-bit UUID and using setManufacturerData(), you save $14\text{ bytes}$ of packet space—allowing you to pack multiple sensor values, battery levels, or timestamps into a single transmission.       

**3. How the Client Filters Packets Without a UUID**     
Instead of filtering by a Service UUID, the client filters by the 2-byte Company ID and data length:     
```cpp
// Check if packet contains Manufacturer Data
if (advertisedDevice->haveManufacturerData()) {
  std::string mData = advertisedDevice->getManufacturerData();
  
  // Filter by matching Company ID (\xFF\xFF) + expected struct size
  if (mData.length() == (2 + sizeof(SensorPayload)) && mData[0] == 0xFF && mData[1] == 0xFF) {
    // Valid broadcast found! Decode payload...
  }
}
```
*Note: 0xFFFF is the standard Bluetooth SIG ID designated for testing/development. For commercial products, companies register a unique 2-byte ID with the Bluetooth SIG (e.g., 0x004C for Apple, 0x00E0 for Google).*

## 11. Other Applications     

Beyond simple sensor-to-client data streaming, the ESP32-S3's BLE stack (especially when running lightweight NimBLE) unlocks several high-utility, low-power applications. Because BLE operates without needing a Wi-Fi router or local network, it bridges the physical hardware world directly to smartphones, PCs, and smart home hubs.

Here are some of the most practical and interesting BLE applications for the ESP32 platform:     
**1. BLE-MIDI Wireless Controllers**     
BLE supports native MIDI over Bluetooth (ESP32-BLE-MIDI), meaning Windows, macOS, iOS, and Android recognize the ESP32 as a class-compliant wireless instrument without needing custom drivers or USB dongles.
- Expression Controllers: Connect potentiometers, breath sensors, or IMUs (gyroscope/accelerometer) to stream low-latency pitch bend, CC parameters, or modulation to DAWs like Ableton Live or Logic Pro.
- Custom Foot Switches: Wireless pedalboards for page-turning digital sheet music (for tablets) or controlling guitar plugin suites.     

**2. Human Interface Devices (BLE HID)**     
The ESP32 can masquerade as a Bluetooth keyboard, mouse, or gamepad (ESP32-BLE-Keyboard).      
- Custom Macro Keypads: Hardware knobs (rotary encoders) and switches mapped to shortcut macros for photo/video editing apps (DaVinci Resolve, Lightroom).
- Assistive Input Devices: Building custom single-switch or gesture-based input accessories for accessibility.
- Air Mouse / Presenter Remotes: Combine an MPU6050 6-axis gyro with BLE mouse emulation for gesture-based slide presentation or HTPC navigation.

**3. BLE-to-MQTT / Home Assistant Gateways**     
In a smart home setup, Wi-Fi can be power-hungry for battery nodes. You can run the ESP32 as an active BLE Scanner Gateway (e.g., using ESPHome or custom NimBLE code).     
- Passive Beacon Sniffing: Listen to raw BLE advertisement packets broadcast by cheap off-the-shelf hardware (Govee temperature probes, Xiaomi sensors, BLE plant monitors, tile trackers) and forward the parsed payload to your local network via Wi-Fi/MQTT.
- Room-Level Presence Tracking: Track BLE RSSI (signal strength) from smartwatches or key fobs to trigger localized room lighting or HVAC adjustments as you walk from room to room.

**4. Touchless Smart Access & Geofencing**      
Using BLE RSSI and advertising callbacks, the ESP32 can estimate proximity without needing a manual button press.     
- Proximity Unlocking: Trigger a relay/solenoid for garage doors or workshop power tools when your phone's unique BLE beacon gets within 1–2 meters.
- Anti-Loss / Asset Tracking: Trigger an alarm or log an event when an ESP32 tag moves out of range of a central hub (or vice versa).

**5. BLE Mesh Networks**
Unlike point-to-point client/server BLE, ESP-BLE-MESH enables thousands of nodes to communicate in a many-to-many topology without a Wi-Fi router.     
- Multi-Room Lighting Systems: Relay control messages across a large area where individual nodes are out of range of the main controller.
- Distributed Sensor Networks: Gather environmental data across large workshops, gardens, or greenhouses by hopping messages node-to-node back to a central logging node.

**6. Secure Device Provisioning**      
Instead of forcing a device into AP mode (where users have to disconnect their phone from Wi-Fi to log into a 192.168.4.1 captive portal), use BLE for initial setup:      
- Use a mobile app to send Wi-Fi SSIDs, passwords, API keys, or calibration parameters via BLE characteristics.
- Once provisioned, the ESP32 saves credentials to NVS (Non-Volatile Storage), shuts down the BLE radio to conserve power, and connects to Wi-Fi.    

## Multi-node relay         
Beacons mode code can not be used with the exact code as-is, because of one fundamental physics constraint: a node in deep sleep cannot hear radio broadcasts.   

If a relay node is sleeping when a sensor node transmits, that broadcast is lost in the air.     

However, you can adapt this architecture into a Low-Power Mesh/Relay Network using one of two strategies:       
### Strategy 1: Network-Wide Synchronized Wake Windows (All Nodes Sleep)      
If all nodes must run on battery, every node in the mesh must share the exact same 30-minute sleep schedule so they wake up together in a synchronized 5-second window.      
```
 Time: 00:00:00                00:00:02                00:00:04                            00:00:05
 [ Leaf Node 1 ] -----------(Broadcasts)---------> [ Relay Node 2 ]
                                                   (Appends data &
                                                    Re-broadcasts) ----(Broadcasts)----> [ Central Node ]
 [ All Nodes Sleep for 30 Minutes ] ------------------------------------------------------------------->
```
How to Modify the Payload for Multi-Hop Relaying      
To make the code relay messages, you expand the SensorPayload structure so a single advertisement packet can carry readings from multiple hops or identify the originator:     
```cpp
struct NodeData {
  uint8_t nodeId;
  float temperature;
  float humidity;
};

// Fixed array inside the manufacturer payload
struct MeshPayload {
  uint8_t hopCount;           // Number of times relayed
  uint8_t totalReadings;      // How many node data blocks are packed
  NodeData readings[3];       // Supports up to 3 hops within the 31-byte BLE limit
};
```
Relay Node Logic (during the awake window):     
1. Listen: Wake up and listen for $2\text{ seconds}$ for incoming packets from upstream leaf nodes.
2. Append: If Node 2 receives a payload from Node 1, it appends its own NodeData to the array and increments hopCount.
3. Re-broadcast: Node 2 advertises the updated combined payload for $2\text{ seconds}$ so the Central Node (or next relay) can pick it up.
4. Sleep: All nodes return to deep sleep for 30 minutes.     

### Strategy 2: ESP-BLE-MESH (Standard SIG Mesh with LPNs)      
If you want a standard, scalable mesh (dozens of nodes over large distances), use the official Bluetooth SIG Mesh standard built into the ESP32 framework (ESP-BLE-MESH).     
Bluetooth Mesh solves power consumption using Low Power Nodes (LPN) and Friend Nodes:      
```
Node Type             Power Mode                 Role
Sensor Node (LPN)     Deep Sleep (~10µA)         Wakes up, sends sensor reading directly to its assigned "Friend",
                                                 and goes right back to sleep.
Relay Node (Friend)   Mains Powered (Always On)  Stays awake 24/7. Buffers messages for sleeping LPNs and relays
                                                 messages hop-by-hop back to the Central Node.
```
**Comparison: Custom Sync Relay vs. Official BLE Mesh**     
```
Feature                   Synchronized Custom Beacons                         Official ESP-BLE-MESH  
Relay Power Requirements  Battery Powered (Deep Sleep allowed)                Mains Powered / Always Awake
Implementation Complexity Simple extension of our existing code               Moderate to High (Requires Mesh Provisioning)
Max Network Size          Small (3–5 nodes per branch)                        Large (Up to 32,767 nodes)
Payload Size              Restricted to $31\text{ bytes}$ advertising packet  Segmented packets (up to 384 bytes)
```

### Multi-node relay: Strategy 1 - Slot-Based Time-Division Multiplexing (TDM)      

To build a multi-hop relay network with battery-powered nodes, we use Slot-Based Time-Division Multiplexing (TDM).     
Because deep-sleeping nodes cannot hear radio signals, each node in the chain is assigned a specific time slot within a shared $20\text{-second}$ test cycle (which you can easily scale up to 30 minutes in production).      
Network Slot Architecture     
```
Time (s):  0s         3s         6s                     20s
           |----------|----------|-----------------------|
Node 1     [Broadcast]-----------> (Deep Sleep 17s) ----->
(Leaf)     (Payload: N1)

Node 2                [Scan &    [Broadcast]-------------> (Deep Sleep 14s) ----->
(Relay)               Receive]   (Payload: N1 + N2)

Node 3                           [Scan & Receive] --------> (Process / Push to Cloud)
(Sink)                           (Payload: N1 + N2)
```
**1. Slot 1 ($0\text{s} - 3\text{s}$)**: Node 1 (Leaf) wakes up, packs its sensor reading into a BLE advertisement, and broadcasts for $3\text{ seconds}$.
**2. Slot 2 ($2\text{s} - 6\text{s}$)**: Node 2 (Relay) wakes up slightly early, scans for Node 1, appends its own reading to the payload array, and re-broadcasts the combined data for $3\text{ seconds}$.
**3. Slot 3 ($5\text{s} - 9\text{s}$)**: Node 3 (Central Sink) scans, receives the full chain payload (Node 1 + Node 2 data), and processes or uploads it via Wi-Fi/MQTT.

The $31\text{-Byte}$ Multi-Hop Payload Structure     
We pack multiple node readings into a single raw advertising packet without exceeding BLE's $31\text{-byte}$ limit:      
```cpp
#define MAX_CHAIN_NODES 3

struct NodeReading {
  uint8_t nodeId;      // 1 byte
  int16_t tempC100;    // 2 bytes (Temperature * 100)
  uint16_t hum100;     // 2 bytes (Humidity * 100)
  uint16_t bootCount;  // 2 bytes
};                     // Total = 7 bytes per node

struct MeshPayload {
  uint8_t readingCount;                   // 1 byte (Number of valid readings)
  NodeReading readings[MAX_CHAIN_NODES];  // 3 * 7 = 21 bytes
};                                        // Payload Total = 22 bytes
```
*Total Manufacturer Data = 2 bytes Header (\xFF\xFF) + 22 bytes Payload = 24 bytes (Well within the 31-byte BLE limit).*     

#### Unified Multi-Hop Code for ESP32-S3          
This single codebase supports all three node roles. Set NODE_ROLE at the top of the file before uploading to each board.      
```cpp
/*
  ESP32-S3 NimBLE v2.x Synchronized Multi-Hop Relay
  Configurable via NODE_ROLE macro:
    - ROLE_LEAF    (Node 1: Battery Sensor)
    - ROLE_RELAY   (Node 2: Battery Relay)
    - ROLE_CENTRAL (Node 3: Mains Powered or Synced Sink)
*/

#include <NimBLEDevice.h>


// --- ROLE CONFIGURATION ---
#define ROLE_LEAF 1
#define ROLE_RELAY 2
#define ROLE_CENTRAL 3

#define NODE_ROLE ROLE_CENTRAL  // 👈 CHANGE THIS FOR EACH BOARD (ROLE_LEAF, ROLE_RELAY, or ROLE_CENTRAL)
#define THIS_NODE_ID 3          // Node 1 for Leaf, Node 2 for Relay, Node 3 for Central

// --- TIMING CONFIGURATION (20-Second Test Cycle) ---
#define CYCLE_INTERVAL_SEC 20   // Total cycle period (Set to 1803 for 30-min prod)
#define BROADCAST_TIME_MS 3000  // Duration each node advertises (ms)
#define SCAN_TIMEOUT_MS 5000    // Duration relay/sink scans for upstream data (ms)
#define SAFETY_MARGIN_SEC 2     // Early wake-up window (seconds)

#define MAX_CHAIN_NODES 3

// Data Structures
struct NodeReading {
  uint8_t nodeId;
  int16_t tempC100;
  uint16_t hum100;
  uint16_t bootCount;
};

struct MeshPayload {
  uint8_t readingCount;
  NodeReading readings[MAX_CHAIN_NODES];
};

RTC_DATA_ATTR bool isSynced = false;
RTC_DATA_ATTR uint32_t localBootCount = 0;

MeshPayload currentPayload;
bool dataReceived = false;

// Scan Callbacks
class MeshScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
    if (advertisedDevice->haveManufacturerData()) {
      std::string mData = advertisedDevice->getManufacturerData();

      if (mData.length() == (2 + sizeof(MeshPayload))) {
        if ((uint8_t)mData[0] == 0xFF && (uint8_t)mData[1] == 0xFF) {
          MeshPayload tempPayload;
          memcpy(&tempPayload, mData.data() + 2, sizeof(MeshPayload));

          // 🎯 ONLY capture if readingCount >= 2 (Node 2 has appended its data)
          if (tempPayload.readingCount >= 2) {
            memcpy(&currentPayload, &tempPayload, sizeof(MeshPayload));
            dataReceived = true;
            NimBLEDevice::getScan()->stop();  // Stop scanner ONLY when Node 2 data arrives
          }
        }
      }
    }
  }
};

void appendLocalSensorData(MeshPayload& payload) {
  if (payload.readingCount >= MAX_CHAIN_NODES) return;

  // Simulated Sensor Data
  NodeReading myData;
  myData.nodeId = THIS_NODE_ID;
  myData.tempC100 = (int16_t)((23.5 + (THIS_NODE_ID * 0.5)) * 100);
  myData.hum100 = (uint16_t)((55.0 + (THIS_NODE_ID * 2.0)) * 100);
  myData.bootCount = localBootCount;

  payload.readings[payload.readingCount] = myData;
  payload.readingCount++;
}

void broadcastPayload(const MeshPayload& payload) {
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  NimBLEAdvertisementData advertData;

  std::string payloadStr((char*)&payload, sizeof(payload));
  advertData.setManufacturerData("\xFF\xFF" + payloadStr);

  pAdvertising->setAdvertisementData(advertData);
  pAdvertising->start();

  Serial.printf("📡 Node %d Broadcasting %d reading(s) for %d ms...\n",
                THIS_NODE_ID, payload.readingCount, BROADCAST_TIME_MS);

  delay(BROADCAST_TIME_MS);
  pAdvertising->stop();
}

void printChainData(const MeshPayload& payload) {
  Serial.println("\n============================================");
  Serial.printf("📥 CENTRAL SINK RECEIVED MULTI-HOP DATA (%d nodes):\n", payload.readingCount);
  for (uint8_t i = 0; i < payload.readingCount; i++) {
    Serial.printf("   ├─ Node ID %d: Temp = %.2f°C | Hum = %.2f%% | Boot = %u\n",
                  payload.readings[i].nodeId,
                  payload.readings[i].tempC100 / 100.0,
                  payload.readings[i].hum100 / 100.0,
                  payload.readings[i].bootCount);
  }
  Serial.println("============================================\n");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  localBootCount++;

  Serial.printf("\n🚀 Node %d Started (Role: %s) | Boot #%u\n",
                THIS_NODE_ID,
                NODE_ROLE == ROLE_LEAF ? "LEAF" : (NODE_ROLE == ROLE_RELAY ? "RELAY" : "CENTRAL SINK"),
                localBootCount);

  NimBLEDevice::init("");
  memset(&currentPayload, 0, sizeof(MeshPayload));

  // ----------------------------------------------------
  // ROLE 1: LEAF NODE (Originator)
  // ----------------------------------------------------
  if (NODE_ROLE == ROLE_LEAF) {
    appendLocalSensorData(currentPayload);
    broadcastPayload(currentPayload);

    // Display Payload
    printChainData(currentPayload);

    uint64_t sleepSec = CYCLE_INTERVAL_SEC;
    Serial.printf("😴 Leaf sleeping for %llu seconds...\n\n", sleepSec);

    NimBLEDevice::deinit(true);
    esp_sleep_enable_timer_wakeup(sleepSec * 1000000ULL);
    esp_deep_sleep_start();
  }

  // ----------------------------------------------------
  // ROLE 2: RELAY NODE (Intermediate)
  // ----------------------------------------------------
  else if (NODE_ROLE == ROLE_RELAY) {
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(new MeshScanCallbacks());
    pScan->setActiveScan(false);

    if (!isSynced) {
      Serial.println("⚠️ Relay Unsynced! Continuous scan for Leaf packet...");
      pScan->start(0, false);
    } else {
      Serial.println("🔍 Relay Synced! Scanning window for Leaf packet...");
      pScan->start(SCAN_TIMEOUT_MS, false);
    }

    // Yield CPU while NimBLE processes incoming packets
    while (pScan->isScanning() && !dataReceived) {
      delay(10);
    }

    if (dataReceived) {
      isSynced = true;
      Serial.println("✅ Upstream packet captured! Appending local data...");

      appendLocalSensorData(currentPayload);
      broadcastPayload(currentPayload);
      // Display Payload
      printChainData(currentPayload);

      int64_t calculatedSleep = (int64_t)CYCLE_INTERVAL_SEC - (int64_t)SAFETY_MARGIN_SEC;
      if (calculatedSleep < 1) calculatedSleep = 1;

      Serial.printf("😴 Relay sleeping for %lld seconds...\n\n", calculatedSleep);

      NimBLEDevice::deinit(true);
      esp_sleep_enable_timer_wakeup((uint64_t)calculatedSleep * 1000000ULL);
      esp_deep_sleep_start();
    } else {
      Serial.println("❌ Upstream scan missed/timed out! Resetting sync...");
      isSynced = false;

      NimBLEDevice::deinit(true);
      esp_sleep_enable_timer_wakeup(2 * 1000000ULL);  // Retry sync in 2s
      esp_deep_sleep_start();
    }
  }

  // ----------------------------------------------------
  // ROLE 3: CENTRAL SINK (Gateway)
  // ----------------------------------------------------
  else if (NODE_ROLE == ROLE_CENTRAL) {
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(new MeshScanCallbacks());
    pScan->setActiveScan(false);

    Serial.println("📡 Central Hub scanning for incoming Node 2 chain payloads...");
    pScan->start(0, false);  // Continuous scanning mode

    uint32_t lastProcessedBoot = 0;

    while (true) {
      if (dataReceived) {
        // Retrieve Node 2's boot count (index 1 = Node 2)
        uint16_t node2Boot = currentPayload.readings[1].bootCount;

        // Deduplicate: Only process if this is a new broadcast cycle
        if (node2Boot != lastProcessedBoot) {
          printChainData(currentPayload);
          lastProcessedBoot = node2Boot;
        }

        // Wait out the remainder of Node 2's 3-second broadcast window
        delay(BROADCAST_TIME_MS);

        // Reset state and resume scanning for the next cycle
        dataReceived = false;
        memset(&currentPayload, 0, sizeof(MeshPayload));
        pScan->start(0, false);
      }
      delay(100);
    }
  }
}

void loop() {}
```
Testing & Deployment Guide      
- Board 1 (Leaf): Set NODE_ROLE to ROLE_LEAF and THIS_NODE_ID to 1. Flash the board.
- Board 2 (Relay): Set NODE_ROLE to ROLE_RELAY and THIS_NODE_ID to 2. Flash the board.
- Board 3 (Central Sink): Set NODE_ROLE to ROLE_CENTRAL and THIS_NODE_ID to 3. Flash the board.

Central Hub Output:     
When the transmission reaches the Central Hub, you will see all accumulated telemetry printed in a single payload:     
```
============================================
📥 CENTRAL SINK RECEIVED MULTI-HOP DATA (2 nodes):
   ├─ Node ID 1: Temp = 24.00°C | Hum = 57.00% | Boot = 14
   ├─ Node ID 2: Temp = 24.50°C | Hum = 59.00% | Boot = 14
============================================
```
### Multi-node relay: Strategy 2 - ESP-BLE-MESH      

Unlike custom broadcast hacks, official Bluetooth SIG Mesh includes a native feature for battery-powered devices called Friendship.     
In an ESP-BLE-MESH network, a Low Power Node (LPN) pairs with a mains-powered Friend Node. The Friend Node stays awake $100\%$ of the time to act as a "mailbox," storing all incoming mesh messages intended for the LPN. The LPN spends most of its time in deep sleep, waking up briefly to query its Friend, download queued messages, transmit its sensor readings, and go back to sleep.     


⚠️ Framework Requirement: Official ESP-BLE-MESH with full LPN/Friendship feature support requires ESP-IDF (Espressif IoT Development Framework v4.4+ or v5.x) or PlatformIO configured with the ESP-IDF framework. Standard Arduino BLE libraries only implement GATT/GAP, not the full Bluetooth Mesh protocol stack.      


#### 1. How Friendship Works (The Handshake & Poll)   

```
Friend Node (Always Awake)                       Low Power Node (LPN)
       │                                                   │
       │<───────────── Friend Request ─────────────────────│ (LPN wakes up)
       │────────────── Friend Offer ──────────────────────>│
       │<───────────── Friend Poll ────────────────────────│
       │────────────── Friend Update (Friendship EST) ────>│
       │                                                   │
   [ Stores messages                                   [ Goes to Deep
    in Mailbox Queue ]                                    Sleep ]
       │                                                   │
       │<───────────── Friend Poll ────────────────────────│ (Wakes up every 30m)
       │────────────── Deliver Queued Messages ───────────>│
       │                                                   │
```

  - **Establishment**: The LPN broadcasts a Friend Request packet with its required queue size and polling parameters. Nearby Friend Nodes respond with a Friend Offer. The LPN selects the best offer and establishes a Friendship.
  - **Buffering**: When other nodes in the mesh send data to the LPN, the Friend Node intercepts and buffers the packets in its local RAM.
  - **Polling**: When the LPN wakes up from sleep, it sends a Friend Poll. The Friend Node responds immediately with any stored messages.
  - **Sleep**: If no messages remain in the queue, the LPN returns to deep sleep.     



#### 2. Setting Up the Friend Node (Mains Powered)     
The Friend Node needs sufficient RAM allocated to buffer messages for one or more LPNs.     

```sdkconfig``` Settings for Friend Node     
In your ESP-IDF project, enable Mesh and the Friend feature via ```menuconfig```:      
```
Component config --->
  ESP-BLE-MESH Support --->
    [*] Enable BLE Mesh node
    [*] Enable BLE Mesh Friend feature
        (10) Maximum number of LPNs supported
        (16) Subscriptions list size per LPN
        (8)  Queue size per LPN
```
Friend Node Code Structure (ESP-IDF)      
```cpp
#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_networking_api.h"

// Define elements and features
static esp_ble_mesh_element_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, ESP_BLE_MESH_MODEL_NONE, ESP_BLE_MESH_MODEL_NONE),
};

static esp_ble_mesh_comp_t composition = {
    .cid = 0x02E5, // Espressif Company ID
    .elements = elements,
    .element_count = ARRAY_SIZE(elements),
};

// Callback to handle Friendship events on the Friend Node
static void ble_mesh_friend_cb(esp_ble_mesh_friend_cb_event_t event,
                               esp_ble_mesh_friend_cb_param_t *param) {
    switch (event) {
    case ESP_BLE_MESH_FRIEND_ESTABLISH_EVT:
        ESP_LOGI("FRIEND", "Friendship established with LPN address: 0x%04x", 
                 param->friend_establish.lpn_addr);
        break;
    case ESP_BLE_MESH_FRIEND_TERMINATE_EVT:
        ESP_LOGW("FRIEND", "Friendship terminated with LPN address: 0x%04x", 
                 param->friend_terminate.lpn_addr);
        break;
    default:
        break;
    }
}

void app_main(void) {
    // 1. Initialize NVS and Bluetooth Controller
    // ... (Standard ESP-IDF Bluetooth init) ...

    // 2. Register Friend Callback
    esp_ble_mesh_register_friend_callback(ble_mesh_friend_cb);

    // 3. Enable BLE Mesh Stack with FRIEND feature flag
    esp_err_t err = esp_ble_mesh_init(&provision, &composition);
    if (err == ESP_OK) {
        // Friend feature is active and listening for LPN requests
        ESP_LOGI("MESH", "Friend Node Initialized!");
    }
}
```


#### 3. Setting Up the Low Power Node (LPN)        
The LPN configures its timing constraints (how fast it expects replies and how long it sleeps between polls) and initiates the Friendship request.

```sdkconfig``` Settings for LPN     
```
Component config --->
  ESP-BLE-MESH Support --->
    [*] Enable BLE Mesh node
    [*] Enable BLE Mesh Low Power Node feature
        (1) Auto-enable LPN feature after provisioning
        (100) Min receive delay (ms)
        (1000) Poll timeout (100ms units) -> 100 seconds
```
LPN Code Structure (ESP-IDF)     
```cpp
#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_lpn_api.h"

// Configurable Friendship Request Parameters
static esp_ble_mesh_lpn_param_t lpn_param = {
    .min_queue_num = 2,      // Min messages Friend must buffer for us
    .poll_timeout  = 18000,  // Max time between polls (18000 * 100ms = 30 minutes)
    .rx_delay      = 100,    // Time (ms) LPN waits before opening receiver after polling
    .poll_interval = 3000,   // Default polling interval during active setup (ms)
};

// Callback to handle LPN events
static void ble_mesh_lpn_cb(esp_ble_mesh_lpn_cb_event_t event,
                            esp_ble_mesh_lpn_cb_param_t *param) {
    switch (event) {
    case ESP_BLE_MESH_LPN_ESTABLISH_COMP_EVT:
        if (param->lpn_establish_comp.err_code == 0) {
            ESP_LOGI("LPN", "Friendship established with Friend: 0x%04x",
                     param->lpn_establish_comp.friend_addr);
            
            // Friendship is ready! Safe to enter deep sleep now.
        } else {
            ESP_LOGE("LPN", "Friendship establishment failed!");
        }
        break;
        
    case ESP_BLE_MESH_LPN_TERMINATE_COMP_EVT:
        ESP_LOGW("LPN", "Friendship terminated.");
        break;

    default:
        break;
    }
}

void setup_lpn(void) {
    // 1. Register LPN callback
    esp_ble_mesh_register_lpn_callback(ble_mesh_lpn_cb);

    // 2. Enable LPN feature & search for nearby Friend Nodes
    esp_ble_mesh_lpn_enable();
}

// Function called after waking up from deep sleep
void on_lpn_wakeup(void) {
    // Poll the Friend for buffered messages
    esp_ble_mesh_lpn_poll();
}
```


#### 4. Key Parameters Comparison     
```
Parameter       Recommended Value                What It Controls
poll_timeout    18000(1,800 seconds = 30 min)    Max time the LPN can sleep before the Friend considers the friendship dead and drops the queue.
rx_delay        10 ms - 50 ms                    Delay between LPN transmitting a Poll and opening its receiver window.
                                                 Saves power by keeping the radio off during processing.
min_queue_num   2 - 8 packets                    Minimum queue depth the Friend Node must promise to allocate for this LPN.
```


#### 5. Provisioning the Mesh       
Before an LPN and Friend Node can communicate, both devices must be provisioned into the same Bluetooth Mesh network:
  - Use an app like nRF Mesh (iOS/Android) or an ESP32 configured as a Mesh Provisioner.
  - Provision the Friend Node first so it is alive on the mesh.
  - Provision the LPN.
  - Assign matching Publish/Subscribe addresses (e.g., bind both to Group Address 0xC000).
  - Call ```esp_ble_mesh_lpn_enable()``` on the LPN. The LPN will automatically discover the Friend Node, negotiate friendship, and start duty-cycling sleep.

Refer to [Espressif ESP-BLE-MESH](https://www.espressif.com/en/products/sdks/esp-idf/esp-ble-mesh) for more details.

## 12. Reference      

https://randomnerdtutorials.com/esp32-bluetooth-low-energy-ble-arduino-ide/

https://randomnerdtutorials.com/esp32-ble-server-client/

https://randomnerdtutorials.com/esp32-web-bluetooth/

https://www.oceanlabz.in/esp32-bluetooth-low-energy-ble/

https://www.oceanlabz.in/esp32-esp-now/

https://www.luisllamas.es/esp32-bluetooth-low-energy-ble/

https://randomnerdtutorials.com/esp32-bluetooth-low-energy-ble-arduino-ide/

https://deepbluembedded.com/esp32-bluetooth-classic-with-arduino-complete-guide/

https://www.bluetooth.com/learn-about-bluetooth/tech-overview/
