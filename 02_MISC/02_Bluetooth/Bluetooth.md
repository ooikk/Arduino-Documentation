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

**Option A: Binary Protocol (Fastest & Lightest)**

Send fixed binary byte arrays rather than ASCII strings:
