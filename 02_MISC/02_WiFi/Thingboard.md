# Connection Options for ESP32-S3 with ThingsBoard

[ThingsBoard](https://thingsboard.io/) supports several architecture patterns for connecting an ESP32-S3 board to the platform.     

<img width="895" height="612" alt="image" src="https://github.com/user-attachments/assets/1c225606-9762-432b-be69-aee715f58803" />

```text
+------------------------------------------------------------------------------------+
|                           CONNECTION ARCHITECTURES                                 |
+------------------------------------------------------------------------------------+
|                                                                                    |
|  [ Direct Connection ]                                                             |
|  ESP32-S3 --- Wi-Fi: MQTT / HTTP / CoAP ---> ThingsBoard Cloud (PaaS)              |
|                                                                                    |
|  [ IoT Gateway Connection: Local Sensor Network ]                                  |
|  Sensor Nodes --- BLE / ESP-NOW ---> ESP32-S3 Gateway ---> ThingsBoard Cloud       |
|                                                                                    |
|  [ External Gateway Connection ]                                                   |
|  ESP32-S3 Node --- BLE / UART / RS485 ---> Raspberry Pi Gateway ---> ThingsBoard   |
|                                                                                    |
+------------------------------------------------------------------------------------+
```


https://thingsboard.io/docs/paas/user-guide/connectivity-guide/?

## 1. Direct Connection

### Most Common Architecture

The ESP32-S3 connects directly to ThingsBoard Cloud through standard Wi-Fi using one of three transport protocols.

### MQTT and MQTTS

```text
MQTT:  Port 1883
MQTTS: Port 8883
```

MQTT is generally the recommended protocol for two-way communication.

It supports:

- Real-time telemetry publishing.
- Server-side attribute updates.
- Client-side attribute requests.
- Remote Procedure Calls, or RPC.
- Commands such as toggling a relay.
- Over-the-Air firmware updates.
- Bi-directional device communication.

Authentication is usually handled through an access token supplied as the MQTT username.

### HTTP and HTTPS

```text
HTTP:  Port 80
HTTPS: Port 443
```

ThingsBoard provides a REST endpoint for telemetry posting:

```text
https://thingsboard.cloud/api/v1/<ACCESS_TOKEN>/telemetry
```

HTTP is useful for low-power battery applications where the ESP32-S3:

1. Wakes from deep sleep.
2. Reads sensors.
3. Sends data through an HTTP request.
4. Returns to deep sleep.

### CoAP and CoAPS

```text
CoAP:  UDP port 5683
CoAPS: UDP port 5684
```

CoAP is a lightweight UDP-based protocol intended for constrained devices and networks where TCP overhead should be minimized.

## 2. IoT Gateway Connection

<img width="903" height="87" alt="image" src="https://github.com/user-attachments/assets/1a7a6fde-04ce-4b03-acc1-2ad138797bde" />


If a deployment contains multiple low-power sensor nodes or legacy protocols, the ESP32-S3 can be used inside a gateway architecture.


### ESP32-S3 Behind an External Gateway

The ESP32-S3 can also operate as a peripheral sensor node.

```text
ESP32-S3
    │
    ├── BLE
    ├── UART
    └── RS485
    │
    ▼
Raspberry Pi Gateway
    │
    ▼
ThingsBoard Cloud
```

In this arrangement:

- The ESP32-S3 sends raw serial or BLE data to the gateway.
- The Raspberry Pi runs ThingsBoard IoT Gateway software.
- The gateway decodes the payload.
- The gateway forwards the data to ThingsBoard Cloud.



### ESP32-S3 as an Edge Gateway

The ESP32-S3 can operate as a gateway that:

- Collects telemetry from sub-nodes.
- Receives BLE sensor data.
- Receives ESP-NOW data.
- Reads RS485 or Modbus devices.
- Receives LoRa packets.
- Relays all collected telemetry to ThingsBoard.

The ThingsBoard Gateway MQTT telemetry topic is:

```text
v1/gateway/telemetry
```

This allows one ESP32-S3 gateway to register and publish telemetry for multiple downstream devices.



### Using an ESP32 as a ThingsBoard IoT Gateway

You can use an ESP32 as an IoT Gateway instead of a Raspberry Pi.

However, you cannot run the official Python-based ThingsBoard IoT Gateway software directly on an ESP32 because it requires a full Linux operating system.

Instead, you can program the ESP32 as a custom gateway using the ThingsBoard Gateway MQTT API.

#### How an ESP32 Gateway Works

Instead of every sensor node connecting directly to Wi-Fi and ThingsBoard, lower-power sub-nodes communicate locally with the ESP32 Gateway.

The ESP32 Gateway then collects, translates, and forwards the telemetry to ThingsBoard through one Wi-Fi connection.

```text
+-----------------------------------------------------------------------------------+
|                           ESP32 GATEWAY ARCHITECTURE                              |
+-----------------------------------------------------------------------------------+
|                                                                                   |
|  [ Peripheral Sub-Nodes ]          [ Local Protocol ]        [ ESP32 Gateway ]    |
|                                                                                   |
|  - ESP32 / ESP8266 Nodes  -------> ESP-NOW ------------\                          |
|  - BLE Temperature Beacons ------> Bluetooth LE ---------> ESP32 Gateway          |
|  - Industrial Meters       -------> RS485 / Modbus ----/   Master Node            |
|                                                                  |                |
|                                                            Wi-Fi / MQTT           |
|                                                                  |                |
|                                                                  v                |
|                                                         ThingsBoard Cloud         |
|                                                        Auto-creates separate      |
|                                                        entities for each node     |
+-----------------------------------------------------------------------------------+
```

#### The ThingsBoard Gateway MQTT API

ThingsBoard provides a specialized Gateway MQTT API topic:

```text
v1/gateway/telemetry
```

When an ESP32 publishes data to this topic using a Gateway Access Token, it can specify the names of downstream devices.

ThingsBoard can then automatically create and update separate device entities for those downstream nodes.

#### Example Gateway Telemetry Payload

The ESP32 Gateway can publish a payload such as:

```json
{
  "Sensor_Node_01": [
    {
      "values": {
        "temperature": 25.4,
        "humidity": 62.1
      }
    }
  ],
  "Sensor_Node_02": [
    {
      "values": {
        "temperature": 21.8,
        "battery": 88
      }
    }
  ]
}
```

When ThingsBoard receives this payload from the ESP32 Gateway:

1. It creates `Sensor_Node_01` if it does not already exist.
2. It creates `Sensor_Node_02` if it does not already exist.
3. It assigns the relevant telemetry to each device.
4. It displays each downstream node as an individual device entity in ThingsBoard.

#### Common ESP32 Gateway Topologies

##### ESP-NOW to Wi-Fi Gateway

This is a common option for Espressif-based projects.

###### Sub-Nodes

The sub-nodes may be:

```text
ESP32 boards
ESP8266 boards
```

They use ESP-NOW, a fast connectionless 2.4 GHz protocol that does not require a Wi-Fi router.

Sub-nodes can:

1. Wake from deep sleep.
2. Send a packet in less than approximately 10 ms.
3. Return to deep sleep.

###### Gateway

The gateway ESP32 connects to:

```text
ESP-NOW local sensor network
Wi-Fi router
ThingsBoard Cloud
```

Architecture:

```text
ESP-NOW Sub-Nodes
    │
    ▼
ESP32 Gateway
    │
    ▼
Wi-Fi Router
    │
    ▼
ThingsBoard Cloud
```

##### BLE to Wi-Fi Gateway

###### Sub-Nodes

The sub-nodes may include:

```text
Bluetooth LE beacons
BLE sensor tags
BLE thermometer and hygrometer devices
```

Example devices include BLE temperature and humidity sensors.

###### Gateway

The ESP32 Gateway:

1. Uses its built-in Bluetooth hardware to scan BLE advertisements.
2. Decodes sensor values from received packets.
3. Formats the values as JSON.
4. Publishes the telemetry to ThingsBoard through Wi-Fi.

Architecture:

```text
BLE Sensors
    │
    ▼
ESP32 Gateway
    │
    ▼
ThingsBoard Cloud
```

##### RS485 or Modbus RTU to Wi-Fi Gateway

###### Sub-Nodes

The sub-nodes may be:

```text
Industrial meters
Solar inverters
Energy meters
Soil sensors
Modbus RTU devices
```

These devices communicate through RS485 wiring.

###### Gateway

The ESP32 Gateway connects to an RS485 transceiver, such as:

```text
MAX485
MAX3485
SP3485
```

The gateway:

1. Queries the sensors using Modbus RTU.
2. Receives the sensor registers.
3. Decodes the values.
4. Packages the data as JSON.
5. Uploads the telemetry to ThingsBoard.

Architecture:

```text
RS485 / Modbus Devices
    │
    ▼
RS485 Transceiver
    │
    ▼
ESP32 Gateway
    │
    ▼
ThingsBoard Cloud
```

##### LoRa or Sub-GHz to Wi-Fi Gateway

###### Sub-Nodes

The sub-nodes may be long-range LoRa devices.

###### Gateway

The ESP32 Gateway can be paired with a LoRa radio module, such as:

```text
SX1276
SX1278
SX1262
```

The gateway:

1. Receives LoRa packets.
2. Decodes the payload.
3. Maps each payload to a downstream device name.
4. Publishes the telemetry to ThingsBoard through Wi-Fi or Ethernet.

Architecture:

```text
LoRa Sensor Nodes
    │
    ▼
ESP32 + LoRa Module Gateway
    │
    ▼
Wi-Fi or Ethernet
    │
    ▼
ThingsBoard Cloud
```

#### ESP32 Gateway versus Raspberry Pi Gateway

| Feature | ESP32 Gateway | Raspberry Pi Gateway |
|---|---|---|
| Cost | Approximately USD $3–$6 | Approximately USD $35–$100 or more |
| Power consumption | Very low, approximately 0.5 W to 1 W; suitable for solar power | Moderate, approximately 2.5 W to 15 W |
| Boot time | Near-instant boot | Commonly 20–40 seconds |
| Reliability | No general-purpose OS and no SD-card corruption risk from sudden power loss | Requires Linux maintenance and SD-card wear mitigation |
| Software setup | Custom C++ or Arduino sketch using MQTT libraries such as `PubSubClient` | Official ThingsBoard IoT Gateway, usually a Python application with YAML configuration |
| Offline data buffering | Limited by onboard RAM and flash; can use a MicroSD module if needed | High capacity; can store millions of records locally during outages |
| Best protocol fit | ESP-NOW, BLE, Modbus RTU, LoRa, serial data | OPC-UA, BACnet, CAN bus, complex databases, and enterprise integrations |
| Maintenance | Low after firmware is stable | Requires operating-system updates, service monitoring, and storage maintenance |
| Extensibility | Limited by MCU RAM, flash, and processing capability | Greater CPU, RAM, storage, and software ecosystem flexibility |

#### Summary and Recommendation

##### Use an ESP32 Gateway When

Use an ESP32 Gateway if you:

- Are building a custom local wireless sensor network.
- Use ESP-NOW, BLE, Modbus RTU, LoRa, or serial devices.
- Need a low-cost solution.
- Need low power consumption.
- Want quick boot time.
- Want to avoid managing a Linux operating system.
- Need high resilience against sudden power loss.
- Do not require large local data storage.

##### Use a Raspberry Pi Gateway When

Use a Raspberry Pi Gateway if you:

- Need official plug-and-play ThingsBoard Gateway connectors.
- Need industrial protocols such as OPC-UA or BACnet.
- Need CAN-bus support.
- Need complex data transformation.
- Need database integration.
- Need large offline telemetry buffering.
- Need a full Linux application environment.
- Need to run Docker, Node-RED, Python services, or local databases.

#### Final Decision Guide

```text
Simple sensor network using ESP-NOW, BLE, Modbus, or LoRa
    → ESP32 Gateway

Low-power or solar-powered gateway
    → ESP32 Gateway

Need large local storage and offline buffering
    → Raspberry Pi Gateway

Need OPC-UA, BACnet, databases, Docker, or advanced integrations
    → Raspberry Pi Gateway

Need to run the official ThingsBoard IoT Gateway software
    → Raspberry Pi Gateway or another Linux computer
```


## 3. Platform Integrations

<img width="756" height="89" alt="image" src="https://github.com/user-attachments/assets/e32d6314-c56f-4b4b-9485-35bcbf75c866" />


Connecting an external MQTT broker, such as Mosquitto, HiveMQ, EMQX, or AWS IoT Core, serves a similar purpose to an external gateway: it bridges external device data into ThingsBoard.

However, in the ThingsBoard ecosystem, **Platform MQTT Integration** and the **ThingsBoard IoT Gateway** are two distinct architectural features.

### 1. Comparing the Two Patterns

| Feature | Pattern A: Platform MQTT Integration | Pattern B: ThingsBoard IoT Gateway |
|---|---|---|
| How it works | ThingsBoard acts as an MQTT client that connects to an external broker, subscribes to selected topics, and decodes messages with Uplink Data Converters. | A standalone ThingsBoard IoT Gateway application runs on local hardware, connects to local brokers or sensors, processes data, and pushes it to ThingsBoard. |
| Where decoder logic runs | Inside ThingsBoard, using JavaScript or TBEL scripts in Data Converters. | Locally on the gateway device, using JSON or Python mapping configurations. |
| Network location | Suitable for a publicly accessible broker, or a broker reachable through a Remote Integration worker on the LAN. | Installed in the same local network as the devices and local broker. |
| Primary use case | Devices already publish to an established cloud or external MQTT broker, and device firmware should not be changed. | Connecting constrained local devices, serial devices, RS485 or Modbus equipment, or LAN-isolated brokers without a public IP address. |

### 2. ESP32-S3 in External Broker Architectures

If the ESP32-S3 publishes telemetry to an external broker, such as Mosquitto or HiveMQ, instead of directly to the ThingsBoard MQTT API, the architecture can look like this:

```text
+-----------------------------------------------------------------------------------+
|                         EXTERNAL BROKER ARCHITECTURE                              |
+-----------------------------------------------------------------------------------+
|                                                                                   |
|  [ Scenario 1: Cloud or Public Broker ]                                           |
|                                                                                   |
|  ESP32-S3 ---> Public Broker ---> TB MQTT Integration ---> ThingsBoard Core       |
|                 HiveMQ / AWS IoT / EMQX                                           |
|                                                                                   |
|  [ Scenario 2: LAN or Private Broker ]                                            |
|                                                                                   |
|  ESP32-S3 ---> Local Broker ---> Remote Integration or IoT Gateway ---> TB Cloud  |
|                                                                                   |
+-----------------------------------------------------------------------------------+
```

### Scenario 1: Public External Broker

#### Direct Cloud Integration

In this scenario, the ESP32-S3 publishes to a public or cloud-hosted MQTT broker.

Example topic:

```text
sensors/esp32-s3/telemetry
```

Example architecture:

```text
ESP32-S3
    │
    ▼
Public MQTT Broker
    │
    ▼
ThingsBoard MQTT Integration
    │
    ▼
ThingsBoard Cloud
```

Typical setup process:

1. Configure the ESP32-S3 to publish telemetry to an external MQTT broker.
2. Create an MQTT Integration in ThingsBoard Cloud.
3. Configure the integration to connect to the external broker.
4. Subscribe to the required MQTT topic or topic wildcard.
5. Create an Uplink Data Converter.
6. Use the converter to:
   - Read the topic name.
   - Extract a device identifier.
   - Decode the payload.
   - Create or update the corresponding ThingsBoard device.
   - Convert the payload into ThingsBoard telemetry.

For example, the device may publish:

```json
{
  "temperature": 25.4,
  "humidity": 61.2,
  "rssi": -58
}
```

to:

```text
sensors/esp32-s3/telemetry
```

The ThingsBoard Data Converter can transform the message into telemetry associated with the correct ThingsBoard device.

### Scenario 2: Private Local Broker

#### Remote Integration or IoT Gateway

If the external broker is hosted on a private LAN, such as a Mosquitto broker on a local Raspberry Pi or server, ThingsBoard Cloud cannot normally reach it directly.

Example architecture:

```text
ESP32-S3
    │
    ▼
Local Mosquitto Broker
    │
    ▼
Local Integration Worker or ThingsBoard IoT Gateway
    │
    ▼
ThingsBoard Cloud
```

A private broker may not be directly reachable because it has:

- No public IP address.
- NAT protection.
- Firewall restrictions.
- LAN-only network access.

You have two main options.

#### Option A: Remote Integration

Run a lightweight ThingsBoard Integration worker inside the local network.

The worker:

1. Connects to the local MQTT broker.
2. Subscribes to local topics.
3. Streams data securely to ThingsBoard Cloud.
4. Allows ThingsBoard Cloud integrations to work with LAN-only brokers.

#### Option B: ThingsBoard IoT Gateway

Run the open-source ThingsBoard IoT Gateway on a local device, such as:

```text
Raspberry Pi
Local Linux server
Industrial gateway
Docker host
```

The gateway:

1. Subscribes to the local Mosquitto broker.
2. Reads incoming MQTT messages.
3. Converts or maps the messages.
4. Sends them to ThingsBoard through the Gateway MQTT API.

The gateway telemetry topic is:

```text
v1/gateway/telemetry
```

### 3. Availability on ThingsBoard Cloud and CE

The availability of these approaches depends on the selected ThingsBoard edition.

#### ThingsBoard Cloud Free or Maker Plan

| Feature | Availability |
|---|---|
| Platform Integrations | Supported |
| MQTT Integration | Supported |
| Free integration quota | 1 integration |
| Free data-converter quota | 3 data converters |
| External MQTT broker testing | Supported within the free quota |

With the free cloud plan, you can connect one external MQTT broker to ThingsBoard Cloud for testing.

#### ThingsBoard Community Edition

ThingsBoard Community Edition, or CE, is self-hosted and open source.

| Feature | Community Edition Availability |
|---|---|
| Platform Integrations UI | Not included |
| Data Converters UI | Not included |
| MQTT Integration UI | Not included |
| ThingsBoard IoT Gateway | Supported |
| Custom bridge script | Supported |

The Platform Integrations and Data Converters user interface is generally a ThingsBoard Professional Edition or Cloud feature.

For Community Edition users, common alternatives are:

- ThingsBoard IoT Gateway.
- Custom MQTT bridge scripts.
- Node-RED.
- Python applications using `paho-mqtt`.
- A local service that converts MQTT payloads into ThingsBoard telemetry.

### 4. Choosing the Right Architecture

#### Use a Direct ESP32-S3 Connection When Starting Fresh

If you are writing the ESP32-S3 firmware yourself, the simplest architecture is usually:

```text
ESP32-S3
    │
    ▼
ThingsBoard Cloud MQTT API
```

The ESP32-S3 connects directly to:

```text
thingsboard.cloud:1883
```

and uses the ThingsBoard access token as its MQTT username.

Benefits include:

- Fewer components.
- Lower latency.
- Less maintenance.
- Simpler troubleshooting.
- No external broker required.
- Direct support for ThingsBoard telemetry, attributes, RPC, and OTA features.

#### Use Platform MQTT Integration When Firmware Cannot Be Changed

Use ThingsBoard Platform MQTT Integration when:

- Devices already use a legacy MQTT broker.
- Firmware cannot be changed.
- Devices are managed by a third party.
- Existing MQTT topics and payloads must remain unchanged.
- You want ThingsBoard to consume data from an established cloud MQTT broker.

In this case, use the available integration quota in ThingsBoard Cloud.

```text
ESP32-S3
    │
    ▼
Existing MQTT Broker
    │
    ▼
ThingsBoard MQTT Integration
    │
    ▼
ThingsBoard Cloud
```

#### Use ThingsBoard IoT Gateway for Local or Offline Networks

Use the ThingsBoard IoT Gateway when:

- Devices use a local broker.
- The broker does not have a public IP address.
- Devices are on an isolated LAN.
- Devices communicate through RS485, Modbus, BLE, LoRa, or serial connections.
- You need local data processing before forwarding to the cloud.
- You want to use ThingsBoard Community Edition without paid Platform Integrations.

```text
ESP32-S3 and Sensor Nodes
    │
    ▼
Local MQTT Broker
    │
    ▼
ThingsBoard IoT Gateway
    │
    ▼
ThingsBoard Cloud or Self-Hosted CE
```

### 5. Summary

| Situation | Recommended Approach |
|---|---|
| New ESP32-S3 project | Connect directly to ThingsBoard MQTT API |
| Legacy ESP32-S3 firmware using a cloud broker | Use ThingsBoard Platform MQTT Integration |
| Local Mosquitto broker with no public access | Use Remote Integration or ThingsBoard IoT Gateway |
| RS485, Modbus, BLE, LoRa, or serial sensor network | Use ThingsBoard IoT Gateway |
| ThingsBoard Community Edition deployment | Use ThingsBoard IoT Gateway or a custom bridge |
| Need to avoid reflashing existing devices | Use Platform MQTT Integration |

The simplest approach for a new ESP32-S3 project is usually direct MQTT communication with ThingsBoard.

External MQTT brokers and gateway architectures are most useful when integrating existing systems, legacy firmware, local sensor networks, or private LAN infrastructure.

---

# Direct MQTT Setup Guide

## Step 1: Create a Device

### Prerequisite

You need a ThingsBoard account.

1. Sign in at [ThingsBoard Cloud](https://thingsboard.cloud/).
2. Go to:

   ```text
   Entities → Devices
   ```

3. Click:

   ```text
   + Add Device
   ```

4. Enter a device name, for example:

   ```text
   ESP32-S3-Sensor-01
   ```

5. Leave the device profile at the default setting unless a custom profile is required.
6. Open the device details.
7. Click:

   ```text
   Manage Credentials
   ```

8. Copy the device access token.

## Step 2: Configure Arduino IDE

### Install ESP32 Board Support

Install the ESP32 board package through Arduino IDE Board Manager:

```text
esp32 by Espressif Systems
```

Then select:

```text
Tools → Board → ESP32 Arduino → ESP32S3 Dev Module
```

### Install Required Libraries

Install the following libraries through Arduino Library Manager:

| Library | Author | Purpose |
|---|---|---|
| `PubSubClient` | [Nick O'Leary](http://pubsubclient.knolleary.net/) | MQTT communication |
| `ArduinoJson` | [Benoit Blanchon](https://arduinojson.org/) | JSON telemetry payload generation |

## Step 3: Upload ESP32-S3 Firmware

Replace the placeholder Wi-Fi and ThingsBoard credentials before uploading.

```cpp
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// -----------------------------------------------------------------------------
// Network and ThingsBoard Credentials
// -----------------------------------------------------------------------------
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

const char* TB_HOST = "thingsboard.cloud";
const int TB_PORT = 1883;

const char* TOKEN = "YOUR_THINGSBOARD_ACCESS_TOKEN";

// -----------------------------------------------------------------------------
// MQTT Client
// -----------------------------------------------------------------------------
WiFiClient espClient;
PubSubClient client(espClient);

// -----------------------------------------------------------------------------
// Connect to Wi-Fi
// -----------------------------------------------------------------------------
void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

// -----------------------------------------------------------------------------
// Connect to ThingsBoard
// -----------------------------------------------------------------------------
void connectToThingsBoard() {
  while (!client.connected()) {
    // ThingsBoard access token is sent as the MQTT username.
    if (client.connect(
      "ESP32S3_Client",
      TOKEN,
      nullptr
    )) {
      // Connected successfully.
    }
    else {
      delay(5000);
    }
  }
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  connectToWiFi();

  client.setServer(
    TB_HOST,
    TB_PORT
  );
}

// -----------------------------------------------------------------------------
// Main Loop
// -----------------------------------------------------------------------------
void loop() {
  if (!client.connected()) {
    connectToThingsBoard();
  }

  client.loop();

  // Prepare JSON telemetry payload
  StaticJsonDocument<200> doc;

  doc["temperature"] =
    24.5 + random(-10, 10) / 10.0;

  doc["humidity"] =
    55.0 + random(-20, 20) / 10.0;

  doc["rssi"] = WiFi.RSSI();

  char payload;

  serializeJson(
    doc,
    payload
  );

  // Publish to the ThingsBoard telemetry topic
  client.publish(
    "v1/devices/me/telemetry",
    payload
  );

  // Publish once every 10 seconds
  delay(10000);
}
```

### Native Protocol Key Mappings

#### Telemetry

```text
v1/devices/me/telemetry
```

Use telemetry for time-series values that change over time, such as:

- Temperature.
- Wi-Fi RSSI.
- Device uptime.
- Button events.

Example telemetry payload:

```json
{
  "temperature": 25.4,
  "rssi": -62,
  "uptime": 3600,
  "button": "pressed"
}
```

#### Attributes

```text
v1/devices/me/attributes
```

Use attributes for device state and metadata, such as:

- Device status.
- LED state.
- Configuration values.
- Device information.

Example attribute payload:

```json
{
  "status": "online",
  "led-state": "ON"
}
```

#### Control Downlink

Use the following mechanisms to send LED-control commands from ThingsBoard to the ESP32-S3:

```text
Shared Attribute Updates:
v1/devices/me/attributes
```

```text
RPC Commands:
v1/devices/me/rpc/request/+
```

The ESP32-S3 can listen to both topics.

##### Shared Attribute Example

Sending this shared attribute from a ThingsBoard switch widget:

```json
{
  "led-control": true
}
```

causes the ESP32-S3 to toggle the physical LED pin.

##### RPC Command Example

Sending this RPC command from a ThingsBoard button or switch widget:

```json
{
  "method": "led-control",
  "params": true
}
```

causes the ESP32-S3 to toggle the physical LED pin.

#### Topic Summary

| Purpose | ThingsBoard MQTT Topic | Example Keys |
|---|---|---|
| Telemetry | `v1/devices/me/telemetry` | `temperature`, `rssi`, `uptime`, `button` |
| Attributes | `v1/devices/me/attributes` | `status`, `led-state` |
| Shared attribute control | `v1/devices/me/attributes` | `led-control` |
| RPC control command | `v1/devices/me/rpc/request/+` | `method: "led-control"` |

In ThingsBoard's Direct Device MQTT API, the ThingsBoard MQTT Topic is a hardcoded system keyword:
- `v1/devices/me/telemetry` tells ThingsBoard: "Assign this telemetry to whichever device owns the Access Token used during MQTT login."     

If you publish to `v1/devices/esp32s3/telemetry`, the broker will not recognize esp32s3 as a valid endpoint and will silently drop or reject your messages.

### ThingsBoard MQTT Topic Subscription Rules

You do not need to subscribe to:

```text
v1/devices/me/telemetry
```

Telemetry is write-only from the device's perspective.

The ESP32-S3 publishes telemetry to ThingsBoard, but ThingsBoard does not send standard telemetry back to the device through this topic.

#### Direction Breakdown

| Purpose | MQTT Topic | ESP32-S3 Action | Direction |
|---|---|---|---|
| Telemetry | `v1/devices/me/telemetry` | Publish only | Device → ThingsBoard |
| Attributes | `v1/devices/me/attributes` | Publish and subscribe | Device ↔ ThingsBoard |
| RPC requests | `v1/devices/me/rpc/request/+` | Subscribe only | ThingsBoard → Device |

#### Telemetry

##### Publish Only

```text
v1/devices/me/telemetry
```

The ESP32-S3 publishes time-series values, such as:

- Temperature.
- Wi-Fi RSSI.
- Device uptime.
- Button status.
- Sensor readings.
- Battery voltage.

Example:

```cpp
mqttClient.publish(
  "v1/devices/me/telemetry",
  "{\"temperature\":25.4,\"rssi\":-62}"
);
```

ThingsBoard stores and visualizes the telemetry, but it does not normally send telemetry messages back to the ESP32-S3 through this topic.

```text
ESP32-S3
    │
    │ Publish telemetry
    ▼
ThingsBoard Cloud
```

#### Attributes

##### Publish and Subscribe

```text
v1/devices/me/attributes
```

The ESP32-S3 can publish device state and metadata.

Example published attributes:

```json
{
  "status": "online",
  "led-state": "ON"
}
```

Example:

```cpp
mqttClient.publish(
  "v1/devices/me/attributes",
  "{\"status\":\"online\",\"led-state\":\"ON\"}"
);
```

The ESP32-S3 can also subscribe to receive shared-attribute updates from ThingsBoard.

For example, a dashboard switch may update:

```json
{
  "led-control": true
}
```

The ESP32-S3 receives the update and changes the physical LED state.

```text
ThingsBoard Dashboard
    │
    │ Shared attribute update
    ▼
v1/devices/me/attributes
    │
    ▼
ESP32-S3
```

#### RPC Requests

##### Subscribe Only

```text
v1/devices/me/rpc/request/+
```

The ESP32-S3 subscribes to this wildcard topic to receive real-time Remote Procedure Call commands from ThingsBoard.

Example command payload:

```json
{
  "method": "led-control",
  "params": true
}
```

Example subscription:

```cpp
mqttClient.subscribe(
  "v1/devices/me/rpc/request/+"
);
```

The `+` wildcard matches RPC request IDs, such as:

```text
v1/devices/me/rpc/request/1
v1/devices/me/rpc/request/42
v1/devices/me/rpc/request/123
```

```text
ThingsBoard Dashboard
    │
    │ RPC command
    ▼
v1/devices/me/rpc/request/+
    │
    ▼
ESP32-S3
```

#### Recommended Subscriptions

Subscribe only to actionable downlink topics:

```cpp
mqttClient.subscribe(
  "v1/devices/me/attributes"
);

mqttClient.subscribe(
  "v1/devices/me/rpc/request/+"
);
```

Do not subscribe to the telemetry topic:

```cpp
// Not required
// mqttClient.subscribe("v1/devices/me/telemetry");
```

#### Summary

```text
Telemetry:
  ESP32-S3 → ThingsBoard
  Publish only

Attributes:
  ESP32-S3 ↔ ThingsBoard
  Publish device state and subscribe to shared updates

RPC:
  ThingsBoard → ESP32-S3
  Subscribe only for real-time commands
```

Subscribing only to:

```text
v1/devices/me/attributes
v1/devices/me/rpc/request/+
```

keeps the MQTT client lightweight and ensures that the ESP32-S3 receives only actionable control messages.



### Key Technical Changes for Secure MQTT

#### Library and Transport Object

Replace the standard Wi-Fi client library:

```cpp
#include <WiFiClient.h>
```

with the secure TLS-capable client library:

```cpp
#include <WiFiClientSecure.h>
```

Then initialize a secure client object:

```cpp
WiFiClientSecure espClient;
```

#### Port Switch

Change the MQTT port from the unencrypted port:

```cpp
const int TB_PORT = 1883;
```

to the secure MQTTS port:

```cpp
const int TB_PORT = 8883;
```

#### Root CA Certificate Validation

Add the ThingsBoard Root Certificate Authority certificate:

```cpp
espClient.setCACert(TB_ROOT_CA);
```

This enables TLS certificate validation.

Benefits include:

- Encrypts MQTT traffic using TLS.
- Protects Wi-Fi credentials, access tokens, telemetry, and commands while in transit.
- Verifies that the ESP32-S3 is connecting to the legitimate ThingsBoard server.
- Helps prevent Man-in-the-Middle, or MITM, attacks.

Example:

```cpp
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

void setup() {
  // Configure the trusted server Root CA certificate
  espClient.setCACert(TB_ROOT_CA);

  // Use secure MQTT port
  mqttClient.setServer(
    "thingsboard.cloud",
    8883
  );
}
```

#### Testing Alternative

During quick prototyping, you can bypass server certificate verification:

```cpp
espClient.setInsecure();
```

Example:

```cpp
WiFiClientSecure espClient;

void setup() {
  // Encryption remains enabled, but the server certificate
  // is not verified.
  espClient.setInsecure();
}
```

> **Warning:** `setInsecure()` still encrypts the connection, but it does not verify the identity of the server. This makes the connection vulnerable to Man-in-the-Middle attacks.

Use `setInsecure()` only for short-term testing. For production deployments, always use:

```cpp
espClient.setCACert(TB_ROOT_CA);
```

## ThingsBoard Access Tokens

You can retrieve or set an access token in ThingsBoard depending on whether you are connecting an IoT device, such as an ESP32, or authenticating a user through the REST API.

### 1. Device Access Token

Use a **Device Access Token** for IoT hardware such as:

- ESP32.
- ESP32-S3.
- ESP8266.
- Raspberry Pi.
- Arduino-compatible devices.
- MQTT clients.
- HTTP telemetry clients.

#### Getting a Token for an Existing Device

1. Log in to ThingsBoard Cloud or a self-hosted ThingsBoard instance.
2. In the left navigation menu, open:

   ```text
   Entities → Devices
   ```

3. Click the device row to open the device-details panel.
4. Click:

   ```text
   Manage Credentials
   ```

5. Look for the key icon.
6. A credentials window opens.
7. Confirm that the credential type is:

   ```text
   Access Token
   ```

8. Copy the value in the **Access Token** field.

Example token:

```text
YOUR_THINGSBOARD_ACCESS_TOKEN
```

Use this token as the MQTT username:

```cpp
const char* TOKEN = "YOUR_THINGSBOARD_ACCESS_TOKEN";
```

Example MQTT connection:

```cpp
mqttClient.connect(
  "ESP32S3_Client",
  TOKEN,
  nullptr
);
```

#### Creating a New Device with an Access Token

1. Go to:

   ```text
   Entities → Devices
   ```

2. Click:

   ```text
   + Add Device
   ```

3. Enter a device name, for example:

   ```text
   ESP32-S3-Sensor-01
   ```

<img width="50%" height="auto" alt="image" src="https://github.com/user-attachments/assets/12fbf007-8282-4525-9144-0da6fb70f1d9" />


4. Optionally enable:

   ```text
   Add Credentials
   ```

5. Select:

   ```text
   Access Token
   ```

6. You can manually enter a custom access token:

   ```text
   esp32s3_secret_token_123
   ```

7. If you leave the field empty, ThingsBoard generates an access token automatically.
8. Click:

   ```text
   Add
   ```

9. Open the new device details.
10. Click:

    ```text
    Manage Credentials
    ```

11. View or copy the generated access token.

<img width="50%" height="auto" alt="image" src="https://github.com/user-attachments/assets/36ead924-0297-4069-9b8c-06f028260870" />


> **Security warning:** Treat a device access token like a password. Anyone with the token may be able to authenticate as that device and publish telemetry or receive device commands.

#### 2. User JWT Token

Use a **JWT token** when writing a script that manages ThingsBoard resources through the REST API.

Examples include:

- Creating devices.
- Creating dashboards.
- Managing users.
- Reading or modifying telemetry.
- Automating configuration.
- Using Python or Node.js scripts.

A user JWT token is different from a device access token.

| Token Type | Purpose |
|---|---|
| Device Access Token | Connects an IoT device through MQTT, HTTP, or CoAP |
| User JWT Token | Authenticates a ThingsBoard user through the REST API |

#### Request a User JWT Token

Send an HTTP `POST` request to:

```text
https://thingsboard.cloud/api/auth/login
```

##### Request Headers

```text
Content-Type: application/json
```

##### Request Payload

```json
{
  "username": "YOUR_THINGSBOARD_EMAIL",
  "password": "YOUR_THINGSBOARD_PASSWORD"
}
```

##### Example `curl` Command

```bash
curl -X POST \
  https://thingsboard.cloud/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "username": "YOUR_THINGSBOARD_EMAIL",
    "password": "YOUR_THINGSBOARD_PASSWORD"
  }'
```

##### Example Response

```json
{
  "token": "eyJhbGciOiJIUzUxMiJ9...",
  "refreshToken": "eyJhbGciOiJIUzUx..."
}
```

The value in:

```json
"token"
```

is the short-lived JWT authentication token.

The value in:

```json
"refreshToken"
```

can be used to obtain a new JWT token when the current token expires.

#### Use the JWT Token in REST Requests

Use the value of `token` in the `Authorization` header:

```text
Authorization: Bearer <token>
```

Example:

```bash
curl \
  https://thingsboard.cloud/api/tenant/devices \
  -H "Authorization: Bearer eyJhbGciOiJIUzUxMiJ9..."
```

#### Token Summary

| Token | Used By | Typical Use |
|---|---|---|
| Device Access Token | ESP32, ESP32-S3, MQTT client, HTTP client | Device telemetry, attributes, RPC, and device authentication |
| JWT Token | Python, Node.js, REST client, automation script | Dashboard, device, user, and API management |
| Refresh Token | REST client or automation script | Requesting a new JWT token after expiration |

#### Security Recommendations

- Do not commit device access tokens to public GitHub repositories.
- Do not share JWT tokens in screenshots, logs, or documentation.
- Use separate tokens for development and production devices.
- Regenerate a token if it is accidentally exposed.
- Use MQTTS on port `8883` for production device connections.
- Use HTTPS for REST API calls.
- Store tokens in environment variables or private configuration files where possible.

### Obtaining the ThingsBoard Root CA Certificate

To obtain the Root CA certificate, such as `TB_ROOT_CA`, for a ThingsBoard instance, use one of the following methods.

The method depends on whether you use:

- ThingsBoard Cloud.
- A self-hosted ThingsBoard server.
- A server with a public certificate.
- A server with a self-signed certificate.

#### Method 1: Download the Root CA Certificate

##### ThingsBoard Cloud

ThingsBoard Cloud at:

```text
thingsboard.cloud
```

uses Amazon Trust Services as a Certificate Authority.

Download the public Amazon Root CA certificate:

[Download Amazon Root CA 1](https://www.amazontrust.com/repository/AmazonRootCA1.pem)

After downloading:

1. Open the `.pem` file in a text editor, such as Notepad or VS Code.
2. Copy the complete certificate block beginning with:

   ```text
   -----BEGIN CERTIFICATE-----
   ```

3. Include everything through:

   ```text
   -----END CERTIFICATE-----
   ```

#### Method 2: Extract Certificates with OpenSSL

You can retrieve the certificate chain directly from port `8883` of a ThingsBoard server.

This method works for:

- ThingsBoard Cloud.
- Self-hosted ThingsBoard servers.
- Custom domains.
- Other MQTT-over-TLS brokers.

##### Linux, macOS, or Git Bash

Run:

```bash
openssl s_client \
  -showcerts \
  -connect thingsboard.cloud:8883 \
  </dev/null
```

##### Find the Root CA in the Output

OpenSSL normally prints two or more certificate blocks.

To locate the root certificate:

1. Review the certificate chain in the terminal output.
2. Locate the last certificate block, which is commonly the root issuer certificate.
3. Copy the entire block from:

   ```text
   -----BEGIN CERTIFICATE-----
   ```

   through:

   ```text
   -----END CERTIFICATE-----
   ```

Example certificate block:

```text
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyT3FSAYio5WrvoDTVieF5C5zANBgkqhkiG9w0BAQsF
... full certificate content ...
-----END CERTIFICATE-----
```

#### Method 3: Export the Certificate Through a Web Browser

1. Open a browser.
2. Navigate to:

   [ThingsBoard Cloud](https://thingsboard.cloud)

   or open the custom ThingsBoard domain.

3. Click the security or padlock icon beside the browser address bar.
4. Select an option similar to:

   ```text
   Connection is secure
   ```

5. Select:

   ```text
   Certificate is valid
   ```

6. In the certificate-viewer window, open:

   ```text
   Details
   ```

   or:

   ```text
   Hierarchy
   ```

   or:

   ```text
   Certification Path
   ```

7. Select the top-most root node in the certificate hierarchy.

Examples include:

```text
Amazon Root CA 1
ISRG Root X1
```

8. Click:

   ```text
   Export
   ```

   or:

   ```text
   Download
   ```

9. Select one of these formats:

   ```text
   PEM
   Base-64 encoded ASCII
   ```

#### Method 4: Self-Hosted ThingsBoard with Self-Signed Certificates

If you deploy a local ThingsBoard instance using self-signed certificates, the Root CA certificate is usually generated during the server setup.

Possible file names include:

```text
ca.pem
server.crt
root-ca.pem
```

Possible locations include:

```text
/usr/share/thingsboard/conf/
```

or a Docker volume directory.

For example:

```text
/path/to/docker/volume/certs/ca.pem
```

Copy the plain-text certificate content into the ESP32 sketch.

> **Important:** When using a self-signed certificate, the ESP32 must trust the same certificate authority that signed the ThingsBoard server certificate.

#### Formatting the Certificate in an ESP32 Sketch

Use a C++ raw string literal to store the certificate in the firmware.

```cpp
const char* TB_ROOT_CA = R"(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyT3FSAYio5WrvoDTVieF5C5zANBgkqhkiG9w0BAQsF
... paste the full Root CA certificate text here ...
-----END CERTIFICATE-----
)";
```

The raw string format:

```cpp
R"(
...
)"
```

allows the certificate to retain its line breaks without manually adding `\n` to each line.

#### Example Secure MQTT Setup

```cpp
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

const char* TB_ROOT_CA = R"(
-----BEGIN CERTIFICATE-----
... full certificate ...
-----END CERTIFICATE-----
)";

WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

void setup() {
  secureClient.setCACert(TB_ROOT_CA);

  mqttClient.setServer(
    "thingsboard.cloud",
    8883
  );
}
```

#### Troubleshooting

##### TLS Connection Fails

Verify the following:

- The hostname matches the certificate hostname.
- The ESP32-S3 clock is synchronized using NTP.
- The Root CA certificate is complete.
- The certificate begins with:

  ```text
  -----BEGIN CERTIFICATE-----
  ```

- The certificate ends with:

  ```text
  -----END CERTIFICATE-----
  ```

- The certificate uses correct line breaks.
- The secure MQTT port is correct:

  ```text
  8883
  ```

##### Testing Without Certificate Verification

For short-term testing only, use:

```cpp
secureClient.setInsecure();
```

Example:

```cpp
WiFiClientSecure secureClient;

void setup() {
  secureClient.setInsecure();

  mqttClient.setServer(
    "thingsboard.cloud",
    8883
  );
}
```

> **Warning:** `setInsecure()` encrypts traffic but does not validate the server identity. It is vulnerable to Man-in-the-Middle attacks and should not be used in production.

#### Summary

| Method | Best For |
|---|---|
| Download Root CA directly | ThingsBoard Cloud using Amazon Trust Services |
| Use OpenSSL | Any MQTT server or custom domain |
| Export from browser | Users who prefer a graphical method |
| Copy self-signed CA file | Local or self-hosted ThingsBoard deployments |

For ThingsBoard Cloud, the simplest method is usually downloading the Amazon Root CA certificate and placing it in the ESP32-S3 sketch using a C++ raw string literal.

## Step 4: Verify Telemetry and Build Dashboards

1. Return to the device page in ThingsBoard Cloud.
2. Open:

   ```text
   Latest Telemetry
   ```

3. Confirm that the following values are arriving:

   ```text
   temperature
   humidity
   rssi
   ```

4. Open:

   ```text
   Dashboards
   ```

5. Create a dashboard with:

   - Gauges.
   - Time-series charts.
   - Numeric cards.
   - Control switches.
   - Alarm indicators.

### Adding `led-control` as a ThingsBoard Shared Attribute

`led-control` does not appear automatically because it is a **Shared Attribute**.

A Shared Attribute is a setting sent from ThingsBoard down to the ESP32-S3.

This differs from values such as `led-state` or `temperature`, which are sent from the ESP32-S3 up to ThingsBoard.

```text
Shared Attribute:
ThingsBoard → ESP32-S3

Telemetry or Client Attribute:
ESP32-S3 → ThingsBoard
```

To make `led-control` visible and functional on the dashboard, complete the following steps.

#### Step 1: Create `led-control` as a Shared Attribute

1. In ThingsBoard, go to:

   ```text
   Entities → Devices
   ```

2. Click the ESP32-S3 device.
3. Open the:

   ```text
   Attributes
   ```

   tab in the device-details panel.

4. Change the dropdown from:

   ```text
   Client attributes
   ```

   to:

   ```text
   Shared attributes
   ```

5. Click:

   ```text
   + Add attribute
   ```

6. Enter the following values:

   ```text
   Key:        led-control
   Value type: Boolean
   Value:      false
   ```

7. Click:

   ```text
   Add
   ```

The new Shared Attribute should appear as:

```json
{
  "led-control": false
}
```

#### Step 2: Add a Control Switch Widget

1. In the left-hand ThingsBoard menu, open:

   ```text
   Dashboards
   ```

2. Open an existing dashboard or create a new dashboard.
3. Click the pencil icon to enter:

   ```text
   Edit Mode
   ```

4. Click:

   ```text
   Add new widget
   ```

5. Select the widget bundle:

   ```text
   Control widgets
   ```

6. Select either:

   ```text
   Switch
   ```

   or:

   ```text
   Toggle Button
   ```

7. In the widget configuration:

   ```text
   Target Entity:         Select the ESP32-S3 device alias
   Shared Attribute Key:  led-control
   ```

8. Click:

   ```text
   Add
   ```

9. Click the checkmark icon to save the dashboard.

#### How It Works

When the dashboard switch is toggled:

1. ThingsBoard updates the Shared Attribute:

   ```json
   {
     "led-control": true
   }
   ```

   or:

   ```json
   {
     "led-control": false
   }
   ```

2. ThingsBoard sends the update to the ESP32-S3 through the subscribed MQTT topic:

   ```text
   v1/devices/me/attributes
   ```

3. The ESP32-S3 receives the JSON payload:

   ```json
   {
     "led-control": true
   }
   ```

4. The ESP32-S3 MQTT callback runs.
5. The callback sets the physical GPIO pin:

   ```cpp
   digitalWrite(
     LED_PIN,
     HIGH
   );
   ```

   or:

   ```cpp
   digitalWrite(
     LED_PIN,
     LOW
   );
   ```

6. The ESP32-S3 reports the new LED state back to ThingsBoard:

   ```json
   {
     "led-state": "ON"
   }
   ```

   or:

   ```json
   {
     "led-state": "OFF"
   }
   ```

#### Data Flow

```text
ThingsBoard Dashboard Switch
    │
    ▼
Shared Attribute Update
    │
    ▼
{
  "led-control": true
}
    │
    ▼
MQTT Topic:
v1/devices/me/attributes
    │
    ▼
ESP32-S3 MQTT Callback
    │
    ▼
digitalWrite(LED_PIN, HIGH)
    │
    ▼
ESP32-S3 Reports State
    │
    ▼
{
  "led-state": "ON"
}
    │
    ▼
ThingsBoard Dashboard
```

#### Recommended MQTT Subscription

The ESP32-S3 should subscribe to:

```cpp
mqttClient.subscribe(
  "v1/devices/me/attributes"
);
```

This allows the ESP32-S3 to receive Shared Attribute updates such as:

```json
{
  "led-control": true
}
```
### Finding the Shared Attribute Key in ThingsBoard Widgets

If you cannot find the **Shared Attribute Key** field in the widget configuration window, it is usually located in a different tab of the widget editor.

Follow these steps based on the ThingsBoard Control Widget layout.

#### 1. Check the Advanced or Settings Tab

For most ThingsBoard Control Widgets, such as:

```text
Switch
Toggle Button
Power Button
```

follow these steps.

1. Open the Widget Configurator.
2. In the **Basic** or **Data** tab, select the target device or entity alias.
3. Switch to the:

   ```text
   Advanced
   ```

   or:

   ```text
   Settings
   ```

   tab.

4. Scroll down until you find the:

   ```text
   Attribute Settings
   ```

   or:

   ```text
   Value Settings
   ```

   section.

5. Configure the fields as follows:

   ```text
   Value attribute key:
   led-control
   ```

   or, if the widget should read the current LED state:

   ```text
   Initial value key:
   led-state
   ```

6. Set the update field:

   ```text
   Update attribute key:
   led-control
   ```

   or:

   ```text
   Target attribute key:
   led-control
   ```

7. Set the attribute scope to:

   ```text
   Shared attribute
   ```

<img width="70%" height="auto" alt="image" src="https://github.com/user-attachments/assets/2a81b1f2-aef2-4057-b832-f9c071ce7bae" />


<img width="70%" height="auto" alt="image" src="https://github.com/user-attachments/assets/59755222-c5af-41b3-bd7f-336dbe6ccf81" />


#### Recommended Shared Attribute Configuration

Use the following settings for a dashboard LED-control switch:

| Widget Setting | Value |
|---|---|
| Target entity | ESP32-S3 device alias |
| Value attribute key | `led-control` |
| Update attribute key | `led-control` |
| Attribute scope | Shared attribute |
| Value type | Boolean |

#### 2. Check Whether the Widget Uses RPC

Some ThingsBoard control widgets are configured to send Remote Procedure Call commands by default.

These widgets may show:

```text
RPC Method
```

instead of:

```text
Attribute Key
```

If the widget uses RPC, enter:

```text
led-control
```

in the RPC Method field.

Example RPC command sent by ThingsBoard:

```json
{
  "method": "led-control",
  "params": true
}
```

The ESP32-S3 can be configured to accept both:

```text
Shared Attribute updates
```

and:

```text
RPC commands
```

using the same command name:

```text
led-control
```

#### 3. Alternative: Generate a Dashboard Widget from Attributes

If the Control Widget configuration is confusing, use the simpler Attributes Card method.

1. Go to:

   ```text
   Entities → Devices
   ```

2. Click the ESP32-S3 device.
3. Open the:

   ```text
   Attributes
   ```

   tab.

4. Change the scope to:

   ```text
   Shared attributes
   ```

5. Find:

   ```text
   led-control
   ```

6. Select the checkbox beside the attribute.
7. Click:

   ```text
   Show on dashboard
   ```

8. Select the target dashboard.
9. Click:

   ```text
   Add
   ```

ThingsBoard automatically creates a toggle widget connected to the `led-control` Shared Attribute.

#### Expected Data Flow

```text
ThingsBoard Dashboard Widget
    │
    ▼
Shared Attribute Update
    │
    ▼
{
  "led-control": true
}
    │
    ▼
MQTT Topic:
v1/devices/me/attributes
    │
    ▼
ESP32-S3 MQTT Callback
    │
    ▼
LED GPIO changes state
```

#### RPC Data Flow

If using an RPC-based widget:

```text
ThingsBoard RPC Widget
    │
    ▼
{
  "method": "led-control",
  "params": true
}
    │
    ▼
MQTT Topic:
v1/devices/me/rpc/request/+
    │
    ▼
ESP32-S3 MQTT Callback
    │
    ▼
LED GPIO changes state
```

# ThingsBoard Cloud Free Plan

ThingsBoard Cloud provides a free Maker tier suitable for evaluation, prototyping, and small personal projects.

| Resource Category | Free Plan Quota or Limit | Notes and Technical Impact |
|---|---:|---|
| Devices and assets | 5 devices / 5 assets | Maximum number of registered physical or gateway devices |
| Data points | 1 million data points per month | A data point is one key-value pair, such as `temperature: 24.5` |
| Transport messages | 500,000 messages per month | Includes MQTT, HTTP, and CoAP traffic across all devices |
| Rule Engine executions | 3 million per month | Number of processing steps executed by rule-chain nodes |
| Data retention | 30 days | Historical time-series data and alarm logs expire after 30 days |
| Dashboards, users, and customers | 5 dashboards / 5 users / 2 customers | Supports basic sharing and multi-tenancy |
| Rule chains and integrations | 3 rule chains / 1 integration / 3 converters | Limits custom backend data routing |
| Per-device rate limit | 50 messages/second / 1,000 messages/minute | Excess requests may be rejected |
| Notifications | 100 emails / 0 SMS | Email works; SMS requires an upgrade or external webhook |

## Calculating Data Usage

A single payload may contain multiple data points.

For example:

```json
{
  "temperature": 24.5,
  "humidity": 55.0,
  "rssi": -62
}
```

This payload contains three data points:

```text
temperature
humidity
rssi
```

If sent every 10 seconds:

```text
6 messages per minute
360 messages per hour
8,640 messages per day
259,200 messages per month
```

With three telemetry keys per message:

```text
259,200 messages × 3 keys
= 777,600 data points per month
```

This is within a 1 million data-point monthly allowance, but it leaves limited capacity for additional sensors, attributes, or other devices.

# Alternative: Self-Hosted ThingsBoard Community Edition

If the application exceeds the cloud free-tier device limit but you want to avoid monthly cloud fees, ThingsBoard Community Edition, or CE, is free and open source under the Apache 2.0 license.

## Advantages

- Unlimited devices.
- Unlimited assets.
- Unlimited dashboards.
- Unlimited message throughput, subject to server resources.
- Full control of the deployment.
- No cloud subscription required.

## Disadvantages

- Requires hosting infrastructure.
- Requires installation and maintenance.
- Requires backups and security updates.
- Requires database management.
- Requires monitoring of CPU, RAM, storage, and network usage.

Typical hosting options include:

```text
Local Docker container
Raspberry Pi
Home server
Virtual Private Server
DigitalOcean
AWS EC2
Hetzner
```

A small VPS may cost approximately:

```text
USD $5–$10 per month
```

depending on the provider and resources.

# Recommendations and Best Practices

## Batch Telemetry to Reduce Quota Usage

Instead of publishing separate values every second, combine multiple sensor readings into one JSON object:

```json
{
  "temp": 24,
  "hum": 60,
  "batt": 98
}
```

For continuous sampling:

1. Store readings locally in ESP32-S3 RAM or PSRAM.
2. Build a JSON array.
3. Send the batch once per minute.

Example conceptual payload:

```json
[
  {
    "ts": 1780000000000,
    "values": {
      "temperature": 24.1,
      "humidity": 60.2
    }
  },
  {
    "ts": 1780000060000,
    "values": {
      "temperature": 24.3,
      "humidity": 60.0
    }
  }
]
```

## Use Deep Sleep for Battery Projects

For battery-operated ESP32-S3 devices:

1. Wake from deep sleep.
2. Read sensors.
3. Connect to Wi-Fi.
4. Open an HTTP or MQTT connection.
5. Publish telemetry.
6. Enter deep sleep again.

Example:

```cpp
esp_deep_sleep_start();
```

This reduces:

- Battery consumption.
- Wi-Fi radio usage.
- Cloud message usage.
- Unnecessary connection time.

## Use TLS for Production Deployments

Plain MQTT on port `1883` is suitable for local testing.

For internet-facing production devices, use TLS:

```text
MQTTS port: 8883
```

Use:

```cpp
#include <WiFiClientSecure.h>
```

Then configure `WiFiClientSecure` with the appropriate ThingsBoard root certificate.

Example concept:

```cpp
WiFiClientSecure secureClient;

secureClient.setCACert(
  thingsboard_root_ca
);

PubSubClient client(secureClient);
```

## Growth Strategy

| Deployment Size | Recommended Platform |
|---|---|
| 0–5 devices, proof of concept, or lab project | ThingsBoard Cloud Free Plan |
| More than 5 devices, hobby project, self-managed | ThingsBoard Community Edition through Docker or a VPS |
| More than 5 devices, commercial deployment, managed service | Upgrade ThingsBoard Cloud to a paid plan |

A possible commercial option is:

```text
ThingsBoard Cloud Prototype Plan
Approximately USD $49/month
Up to 50 devices
Up to 10 million data points per month
```

> Pricing, plan limits, and service features can change. Confirm the current details directly on the
