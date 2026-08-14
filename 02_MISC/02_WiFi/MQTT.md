# ESP32 MQTT     

The **ESP32-S3** is a powerful dual-core microcontroller from Espressif designed for IoT applications, featuring 2.4 GHz Wi-Fi, Bluetooth 5 (LE), and dedicated vector instructions for AI acceleration. When combined with **MQTT** (Message Queuing Telemetry Transport), it forms one of the most efficient stacks for lightweight, real-time wireless communication.


---

## 1. Basic Technology Overview

### The MQTT Architecture

MQTT is a lightweight, publish-subscribe messaging protocol designed for constrained network bandwidth and low memory footprints. Instead of communicating directly using a traditional client-server architecture (like HTTP GET/POST), endpoints route messages through a central **Broker**.

<img width="70%" height="auto" alt="image" src="https://github.com/user-attachments/assets/40c0b8e4-ad85-4213-93fe-2bc5ffe79dfb" />

*MQTT Publish-Subscribe Architecture. Source: EMQX / Introduction to MQTT Publish-Subscribe Pattern | EMQ*

* **Publisher:** An endpoint (e.g., an ESP32-S3 sensor) that sends data to a specific topic on the broker.
* **Subscriber:** An endpoint (e.g., a dashboard or another ESP32) that registers interest in a topic to receive published data.
* **Broker:** The central hub that receives published messages and dispatches them to all matched subscribers.
* **Topics:** UTF-8 hierarchical strings separated by slashes (e.g., `home/livingroom/temperature`).
* **Wildcards:**
  * `+` (Single-level wildcard): `home/+/temperature` matches `home/livingroom/temperature` and `home/kitchen/temperature`.
  * `#` (Multi-level wildcard): `home/#` matches anything under `home/`.

---

## 2. MQTT Libraries for ESP32-S3

Depending on the development environment (Arduino IDE, PlatformIO, or ESP-IDF), several popular MQTT libraries exist:

| Library | Framework | Features / Best Used For |
| :--- | :--- | :--- |
| **PubSubClient** | Arduino IDE / PlatformIO | Lightweight, easy to learn, synchronous. Best for simple projects. |
| **ESP32MQTTClient** | Arduino Core 3.x / ESP-IDF | Thread-safe, non-blocking background task based on native `esp-mqtt`. |
| **AsyncMQTT_ESP32** | Arduino IDE / PlatformIO | Non-blocking, event-driven async architecture for high-concurrency tasks. |
| **esp-mqtt** | Native ESP-IDF C/C++ | Espressif's native C component with TLS support and low memory overhead. |

---

## 3. Key API Reference (`PubSubClient`)

PubSubClient by Nick O’Leary:     
https://github.com/knolleary/pubsubclient     

When using the widely adopted `PubSubClient` library, the core API methods include:

* `PubSubClient(Client& client)`: Constructor that accepts a `WiFiClient` or `WiFiClientSecure` network client.
* `setServer(const char * domain, uint16_t port)`: Configures the MQTT broker address and port (default port `1883` for unencrypted, `8883` for TLS).
* `setCallback(MQTT_CALLBACK_SIGNATURE)`: Sets the callback function triggered when a subscribed topic receives a message.
* `connect(const char *id, [user], [pass], [willTopic], [willQos], [willRetain], [willMessage])`: Establishes a session with the broker.
* `publish(const char* topic, const char* payload)`: Sends data to a topic.
* `subscribe(const char* topic, [qos])`: Subscribes to a topic filter.
* `loop()`: Must be called regularly in the `loop()` function to process incoming messages and maintain keep-alive signals.

---

## 4. MQTT Brokers

An MQTT client cannot function without a broker. Common setups include:

1. **Local Self-Hosted Brokers:**
   * **Eclipse Mosquitto:** Lightweight open-source broker running on Raspberry Pi, local servers, or Docker containers.
   * **EMQX:** Enterprise-grade broker with rich web UI and rule engine.
2. **Cloud & Public Testing Brokers:**
   * **Public Test Brokers:** `broker.emqx.io`, `test.mosquitto.org`, or `broker.hivemq.com` (for development only, not production).
   * **Production Cloud Brokers:** HiveMQ Cloud, AWS IoT Core, Azure IoT Hub, Google Cloud IoT.

---

## 5. Critical Engineering Concepts

* **Quality of Service (QoS):**
  * **QoS 0 (At most once):** Fire-and-forget; no delivery acknowledgment.
  * **QoS 1 (At least once):** Message delivery is guaranteed via acknowledgment (`PUBACK`), but duplicates can occur.
  * **QoS 2 (Exactly once):** Handshake process ensures exact single delivery (higher memory/latency overhead).
* **Last Will and Testament (LWT):** A pre-configured message registered with the broker during connection. If the ESP32-S3 loses power or abruptly disconnects, the broker automatically publishes the LWT message (e.g., `device/status` -> `"offline"`) to inform other clients.
* **Security (MQTTS / TLS 1.2/1.3):** The ESP32-S3 features dedicated hardware encryption engines (AES, RSA, ECC). In production, always use `WiFiClientSecure` on port `8883` with Root CA certificates or X.509 client certificates to encrypt communication.

---

## 6. Practical Software Implementation Example

This complete Arduino C++ example demonstrates how an ESP32-S3 connects to Wi-Fi, publishes periodic telemetry to an MQTT broker, and listens for control commands to toggle the built-in LED.

### Prerequisites
In Arduino IDE, go to **Tools > Manage Libraries** and install:
1. **PubSubClient** by Nick O'Leary
2. **ArduinoJson** by Benoit Blanchon (optional, for structuring JSON payloads)

### Complete ESP32-S3 Code

```cpp
#include <WiFi.h>
#include <PubSubClient.h>

// Configuration - Wi-Fi & MQTT
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Public MQTT Broker for testing
const char* MQTT_BROKER   = "broker.emqx.io";
const int   MQTT_PORT     = 1883;

// Topics
const char* TOPIC_PUB_TEMP = "esp32s3/telemetry/temperature";
const char* TOPIC_SUB_LED  = "esp32s3/commands/led";

// Hardware Configuration
#define LED_PIN 2  // Onboard LED Pin for ESP32-S3

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsgTime = 0;

void setupWifi() {
  delay(10);
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected. IP Address: ");
  Serial.println(WiFi.localIP());
}

// Callback executed when an incoming MQTT message arrives
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");

  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // Control LED based on incoming command
  if (String(topic) == TOPIC_SUB_LED) {
    if (message == "ON" || message == "1") {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("Action: LED Turned ON");
    } else if (message == "OFF" || message == "0") {
      digitalWrite(LED_PIN, LOW);
      Serial.println("Action: LED Turned OFF");
    }
  }
}

void reconnectMqtt() {
  while (!client.connected()) {
    // Generate a unique client ID based on ESP32 MAC address
    String clientId = "ESP32S3Client-" + String(WiFi.macAddress());
    Serial.print("Attempting MQTT connection as ");
    Serial.print(clientId);
    Serial.print("...");

    // Connect with Last Will and Testament (LWT) setup
    const char* lwtTopic   = "esp32s3/status";
    const char* lwtPayload = "offline";
    
    if (client.connect(clientId.c_str(), lwtTopic, 0, true, lwtPayload)) {
      Serial.println(" Connected!");
      
      // Publish online status
      client.publish(lwtTopic, "online", true);

      // Subscribe to control commands
      client.subscribe(TOPIC_SUB_LED);
      Serial.print("Subscribed to: ");
      Serial.println(TOPIC_SUB_LED);
    } else {
      Serial.print(" Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  setupWifi();

  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(mqttCallback);
}

void loop() {
  // Reconnect Wi-Fi and MQTT automatically if connection drops
  if (!client.connected()) {
    reconnectMqtt();
  }
  client.loop(); // Keeps MQTT keep-alive active & processes incoming messages

  // Publish telemetry every 5 seconds (non-blocking)
  unsigned long now = millis();
  if (now - lastMsgTime > 5000) {
    lastMsgTime = now;

    // Simulate reading temperature from a sensor connected to ESP32-S3
    float simulatedTemp = 20.0 + (random(0, 100) / 10.0);
    String payload = String(simulatedTemp, 2);

    Serial.print("Publishing telemetry: ");
    Serial.print(payload);
    Serial.print(" to topic ");
    Serial.println(TOPIC_PUB_TEMP);

    client.publish(TOPIC_PUB_TEMP, payload.c_str());
  }
}
```
## 7. Testing the Implementation

### Upload the Code

Select **ESP32S3 Dev Module** in the Arduino IDE and upload the program.

### Open the Serial Monitor

Set the baud rate to `115200`. You will see connection messages and periodic telemetry publications.

### Use an MQTT Client Application

You can use an MQTT client application such as **MQTTX** or the **Mosquitto CLI**.

1. Connect to the MQTT broker:

   ```text
   Broker: broker.emqx.io
   Port: 1883
   ```

2. Subscribe to the following topic to receive live sensor updates:

   ```text
   esp32s3/telemetry/temperature
   ```

3. Publish `ON` or `OFF` to the following topic to remotely toggle the ESP32-S3 onboard LED:

   ```text
   esp32s3/commands/led
   ```
   

---

# ESP32-S3 MQTT: Introduction, Description, and Tutorial

This guide explains how to use MQTT with the ESP32-S3. It covers the basic technology, MQTT brokers, useful libraries, API documentation, a practical software implementation example, and additional production-related topics such as TLS security, OTA updates, power management, debugging, and best practices.

---

## 1. Introduction

The **ESP32-S3** is a low-cost, low-power system-on-chip from Espressif with integrated Wi-Fi and Bluetooth LE. It is widely used in IoT projects such as sensors, actuators, smart home devices, industrial gateways, wearable prototypes, and connected appliances.

**MQTT**, or **Message Queuing Telemetry Transport**, is a lightweight publish/subscribe messaging protocol commonly used in IoT systems. It is designed for unreliable networks, low-bandwidth connections, and constrained devices.

Together, **ESP32-S3 + MQTT** form a practical foundation for building IoT systems where a device can:

- Publish sensor data to a server or cloud platform.
- Subscribe to command topics and control actuators.
- Report online/offline status using MQTT Last Will and Testament.
- Communicate with dashboards, mobile apps, home automation systems, or industrial backends.

Typical examples include:

- Temperature and humidity monitoring.
- Smart relays and switches.
- Energy metering.
- Machine status reporting.
- Battery-powered telemetry nodes.
- MQTT-controlled LEDs, pumps, fans, or motors.
- Gateways that collect BLE sensor data and forward it over MQTT.

---

## 2. Description of ESP32-S3 MQTT Systems

An ESP32-S3 MQTT system usually consists of four parts:

```text
Sensors / Actuators
        |
   ESP32-S3 Device
        |
     Wi-Fi Network
        |
     MQTT Broker
        |
Backend / Dashboard / Mobile App / Cloud
```

The ESP32-S3 connects to Wi-Fi, then connects to an MQTT broker. After that, it can publish messages to topics and subscribe to topics sent by other clients.

Example topic structure:

```text
esp32s3/status
esp32s3/telemetry
esp32s3/led/set
esp32s3/led/state
```

Example messages:

```text
esp32s3/status       -> online
esp32s3/telemetry    -> {"temp":25.4,"rssi":-62}
esp32s3/led/set      -> 1
esp32s3/led/state    -> 1
```

---

## 3. MQTT Basic Technology

MQTT is a **publish/subscribe** protocol. Instead of devices talking directly to each other, they communicate through an intermediary called an **MQTT broker**.

### 3.1 MQTT Clients and Broker

An MQTT client can be:

- ESP32-S3
- Raspberry Pi
- PC
- Mobile app
- Cloud backend
- Dashboard software

The broker:

- Receives published messages.
- Routes messages to subscribed clients.
- Manages client sessions.
- Stores retained messages.
- Handles Last Will and Testament messages.
- Enforces authentication and authorization.

Popular MQTT brokers include:

| Broker | Notes |
|---|---|
| Eclipse Mosquitto | Lightweight, easy to install, good for local development |
| EMQX | High-performance, scalable, good for production clusters |
| HiveMQ | Enterprise MQTT broker with strong tooling |
| VerneMQ | Distributed MQTT broker |
| NanoMQ | Lightweight broker for edge deployments |
| AWS IoT Core | Managed cloud MQTT service |
| Azure IoT Hub | Managed cloud device service using MQTT among other protocols |
| ThingsBoard | IoT platform with built-in MQTT support |
| ThingSpeak | IoT analytics platform with MQTT support |

---

### 3.2 Publish and Subscribe

MQTT uses topics instead of direct URLs.

Example:

```text
Device publishes:
Topic:   esp32s3/telemetry
Payload: {"temp":24.5}

Dashboard subscribes:
Topic:   esp32s3/telemetry
```

A dashboard can also publish commands:

```text
Dashboard publishes:
Topic:   esp32s3/led/set
Payload: 1

ESP32-S3 subscribes:
Topic:   esp32s3/led/set
```

---

### 3.3 MQTT Topics

Topics are hierarchical strings separated by `/`.

Examples:

```text
home/livingroom/temperature
factory/line1/motor/status
device/esp32s3/abc123/command
```

Topic wildcards:

| Wildcard | Meaning |
|---|---|
| `+` | Single-level wildcard |
| `#` | Multi-level wildcard |

Examples:

```text
home/+/temperature
home/#
```

Best practices:

- Use lowercase topics.
- Keep topic names short.
- Avoid spaces.
- Avoid leading or trailing slashes.
- Separate device identity from command topics.
- Use state topics for reporting actual state.
- Use set topics for commands.

Example structure:

```text
site/device-type/device-id/telemetry
site/device-type/device-id/status
site/device-type/device-id/led/set
site/device-type/device-id/led/state
```

---

### 3.4 MQTT QoS Levels

MQTT defines three Quality of Service levels:

| QoS | Name | Meaning |
|---|---|---|
| 0 | At most once | Fire and forget, message may be lost |
| 1 | At least once | Message arrives at least once but may duplicate |
| 2 | Exactly once | Message arrives exactly once, more overhead |

For ESP32-S3 projects:

- QoS 0 is fine for frequent sensor telemetry.
- QoS 1 is often good for commands and status.
- QoS 2 is rarely needed on constrained devices unless the application requires it.

---

### 3.5 Retained Messages

A retained message is stored by the broker and sent to new subscribers immediately.

Example:

```text
Topic:   esp32s3/status
Payload: online
Retain:  true
```

If a dashboard subscribes later, it immediately receives the last known status.

Retained messages are useful for:

- Device online/offline state.
- LED state.
- Switch state.
- Alarm state.
- Configuration state.

Do not retain fast-changing telemetry such as sensor samples every second.

---

### 3.6 Last Will and Testament, LWT

MQTT Last Will and Testament allows the broker to publish a message if the client disconnects unexpectedly.

Example:

```text
Will Topic:   esp32s3/status
Will Payload: offline
Will Retain:  true
```

When the device connects successfully, it publishes:

```text
esp32s3/status -> online
```

If the device crashes or loses network unexpectedly, the broker publishes:

```text
esp32s3/status -> offline
```

This is extremely useful for monitoring device health.

---

### 3.7 Keep Alive

MQTT clients periodically send a keep-alive message to show they are still connected.

Example:

```text
Keep Alive: 30 seconds
```

If the broker does not receive any packet within the keep-alive window, it may consider the client disconnected and publish the LWT message.

For battery-powered devices, a longer keep-alive saves power but delays offline detection.

---

### 3.8 MQTT Packet Types

Common MQTT packets include:

| Packet | Purpose |
|---|---|
| CONNECT | Client asks to connect to broker |
| CONNACK | Broker acknowledges connection |
| PUBLISH | Send a message |
| PUBACK | QoS 1 publish acknowledgement |
| SUBSCRIBE | Request subscription to topics |
| SUBACK | Broker acknowledges subscription |
| UNSUBSCRIBE | Remove subscription |
| PINGREQ | Keep-alive request |
| PINGRESP | Keep-alive response |
| DISCONNECT | Clean disconnect |

---

### 3.9 MQTT Versions

The most common versions are:

| Version | Notes |
|---|---|
| MQTT 3.1.1 | Very widely supported, simple, stable |
| MQTT 5.0 | Adds reason codes, user properties, message expiry, shared subscriptions, topic aliases |

For many ESP32-S3 projects, MQTT 3.1.1 is sufficient. MQTT 5.0 is useful in larger or more advanced systems.

---

## 4. ESP32-S3 Features Relevant to MQTT

The ESP32-S3 is well suited for MQTT-based IoT devices.

Key features:

| Feature | Relevance to MQTT |
|---|---|
| Wi-Fi 802.11 b/g/n | Connects to local network or internet |
| Dual-core Xtensa LX7 CPU | Handles Wi-Fi, MQTT, sensors, and application logic |
| Bluetooth LE | Useful for BLE-to-MQTT gateways or provisioning |
| USB OTG | Easy serial debugging and firmware flashing |
| Hardware cryptographic acceleration | Helps TLS performance |
| PSRAM support | Useful for larger buffers or TLS workloads |
| Low-power modes | Useful for battery MQTT devices |
| GPIO, ADC, I2C, SPI, UART | Connect sensors and actuators |
| Secure boot and flash encryption | Useful for production security |

Common ESP32-S3 boards:

- ESP32-S3-DevKitC-1
- ESP32-S3-WROOM-1 modules
- ESP32-S3-WROOM-2 modules
- ESP32-S3-BOX
- Custom ESP32-S3 PCB designs

---

## 5. MQTT Broker

An MQTT broker is the central message router. For development, you can run a broker locally on a PC, Raspberry Pi, server, or Docker container.

### 5.1 Broker Responsibilities

The broker handles:

- TCP connections.
- MQTT protocol handling.
- Authentication.
- Authorization.
- Topic routing.
- Retained messages.
- QoS message delivery.
- Client session management.
- Last Will messages.
- TLS encryption.
- Clustering and scaling in production.

---

### 5.2 Installing Mosquitto on Linux

Mosquitto is a good broker for learning and local testing.

On Debian/Ubuntu/Raspberry Pi OS:

```bash
sudo apt update
sudo apt install mosquitto mosquitto-clients
```

Check service status:

```bash
sudo systemctl status mosquitto
```

---

### 5.3 Configure Mosquitto with Username and Password

Mosquitto 2.x often disables anonymous access by default. For a development broker accessible on the LAN, create a configuration file.

Example:

```bash
sudo nano /etc/mosquitto/conf.d/esp32.conf
```

Add:

```conf
listener 1883
protocol mqtt
allow_anonymous false
password_file /etc/mosquitto/passwd
```

Create a user:

```bash
sudo mosquitto_passwd -c /etc/mosquitto/passwd esp32
```

Enter a password when prompted.

Restart Mosquitto:

```bash
sudo systemctl restart mosquitto
```

If using a firewall, allow port 1883:

```bash
sudo ufw allow 1883/tcp
```

For production, prefer TLS on port 8883.

---

### 5.4 Test the Broker

Subscribe to all ESP32-S3 topics:

```bash
mosquitto_sub -h localhost -u esp32 -P YOUR_PASSWORD -t 'esp32s3/#' -v
```

Publish a test message:

```bash
mosquitto_pub -h localhost -u esp32 -P YOUR_PASSWORD -t esp32s3/test -m hello
```

You should see:

```text
esp32s3/test hello
```

---

## 6. MQTT Libraries for ESP32-S3

There are several ways to implement MQTT on ESP32-S3.

### 6.1 ESP-IDF ESP-MQTT

ESP-MQTT is the official MQTT component in ESP-IDF.

Best for:

- Production firmware.
- ESP-IDF projects.
- Advanced TLS configuration.
- MQTT event handling.
- Stable low-level control.

Features:

- MQTT 3.1.1 support.
- MQTT 5.0 support in newer ESP-IDF versions.
- TLS support.
- Username/password authentication.
- Last Will and Testament.
- QoS 0, 1, and 2 depending on configuration and broker.
- Event-driven architecture.
- Reconnection handling.
- MQTT over WebSocket support.

Documentation reference:


ESP-IDF MQTT documentation:        
https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/protocols/mqtt.html


---

### 6.2 Arduino ESP32 Core MQTT Libraries

If using the Arduino IDE or PlatformIO with the Arduino ESP32 core, common options include:

| Library | Best For | Notes |
|---|---|---|
| ESP32MQTTClient | Arduino ESP32 core 3.x projects | Wraps ESP-IDF ESP-MQTT |
| PubSubClient | Simple learning projects | Very popular, easy to use |
| AsyncMqttClient | Asynchronous Arduino projects | Event-driven, good for non-blocking designs |
| ArduinoMqttClient | Generic Arduino MQTT | Simple API from Arduino ecosystem |

For production ESP32-S3 projects, ESP-MQTT or an ESP-MQTT wrapper is usually preferred.

---

### 6.3 Library Comparison

| Feature | ESP-MQTT | ESP32MQTTClient | PubSubClient | AsyncMqttClient |
|---|---:|---:|---:|---:|
| Official Espressif component | Yes | Wrapper | No | No |
| ESP-IDF support | Native | Arduino core | Arduino library | Arduino library |
| TLS support | Strong | Strong | Possible via WiFiClientSecure | Possible |
| Event-driven | Yes | Yes | No | Yes |
| QoS support | Good | Good | Limited/simple | Good |
| MQTT 5 support | Depending on ESP-IDF | Depending on core | Usually no | Usually no |
| Ease of use | Medium | Easy | Very easy | Medium |
| Production suitability | High | High | Medium | Medium/high |

---

## 7. API Documentation Overview

This section summarizes important APIs. Exact API names may vary depending on library version.

---

## 7.1 ESP-IDF ESP-MQTT API

### Important Types

| Type | Purpose |
|---|---|
| `esp_mqtt_client_config_t` | MQTT client configuration |
| `esp_mqtt_client_handle_t` | MQTT client handle |
| `esp_mqtt_event_handle_t` | Event data passed to event handler |
| `esp_mqtt_event_id_t` | Event type |

---

### Common Events

| Event | Meaning |
|---|---|
| `MQTT_EVENT_ERROR` | Error occurred |
| `MQTT_EVENT_CONNECTED` | Client connected to broker |
| `MQTT_EVENT_DISCONNECTED` | Client disconnected |
| `MQTT_EVENT_SUBSCRIBED` | Subscription acknowledged |
| `MQTT_EVENT_UNSUBSCRIBED` | Unsubscribe acknowledged |
| `MQTT_EVENT_PUBLISHED` | Publish acknowledged |
| `MQTT_EVENT_DATA` | Message received |
| `MQTT_EVENT_BEFORE_CONNECT` | Called before connection attempt |

---

### Common Functions

| Function | Purpose |
|---|---|
| `esp_mqtt_client_init()` | Create MQTT client |
| `esp_mqtt_client_register_event()` | Register event handler |
| `esp_mqtt_client_start()` | Start MQTT client |
| `esp_mqtt_client_stop()` | Stop MQTT client |
| `esp_mqtt_client_destroy()` | Destroy MQTT client |
| `esp_mqtt_client_publish()` | Publish message |
| `esp_mqtt_client_subscribe()` | Subscribe to topic |
| `esp_mqtt_client_unsubscribe()` | Unsubscribe from topic |

---

### ESP-MQTT Configuration Example

Modern ESP-IDF v5.x style:

```c
esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.hostname = "192.168.1.100",
    .broker.address.port = 1883,

    .credentials.client_id = "esp32s3-device-001",
    .credentials.username = "esp32",
    .credentials.authentication.password = "CHANGE_ME",

    .session.keepalive = 30,

    .session.last_will = {
        .topic = "esp32s3/status",
        .msg = "offline",
        .msg_len = 0,
        .qos = 1,
        .retain = true,
    },
};
```

Older ESP-IDF versions often used URI-style configuration:

```c
esp_mqtt_client_config_t mqtt_cfg = {
    .uri = "mqtt://esp32:CHANGE_ME@192.168.1.100:1883",
};
```

For TLS:

```c
.broker.verification.certificate = root_ca_pem,
```

or for MQTT over WebSocket:

```c
.broker.address.uri = "mqtts://broker.example.com:8883",
```

---

## 7.2 PubSubClient API

PubSubClient is a popular Arduino MQTT library.    

**PubSubClient by Nick O’Leary**:    
https://github.com/knolleary/pubsubclient

### Common Functions

| Function | Purpose |
|---|---|
| `setServer()` | Set broker address and port |
| `setCallback()` | Set message receive callback |
| `setClient()` | Set network client |
| `setKeepAlive()` | Set MQTT keep-alive interval |
| `setBufferSize()` | Set maximum MQTT packet buffer |
| `setSocketTimeout()` | Set socket timeout |
| `connect()` | Connect to broker |
| `connected()` | Check connection state |
| `loop()` | Process MQTT background tasks |
| `publish()` | Publish message |
| `subscribe()` | Subscribe to topic |
| `unsubscribe()` | Unsubscribe from topic |
| `state()` | Get connection/error state |

---

### PubSubClient Connection States

| State | Meaning |
|---|---|
| `-4` | MQTT connection timeout |
| `-3` | Network connection lost |
| `-2` | TCP connection failed |
| `-1` | Client disconnected cleanly |
| `0` | Connected |
| `1` | Invalid protocol |
| `2` | Rejected client ID |
| `3` | Broker unavailable |
| `4` | Bad username/password |
| `5` | Not authorized |

---

## 8. Tutorial: ESP32-S3 MQTT with Arduino IDE

This tutorial builds a practical ESP32-S3 MQTT example.

The device will:

1. Connect to Wi-Fi.
2. Connect to an MQTT broker.
3. Publish telemetry every 10 seconds.
4. Publish online/offline status using MQTT Last Will.
5. Subscribe to an LED control topic.
6. Turn an LED on or off from MQTT messages.
7. Report LED state on a retained topic.

---

## 8.1 Hardware Required

- ESP32-S3 development board.
- USB cable compatible with the board.
- Optional: external LED and resistor if your board does not have a usable GPIO LED.
- Wi-Fi network.
- Computer with Arduino IDE or PlatformIO.
- MQTT broker, such as Mosquitto.

Note: Some ESP32-S3 development boards use an addressable RGB LED, such as WS2812. If your onboard LED is addressable, this simple `digitalWrite()` example may not control it. In that case, use an external LED or replace the LED logic with a NeoPixel library.

---

## 8.2 Software Required

Install:

1. Arduino IDE.
2. ESP32 Arduino core.
3. PubSubClient library.
4. Mosquitto broker, or another MQTT broker.

---

## 8.3 Install ESP32-S3 Support in Arduino IDE

In Arduino IDE:

1. Open **File > Preferences**.
2. Add the ESP32 board manager URL:

```text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

3. Open **Tools > Board > Boards Manager**.
4. Search for **esp32**.
5. Install **esp32 by Espressif Systems**.

Then select your board:

```text
Tools > Board > ESP32 Arduino > ESP32S3 Dev Module
```

Common settings:

```text
USB CDC On Boot: Enabled
Flash Size: Match your board, often 4MB or 8MB
Partition Scheme: Default or match your board
PSRAM: Match your board, disabled if unsure
Upload Speed: 921600 or default
```

If using the native USB port, make sure the correct COM port or serial device is selected.

---

## 8.4 Install PubSubClient

In Arduino IDE:

1. Open **Sketch > Include Library > Manage Libraries**.
2. Search for **PubSubClient**.
3. Install the library by **Nick O'Leary**.

---

## 8.5 Prepare the MQTT Broker

This example assumes Mosquitto is running on your local network.

Example broker address:

```text
192.168.1.100
```

Broker port:

```text
1883
```

Username:

```text
esp32
```

Password:

```text
CHANGE_ME
```

Topics used:

```text
esp32s3/status
esp32s3/telemetry
esp32s3/led/set
esp32s3/led/state
```

---

## 8.6 ESP32-S3 Arduino MQTT Example

Copy this into the Arduino IDE and modify the configuration section.

```cpp
#include <WiFi.h>
#include <PubSubClient.h>

// ----------------------------
// User configuration
// ----------------------------
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* MQTT_HOST     = "192.168.1.100";
const uint16_t MQTT_PORT  = 1883;
const char* MQTT_USERNAME = "esp32";
const char* MQTT_PASSWORD = "CHANGE_ME";

// ----------------------------
// MQTT topics
// ----------------------------
const char* TOPIC_STATUS      = "esp32s3/status";
const char* TOPIC_TELEMETRY   = "esp32s3/telemetry";
const char* TOPIC_LED_SET     = "esp32s3/led/set";
const char* TOPIC_LED_STATE   = "esp32s3/led/state";

// ----------------------------
// Hardware configuration
// ----------------------------
// Adjust this pin for your board.
// Many ESP32-S3 boards have addressable LEDs, so an external LED may be easier.
const int LED_PIN = 2;

// ----------------------------
// Timing configuration
// ----------------------------
const uint32_t TELEMETRY_PERIOD_MS = 10000;
const uint32_t MQTT_RECONNECT_INTERVAL_MS = 2000;

// ----------------------------
// Global objects
// ----------------------------
WiFiClient tcpClient;
PubSubClient mqtt(tcpClient);

bool ledState = false;
uint32_t lastTelemetryMs = 0;
uint32_t lastMqttConnectAttemptMs = 0;

// ----------------------------
// Wi-Fi connection
// ----------------------------
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");

  uint32_t startMs = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startMs < 15000) {
    delay(250);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi connected. IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi connection failed.");
  }
}

// ----------------------------
// MQTT callback
// ----------------------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Copy payload into a null-terminated string.
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

  Serial.printf("MQTT RX topic: %s, payload: %s\n", topic, message);

  if (strcmp(topic, TOPIC_LED_SET) == 0) {
    String value = String(message);
    value.trim();
    value.toLowerCase();

    bool turnOn = false;
    bool turnOff = false;

    if (value == "1" || value == "on" || value == "true") {
      turnOn = true;
    } else if (value == "0" || value == "off" || value == "false") {
      turnOff = true;
    }

    if (turnOn) {
      ledState = true;
      digitalWrite(LED_PIN, HIGH);
      mqtt.publish(TOPIC_LED_STATE, "1", true);
      Serial.println("LED turned ON");
    } else if (turnOff) {
      ledState = false;
      digitalWrite(LED_PIN, LOW);
      mqtt.publish(TOPIC_LED_STATE, "0", true);
      Serial.println("LED turned OFF");
    } else {
      Serial.println("Unknown LED command");
    }
  }
}

// ----------------------------
// MQTT connection
// ----------------------------
void connectMQTT() {
  if (mqtt.connected()) {
    return;
  }

  // Prevent reconnect spam.
  if (millis() - lastMqttConnectAttemptMs < MQTT_RECONNECT_INTERVAL_MS) {
    return;
  }

  lastMqttConnectAttemptMs = millis();

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  String clientId = "esp32s3-";
  clientId += WiFi.macAddress();
  clientId.replace(":", "");

  Serial.printf("Attempting MQTT connection as client ID: %s\n", clientId.c_str());

  // Connect with Last Will and Testament.
  // If the device disconnects unexpectedly, broker publishes "offline".
  bool connected = mqtt.connect(
    clientId.c_str(),
    MQTT_USERNAME,
    MQTT_PASSWORD,
    TOPIC_STATUS,
    0,          // Will QoS
    true,       // Will retain
    "offline"   // Will message
  );

  if (connected) {
    Serial.println("MQTT connected");

    // Publish online status.
    mqtt.publish(TOPIC_STATUS, "online", true);

    // Subscribe to command topics.
    mqtt.subscribe(TOPIC_LED_SET, 1);

    // Report current LED state.
    mqtt.publish(TOPIC_LED_STATE, ledState ? "1" : "0", true);
  } else {
    Serial.printf("MQTT connection failed, state code: %d\n", mqtt.state());
  }
}

// ----------------------------
// Publish telemetry
// ----------------------------
void publishTelemetry() {
  if (!mqtt.connected()) {
    return;
  }

  // Replace this with a real sensor reading.
  int temperature = random(20, 30);

  char payload[128];
  snprintf(
    payload,
    sizeof(payload),
    "{\"temp\":%d,\"rssi\":%ld,\"uptime_sec\":%llu}",
    temperature,
    (long)WiFi.RSSI(),
    (unsigned long long)(millis() / 1000)
  );

  bool ok = mqtt.publish(TOPIC_TELEMETRY, payload);

  if (ok) {
    Serial.printf("Published telemetry: %s\n", payload);
  } else {
    Serial.println("Telemetry publish failed");
  }
}

// ----------------------------
// Setup
// ----------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("ESP32-S3 MQTT Example");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  connectWiFi();

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  // Increase buffer if using larger JSON payloads.
  mqtt.setBufferSize(512);

  // MQTT keep-alive interval in seconds.
  mqtt.setKeepAlive(30);

  // Socket timeout in seconds.
  mqtt.setSocketTimeout(15);
}

// ----------------------------
// Main loop
// ----------------------------
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqtt.connected()) {
    connectMQTT();
  }

  mqtt.loop();

  if (millis() - lastTelemetryMs >= TELEMETRY_PERIOD_MS) {
    lastTelemetryMs = millis();
    publishTelemetry();
  }

  delay(10);
}
```

---

## 8.7 Upload and Monitor

1. Select the correct board and port.
2. Upload the sketch.
3. Open Serial Monitor at `115200`.

Expected output:

```text
ESP32-S3 MQTT Example
Connecting to Wi-Fi....
Wi-Fi connected. IP address: 192.168.1.123
Attempting MQTT connection as client ID: esp32s3-AABBCCDDEEFF
MQTT connected
Published telemetry: {"temp":25,"rssi":-61,"uptime_sec":10}
```

---

## 8.8 Test the Device

Open one terminal and subscribe to all device topics:

```bash
mosquitto_sub -h localhost -u esp32 -P CHANGE_ME -t 'esp32s3/#' -v
```

You should see:

```text
esp32s3/status online
esp32s3/led/state 0
esp32s3/telemetry {"temp":24,"rssi":-60,"uptime_sec":10}
```

Turn the LED on:

```bash
mosquitto_pub -h localhost -u esp32 -P CHANGE_ME -t esp32s3/led/set -m 1
```

Turn the LED off:

```bash
mosquitto_pub -h localhost -u esp32 -P CHANGE_ME -t esp32s3/led/set -m 0
```

You can also use text commands:

```bash
mosquitto_pub -h localhost -u esp32 -P CHANGE_ME -t esp32s3/led/set -m ON
```

```bash
mosquitto_pub -h localhost -u esp32 -P CHANGE_ME -t esp32s3/led/set -m OFF
```

---

## 9. Adding TLS Security to ESP32-S3 MQTT

For production, avoid plaintext MQTT over port 1883. Use TLS on port 8883.

### 9.1 Why TLS Matters

TLS helps protect:

- Wi-Fi credentials.
- MQTT username and password.
- Sensor data.
- Control commands.
- Device identity.

Without TLS, credentials and messages may be visible on the network.

---

### 9.2 Mosquitto TLS Configuration Example

Example Mosquitto TLS listener:

```conf
listener 8883
cafile /etc/mosquitto/certs/ca.crt
certfile /etc/mosquitto/certs/server.crt
keyfile /etc/mosquitto/certs/server.key
allow_anonymous false
password_file /etc/mosquitto/passwd
```

Restart Mosquitto:

```bash
sudo systemctl restart mosquitto
```

Test TLS connection:

```bash
mosquitto_sub -h localhost -p 8883 --cafile ca.crt -u esp32 -P CHANGE_ME -t 'esp32s3/#' -v
```

---

### 9.3 Arduino TLS Concept

For TLS with PubSubClient, use `WiFiClientSecure`.

Conceptual example:

```cpp
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

const char* ROOT_CA = R"(
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
)";

WiFiClientSecure secureClient;
PubSubClient mqtt(secureClient);

void setup() {
  secureClient.setCACert(ROOT_CA);

  mqtt.setServer("broker.example.com", 8883);
}
```

Important notes:

- TLS certificate validation usually requires correct system time.
- Use SNTP to synchronize time before connecting.
- Avoid `setInsecure()` in production.
- Store CA certificates carefully.
- TLS uses more RAM than plaintext MQTT.

Time synchronization example:

```cpp
#include <time.h>

configTime(0, 0, "pool.ntp.org", "time.nist.gov");
```

Wait for valid time before TLS connection.

---

## 10. ESP-IDF ESP-MQTT Example

For production systems, ESP-IDF with ESP-MQTT is often preferred.

Below is a simplified example structure.

```c
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt_client.h"

static const char *TAG = "ESP32S3_MQTT";

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    mqtt_client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {

    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        mqtt_connected = true;

        esp_mqtt_client_publish(mqtt_client,
                                "esp32s3/status",
                                "online",
                                0,
                                1,
                                1);

        esp_mqtt_client_subscribe(mqtt_client,
                                  "esp32s3/led/set",
                                  1);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED");
        mqtt_connected = false;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
        break;

    default:
        ESP_LOGI(TAG, "Other event id:%d", event_id);
        break;
    }
}

static void telemetry_task(void *pvParameters)
{
    char payload[128];

    while (1) {
        if (mqtt_connected) {
            snprintf(payload,
                     sizeof(payload),
                     "{\"uptime_sec\":%lld}",
                     (long long)(esp_timer_get_time() / 1000000));

            esp_mqtt_client_publish(mqtt_client,
                                    "esp32s3/telemetry",
                                    payload,
                                    strlen(payload),
                                    1,
                                    0);
        }

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void app_main(void)
{
    // In a full project, initialize Wi-Fi here:
    // nvs_flash_init();
    // esp_netif_init();
    // esp_event_loop_create_default();
    // wifi_init_sta();
    // wait_for_wifi_connected();

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.hostname = "192.168.1.100",
        .broker.address.port = 1883,

        .credentials.client_id = "esp32s3-idf-001",
        .credentials.username = "esp32",
        .credentials.authentication.password = "CHANGE_ME",

        .session.keepalive = 30,

        .session.last_will = {
            .topic = "esp32s3/status",
            .msg = "offline",
            .msg_len = 0,
            .qos = 1,
            .retain = true,
        },
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(mqtt_client,
                                   ESP_EVENT_ANY_ID,
                                   mqtt_event_handler,
                                   NULL);

    esp_mqtt_client_start(mqtt_client);

    xTaskCreate(&telemetry_task,
                "telemetry_task",
                4096,
                NULL,
                5,
                NULL);
}
```

In a complete ESP-IDF project, you must also implement Wi-Fi initialization and connection handling.

---

## 11. MQTT Topic Design Example

A good topic design makes your system easier to scale.

Example:

```text
home/livingroom/esp32s3/status
home/livingroom/esp32s3/telemetry
home/livingroom/esp32s3/led/set
home/livingroom/esp32s3/led/state
```

More production-like:

```text
site1/esp32s3/device001/status
site1/esp32s3/device001/telemetry
site1/esp32s3/device001/relay/set
site1/esp32s3/device001/relay/state
```

Separate:

```text
set   -> command sent to device
state -> actual state reported by device
```

This is important because a command may fail, may be ignored, or may take time. The device should report the real state.

---

## 12. Relevant Additional Topics

The following topics are not strictly required for a basic ESP32-S3 MQTT demo, but they are important in real projects.

---

### 12.1 Secure Boot and Flash Encryption

ESP32-S3 supports:

- Secure Boot.
- Flash encryption.

These features help protect firmware and secrets.

Useful for production when devices are deployed in the field.

---

### 12.2 Certificate Management

For TLS MQTT, you need to manage:

- Root CA certificates.
- Server certificates.
- Device certificates, if using mutual TLS.
- Certificate expiration.
- Certificate rotation.

Best practices:

- Use long-lived CA certificates.
- Use shorter-lived device certificates if possible.
- Automate certificate provisioning.
- Store certificates securely.
- Do not hard-code production secrets in public source code.

---

### 12.3 SNTP Time Synchronization

TLS certificate validation requires accurate time.

Use SNTP:

```text
pool.ntp.org
time.google.com
time.nist.gov
```

Without correct time, the ESP32 may reject valid certificates or accept expired ones depending on implementation.

---

### 12.4 MQTT Authentication and Authorization

Authentication confirms who the client is.

Common methods:

- Username/password.
- Client certificate.
- Token-based authentication.
- Cloud-specific device credentials.

Authorization controls what the client can do.

Example Mosquitto ACL:

```text
user esp32
topic read esp32s3/led/set
topic write esp32s3/status
topic write esp32s3/telemetry
topic write esp32s3/led/state
```

This prevents a device from publishing to topics it should not control.

---

### 12.5 OTA Updates

MQTT can be used to trigger OTA updates.

Example command topic:

```text
esp32s3/ota/trigger
```

Example payload:

```json
{
  "url": "https://firmware.example.com/app-v1.0.2.bin",
  "version": "1.0.2"
}
```

Best practices:

- Use HTTPS.
- Verify firmware signature or checksum.
- Use rollback-capable partition tables.
- Do not transfer large firmware images directly inside MQTT payloads.
- Trigger OTA over MQTT, download firmware over HTTP/HTTPS.

---

### 12.6 Power Management

MQTT over Wi-Fi can consume significant power.

For battery-powered ESP32-S3 devices:

- Use deep sleep between telemetry publishes.
- Use modem sleep when possible.
- Increase MQTT keep-alive interval.
- Reduce Wi-Fi transmit power if appropriate.
- Avoid constant TCP reconnections.
- Use QoS carefully.
- Avoid retained telemetry.

Deep-sleep MQTT pattern:

```text
Wake up
Connect Wi-Fi
Connect MQTT
Publish telemetry
Disconnect
Sleep
```

In this pattern, subscriptions are usually not useful because the device is asleep most of the time.

---

### 12.7 BLE Provisioning

ESP32-S3 supports Bluetooth LE. BLE can be used to provision Wi-Fi credentials before the device connects to MQTT.

Common provisioning methods:

- SoftAP web configuration.
- BLE configuration.
- ESP BluFi.
- ESP-IDF Unified Provisioning.
- Wi-Fi Manager style configuration portal.

Provisioning flow:

```text
User connects to ESP32-S3 over BLE or AP
User provides Wi-Fi SSID/password
ESP32-S3 saves credentials
ESP32-S3 connects to Wi-Fi
ESP32-S3 connects to MQTT broker
```

---

### 12.8 MQTT over WebSocket

Some networks block port 1883 or 8883. MQTT can also run over WebSocket.

Example URIs:

```text
ws://broker.example.com:8080/mqtt
wss://broker.example.com:443/mqtt
```

ESP-MQTT supports WebSocket connections in many ESP-IDF versions.

This is useful when connecting through restrictive firewalls or web-based dashboards.

---

### 12.9 Device Shadow / Digital Twin

A device shadow is a stored representation of device state.

Example:

```json
{
  "led": {
    "desired": 1,
    "reported": 0
  }
}
```

This is useful when the device is offline.

Cloud platforms provide shadow/twin services:

- AWS IoT Device Shadow.
- Azure IoT Device Twin.
- Google Cloud IoT device state/config historically.
- ThingsBoard device attributes.
- Custom retained MQTT topics.

---

### 12.10 Data Serialization

MQTT payloads can be:

- Plain text.
- JSON.
- CBOR.
- MessagePack.
- Protocol Buffers.
- Binary structures.

For simple systems, JSON is easy.

Example:

```json
{
  "temp": 24.5,
  "humidity": 58.2,
  "rssi": -63,
  "uptime": 3600
}
```

For constrained devices or high-frequency data, binary formats may be more efficient.

---

### 12.11 Logging and Debugging

Useful debugging tools:

- Serial monitor logs.
- `mosquitto_sub -v`
- `mosquitto_sub -d`
- Broker logs.
- Wireshark or tcpdump.
- Network scanner.
- Ping to broker.
- DNS lookup tools.
- MQTT client state codes.

Example verbose subscribe:

```bash
mosquitto_sub -h localhost -u esp32 -P CHANGE_ME -t '#' -v
```

Example debug output:

```bash
mosquitto_sub -h localhost -u esp32 -P CHANGE_ME -t '#' -v -d
```

---

## 13. Common Problems and Troubleshooting

| Problem | Possible Cause | Solution |
|---|---|---|
| ESP32-S3 cannot connect to Wi-Fi | Wrong SSID/password | Check credentials and signal strength |
| ESP32-S3 cannot connect to MQTT broker | Wrong broker IP | Use correct IP or hostname |
| MQTT rc=4 or rc=5 | Bad username/password or authorization failure | Check broker credentials and ACL |
| MQTT rc=-2 | TCP connection failed | Check firewall, broker port, network route |
| Device connects then disconnects | Duplicate client ID | Use unique client ID |
| No messages received | Wrong topic | Check topic spelling and case |
| TLS fails | Wrong CA certificate | Verify CA certificate and server name |
| TLS fails | Incorrect time | Synchronize SNTP time |
| Messages cut off | Buffer too small | Increase MQTT buffer size |
| Device resets | Stack overflow or watchdog reset | Increase task stack, avoid blocking callbacks |
| Broker rejects connection | Anonymous access disabled | Configure username/password |
| Broker only local | Mosquitto listener bound to localhost | Configure LAN listener |
| Commands delayed | Wi-Fi power saving or keep-alive too long | Adjust power save and keep-alive |
| LED not changing | Wrong GPIO or addressable LED | Use correct pin or NeoPixel library |
| Random disconnects | Router NAT timeout, weak Wi-Fi, broker overload | Improve network, reduce keep-alive, check broker logs |

---

## 14. Production Best Practices

For a reliable ESP32-S3 MQTT product, consider the following.

### Connectivity

- Use static DHCP or DNS names where appropriate.
- Implement reconnect backoff.
- Avoid aggressive reconnect loops.
- Monitor Wi-Fi RSSI.
- Handle router reboots gracefully.
- Use SNTP for time synchronization.

### MQTT

- Use unique client IDs.
- Use LWT for offline detection.
- Use retained messages only for state, not telemetry.
- Prefer QoS 1 for commands.
- Use QoS 0 for high-frequency telemetry if occasional loss is acceptable.
- Keep payloads small.
- Avoid deeply nested topics.
- Version your firmware and include it in telemetry.

### Security

- Use TLS.
- Use username/password or certificates.
- Use broker ACLs.
- Isolate IoT devices on a separate VLAN if possible.
- Store secrets securely.
- Enable secure boot and flash encryption if required.
- Rotate credentials when possible.
- Do not expose broker ports unnecessarily to the internet.

### Firmware

- Add OTA updates.
- Add firmware version reporting.
- Add safe rollback.
- Log disconnect reasons.
- Use non-blocking code where possible.
- Avoid long delays in MQTT callbacks.
- Use FreeRTOS tasks for sensor polling and publishing.
- Test watchdog behavior.
- Test power-loss recovery.

### Monitoring

- Publish device status.
- Publish Wi-Fi RSSI.
- Publish free heap.
- Publish uptime.
- Publish firmware version.
- Track reconnect count.
- Track publish failure count.

Example telemetry payload:

```json
{
  "fw": "1.0.3",
  "uptime": 86400,
  "rssi": -63,
  "heap": 210000,
  "temp": 24.7
}
```

---

## 15. Example System Architecture

A complete ESP32-S3 MQTT system might look like this:

```text
+----------------+        +----------------+        +----------------+
|   Sensors      |        |   ESP32-S3     |        |   MQTT Broker  |
|   Buttons      +------->+   Wi-Fi        +------->+   Mosquitto /  |
|   Relays       |        |   MQTT Client  |        |   EMQX / Cloud |
+----------------+        +----------------+        +-------+--------+
                                                            |
                                                            v
                                                    +-------+--------+
                                                    | Dashboard /    |
                                                    | Backend / App  |
                                                    +----------------+
```

The device publishes:

```text
esp32s3/status
esp32s3/telemetry
esp32s3/led/state
```

The backend publishes:

```text
esp32s3/led/set
esp32s3/ota/trigger
esp32s3/config/set
```

---

## 16. Simple Project Ideas

Once the basic example works, you can extend it into these projects:

### 16.1 Wi-Fi Temperature Sensor

Publish:

```text
esp32s3/telemetry
```

Payload:

```json
{"temp":24.5,"humidity":55.1}
```

### 16.2 MQTT Relay Controller

Subscribe:

```text
esp32s3/relay/set
```

Publish:

```text
esp32s3/relay/state
```

### 16.3 BLE-to-MQTT Gateway

ESP32-S3 scans BLE sensors and publishes their data to MQTT.

Example topic:

```text
gateway/ble/sensor-aabbcc/temperature
```

### 16.4 Home Automation Node

Integrate with:

- Home Assistant.
- OpenHAB.
- Node-RED.
- ThingsBoard.
- Custom MQTT dashboard.

### 16.5 Industrial Status Monitor

Publish:

```text
factory/line1/machine5/status
factory/line1/machine5/fault
factory/line1/machine5/counter
```

---

## 17. Summary

ESP32-S3 MQTT is a powerful and practical combination for IoT development. The ESP32-S3 provides Wi-Fi, BLE, enough processing power, and hardware security features, while MQTT provides a lightweight and reliable messaging model for device-to-cloud and cloud-to-device communication.

A basic ESP32-S3 MQTT project needs:

1. ESP32-S3 firmware.
2. Wi-Fi connection.
3. MQTT client library.
4. MQTT broker.
5. Topics for telemetry, status, and commands.
6. Authentication and, preferably, TLS security.
7. Robust reconnection and error handling.

For learning, PubSubClient with Arduino IDE is simple and effective. For production firmware, ESP-IDF ESP-MQTT or an ESP-MQTT-based Arduino wrapper is usually the better choice because it provides stronger event handling, TLS support, and long-term maintainability.

# 18. Setting MQTT Broker

The absolute quickest way to test MQTT without installing any software (like Mosquitto, MQTTX, or MQTT Explorer) is to use a **Public MQTT Broker** combined with a **Browser-based Web MQTT Client**.

This method requires **zero installation**, bypasses local firewall/router port-forwarding issues, and takes less than 60 seconds to set up.

---

## Use a Browser-Based Web Client
Open one of these free, web-based MQTT clients in your browser. They connect to public brokers using WebSockets.

**Option A: HiveMQ Web Client (Easiest)**
*   **URL:** [https://www.hivemq.com/demos/websocket-client/](https://www.hivemq.com/demos/websocket-client/)
*   *Note:* It automatically connects to HiveMQ’s public broker the moment you open the page.

**Option B: MQTTX Web (More Modern UI)**
*   **URL:** [MQTTX Web](https://mqttx.app/web-client#/)
*   *Setup:* Click "New Connection", set Host to `broker.emqx.io`, Port to `8084`, and ensure "WebSocket" is selected.

**Option C: EMQX Web (More Modern UI)**
*   **URL:** [EMQX Cloud](https://www.emqx.com/en/cloud)
*   *Sign up:* Sign up for EMQX Platform, set Host to `ffcebc18.ala.asia-southeast1.emqxsl.com`, Port to `8084`, and ensure "WebSocket" is selected.
---

## Option A: HiveMQ Web Client
### Step 1: Test Publishing and Subscribing (In the Browser)
Using the **HiveMQ Web Client** as an example:

1. **Subscribe to a topic:**
   * In the "Subscriptions" box, type a unique topic: `myname/esp32s3/test` *(Replace "myname" with something unique so you don't see other people's traffic)*.
   * Click **Subscribe**.
2. **Publish a message:**
   * In the "Publish" box, type the exact same topic: `myname/esp32s3/test`.
   * Type a message: `Hello from browser!`.
   * Click **Publish**.
3. You will instantly see your message appear in the "Messages" log at the bottom.

---

### Step2: Connect your ESP32-S3 to the Same Broker
To test your ESP32-S3 without setting up a local server, simply point your ESP32 code to the public broker. The ESP32 will connect via standard TCP, while your browser is connected via WebSockets, but **they will see each other's messages**.

Modify your ESP32 Arduino code configuration section to this:

```cpp
// ----------------------------
// Public Broker Configuration
// ----------------------------
const char* MQTT_HOST     = "broker.hivemq.com"; // Free public broker
const uint16_t MQTT_PORT  = 1883;                // Standard TCP port for ESP32
const char* MQTT_USERNAME = "";                  // No username needed
const char* MQTT_PASSWORD = "";                  // No password needed

// Use a unique topic so you don't collide with others!
const char* TOPIC_STATUS    = "myname/esp32s3/status";
const char* TOPIC_TELEMETRY = "myname/esp32s3/telemetry";
const char* TOPIC_LED_SET   = "myname/esp32s3/led/set";
```

*Note: Because public brokers don't require authentication, you must remove the username/password from your `mqtt.connect()` function call in the ESP32 code:*

```cpp
// Change this:
bool connected = mqtt.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD, ...);

// To this (No Auth):
bool connected = mqtt.connect(clientId.c_str(), TOPIC_STATUS, 0, true, "offline");
```

---
### Step 3: Setup HiveMQ Web Client       
1. Set `Host: broker.hivemq.com`
2. Copy `clientId` eg. `esp32s3-441BF6D63E30` from your Serial port.
   ```text
   ESP32-S3 MQTT Example
   Connecting to Wi-Fi.....
   Wi-Fi connected. IP address: 192.168.0.35
   Attempting MQTT connection as client ID: esp32s3-441BF6D63E30
   MQTT connected
   ```
3. Set `Port: 8884`
4. Add Subscription eg `esp32s3/#`
5. Under Publish, select topic (must match the code setting), eg `esp32s3/led/set`

Below is the screenshot     
<img width="70%" height="auto" alt="image" src="https://github.com/user-attachments/assets/204346ae-02f5-4bf4-a408-15d3df71a8d4" />


### Step 4: Test the ESP32 and Browser Together
1. Flash the updated code to your ESP32-S3 and open the Serial Monitor.
2. Go back to your **HiveMQ Web Client** browser tab.
3. Subscribe to `myname/esp32s3/#` (The `#` is a wildcard that catches all sub-topics).
4. You will instantly see the ESP32's telemetry and "online" status appear in your browser!
5. Publish `1` to `myname/esp32s3/led/set` from the browser, and watch your ESP32's physical LED turn on.

## Option B: Connect to MQTTX Web (More Modern UI)

If you want to use the standalone web client, open [MQTTX Web](https://mqttx.app/web) in your browser.

1. Click **+ New Connection**.

2. Change the protocol dropdown to `wss://`.

3. Enter the following host:

   ```text
   broker.emqx.io
   ```

4. Enter the port:

   ```text
   8084
   ```

   Port `8084` is the default secure WebSocket port for EMQX Serverless.

5. Set the WebSocket path to:

   ```text
   /mqtt
   ```

6. Enter the **Username** and **Password** that you created.

7. Click **Connect**.

8. Click **+ New Subscription** and enter your ESP32 topic, for example:

   ```text
   esp32s3/#
   ```
9. Under **Publish**, select topic (must match the code setting), for example:     
   ```text
   esp32s3/led/set
   ```      
Below is the screenshot    
<img width="90%" height="auto" alt="image" src="https://github.com/user-attachments/assets/5796ec8c-7643-4cde-8d0c-94c5a5d056b3" />

Below is the screenshot for subscriptions     
<img width="90%" height="auto" alt="image" src="https://github.com/user-attachments/assets/732bfe93-761b-4b8b-b268-fef56c2fedc5" />

## Option C: EMQX Web
1. Sign up for EMQX Platform at **Start Free**: https://www.emqx.com/en/cloud
2. Create new project, select EMQX Broker
3. Set Host: `ffcebc18.ala.asia-southeast1.emqxsl.com`
4. Enter the port: `8084`
5. Enter Client ID
6. Access Control/Authentication: register user name and password

To view the actual message content in your private EMQX Cloud deployment, use the built-in console tools or MQTTX:

### 1. View Retained Messages directly in EMQX Console
Your screenshot shows **Retained messages: 2**. Because your ESP32 code publishes status messages with `retained = true`, the broker stores the last published payload for those topics.

1. On the left menu under **Monitor**, click **Retained messages** (5th item down from Monitor).
2. Click on the topic name (e.g., `esp32s3/status` or `esp32s3/led/state`) to view the exact payload stored by your ESP32.

---

### 2. Live Stream Data via Diagnostics (WebSocket Client)
To watch incoming telemetry streaming in real-time:

1. Look at the left sidebar menu under **Diagnostics** (bottom section).
2. Click on **Diagnostics** to expand the menu, then select **WebSocket Client**.
3. Click **Connect** using your MQTT username (`esp32s3`) and password.
4. Under **Subscriptions**, add a subscription to `esp32s3/#` and click **Subscribe**.
5. As your ESP32 publishes every 10 seconds, the messages will appear in the **Received Messages** panel at the bottom.

<img width="90%" height="auto" alt="image" src="https://github.com/user-attachments/assets/71caec08-cd7c-43c4-9914-3ae7b4f7f99b" />

<img width="90%" height="auto" alt="image" src="https://github.com/user-attachments/assets/bebf3465-888f-4db4-97ce-70a8ea60a85e" />

<img width="90%" height="auto" alt="image" src="https://github.com/user-attachments/assets/e8245cfa-e88a-4dd7-9a92-f3fc0806a083" />

---

### 3. View Live Data in MQTTX Web / Desktop
If you are connected via MQTTX Web using your private broker host (`ffcebc18.ala.asia-southeast1.emqxsl.com` on port `8084`):

1. Make sure your ESP32 code is also set to `MQTT_HOST = "ffcebc18.ala.asia-southeast1.emqxsl.com"`.
2. Click **+ New Subscription** in MQTTX.
3. Type `esp32s3/#` in the **Topic** field and save.
4. Your ESP32's JSON telemetry `{"temp":...,"rssi":...}` will appear in the main chat view.

---

## ⚠️ Crucial Warnings for Public Brokers
1. **Zero Privacy:** Public brokers are exactly that—public. Anyone in the world who subscribes to `#` or guesses your topic name can see your messages. **Never send passwords, API keys, or sensitive personal data over a public broker.**
2. **No Guarantees:** Public brokers are for testing only. They may restart, drop connections, or rate-limit you without warning.
3. **Use Unique Topics:** Always prefix your test topics with something unique (like your name, a random string, or your MAC address, e.g., `john_doe_8472/esp32s3/...`) to avoid cross-talk with other developers testing at the same time.
4. **Retained Messages:** Be careful using the `Retain` flag on public brokers. If you retain a message on a common topic like `test/led`, the next person who tests that topic will receive your old message.

---


## Common Paths for Processing IoT Data

Once your ESP32-S3 is publishing data to EMQX, the MQTT broker acts as the central transit hub. To transform raw messages into a functional IoT solution, typical next steps involve database storage, visualization dashboards, or automated alerts.    


### 1. Store Data in a Database (Persistence)
By default, MQTT messages exist in-flight and are not saved long-term. You can use EMQX's built-in **Data Integration (Rule Engine)** to automatically forward incoming JSON payloads into a database:
* **Time-Series Databases (Best for telemetry/sensors):** InfluxDB, TimescaleDB, TDengine.
* **Relational Databases:** PostgreSQL, MySQL.
* **NoSQL Databases:** MongoDB, Redis.

---

### 2. Visualize Data on Dashboards
Connect your database or broker to a dashboard to build graphs, gauges, and historical trend views:
* **Grafana:** Connects seamlessly to InfluxDB or PostgreSQL to display real-time sensor charts.
* **Node-RED:** A low-code flow-based visual editor that can subscribe to EMQX and render UI dashboards.
* **ThingsBoard / Adafruit IO / Blynk:** Dedicated IoT platforms with pre-built widget libraries for mobile and web.

---

### 3. Send to Web Apps or Cloud APIs (HTTP Webhooks)
You can set up EMQX to trigger an HTTP POST request whenever new data arrives:
* Forward telemetry to a **Google Apps Script Web App** or custom REST API.
* Send data directly to a Python Flask/FastAPI server or Node.js backend.

---

### 4. Set Up Automated Alerts & Triggers
Create rule conditions directly inside EMQX to monitor critical threshold values:
* **Rules SQL Example:** `SELECT payload.temp FROM "esp32s3/telemetry" WHERE payload.temp > 30`
* **Actions:** Send instant alerts via email, Telegram Bot, Slack, or Webhook when thresholds are breached.

---

## Recommended Next Step: Set Up Data Integration in EMQX

1. Open your deployment in EMQX Cloud and navigate to **Data Integration** (the cylinder/database icon on the left navigation bar).
2. Click **Data Integration** -> **Rules** -> **Create Rule**.
3. Use the SQL editor to extract fields from your ESP32 JSON payload:
   ```sql
   SELECT
     payload.temp AS temperature,
     payload.rssi AS wifi_signal,
     clientid AS device_id
   FROM
     "esp32s3/telemetry"
   ```


Here is how the options compare for completely free setups:

* **HTTP Server (Recommended):** Completely free. It allows you to push incoming ESP32 data straight to a custom web endpoint, such as a free **Google Apps Script Web App**, a **Pipedream / Make webhook**, or a local server on your network.
* **EMQX Tables:** Internal to EMQX. While it doesn't incur an extra cloud database cost, storage is temporary (in-memory) and restricted to limited usage inside the cluster rather than a full long-term data store.
* **Kafka Producer:** Requires running or paying for a managed Apache Kafka cluster (like Confluent Cloud), which is overkill and not completely free for simple projects.

### How to set up HTTP Server with Google Apps Script:
1. Choose **HTTP Server**.
2. Set the URL to your deployed Google Apps Script Web App URL (`https://script.google.com/macros/s/.../exec`).
3. Whenever your ESP32 publishes telemetry, EMQX will trigger an HTTP POST request to append that data directly into a Google Sheet.

---

# Use Google Sheets via HTTP Server (100% Free & Fast)

## Phase 1: Set Up the Google Sheet & Script

### Step 1: Create the Google Sheet
1. Open [Google Sheets](https://sheets.new) and create a blank spreadsheet.
2. Label the top row (Row 1) as follows:
   * **Cell A1:** `Timestamp`
   * **Cell B1:** `Temperature`
   * **Cell C1:** `RSSI`
   * **Cell D1:** `Uptime`

---

### Step 2: Add Google Apps Script
1. Click **Extensions** -> **Apps Script**.
2. Replace all the code in `Code.gs` with the following script:

```javascript
function doPost(e) {
  try {
    var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
    var data = JSON.parse(e.postData.contents);
    
    // Append a new row with the parsed fields
    sheet.appendRow([
      new Date(),
      data.temp,
      data.rssi,
      data.uptime
    ]);
    
    return ContentService.createTextOutput("Success").setMimeType(ContentService.MimeType.TEXT);
  } catch (err) {
    return ContentService.createTextOutput("Error: " + err.message).setMimeType(ContentService.MimeType.TEXT);
  }
}
```
### Step 3: Deploy as a Web App   
1. At the top right of the Apps Script window, click **Deploy** -> **New deployment**.
2. Click the **Gear icon (Select type)** next to "Select type" and choose **Web app**.
3. Configure the settings:
   - **Description**: `EMQX Telemetry Logger`
   - **Execute as**: `Me (your-email@gmail.com)`
   - **Who has access**: `Anyone` (Crucial for EMQX to access it without authentication header issues)
4. Click Deploy.
5. Grant access permissions when prompted.
6. Copy the **Web App URL** (starts with `https://script.google.com/macros/s/.../exec`).

## Phase 2: Configure EMQX Cloud Integration
### Step 4: Create the HTTP Server Connector
1. Go back to your **EMQX Cloud Console**.
2. In the left menu, select **Data Integration** -> **Connectors** -> **New Connector**.
3. Select **HTTP Server** as the Connector Type.
4. Fill in the fields:
   - **Connector Name**: `google_sheets_connector`
   - **URL**: Paste your Google Apps Script Web App URL.
5. Click **Test Connectivity** (it may return a success or direct code response) and click **Create**.
### Step 5: Create the Processing Rule
1. Under **Data Integration**, go to **Rules** -> **Create Rule**.
2. In the **SQL Editor**, enter:
   ```SQL
   SELECT
    payload.temp as temp,
    payload.rssi as rssi,
    payload.uptime_sec as uptime
   FROM
    "esp32s3/telemetry"
   ```

### Step 6: Attach Action & Body Template
1. On the same page, click **Add Action** (or **Add Sink**) and select **HTTP Server**.
2. Choose your `google_sheets_connector`.
3. Configure the HTTP Action request settings:
   - **Method**: `POST`
   - **Headers**: `Add Content-Type: application/json`
   - **Request Body**: Use JSON matching your script keys:
```JSON
{
  "type": "telemetry",
  "temp": ${temp},
  "rssi": ${rssi},
  "uptime": ${uptime}
}
```

4. Click **Save** to complete rule creation.    

### Step 7: Verify Data Ingestion
Ensure your ESP32-S3 is connected and publishing to `esp32s3/telemetry`.
Open your Google Sheet—new rows with the current timestamp, temperature, RSSI, and uptime values will begin populating automatically as messages arrive.     

**Here is the spreadsheet**:     
https://docs.google.com/spreadsheets/d/1GANocTNqmIkN6JLxhaTbU2BrWMLX-QrQ-5-Tz_ssT1g/edit?gid=0#gid=0

## Handle both sensor telemetry and LED control/status messages

To handle both sensor telemetry and LED control/status messages in the same Google Sheet, update your Apps Script to dynamically process different MQTT topics, and add a second EMQX rule.

### Step 1: Update the Google Sheet Header

In your Google Sheet, add two new columns to Row 1 so it accommodates both telemetry and LED status data:

- Cell A1: Timestamp
- Cell B1: Temperature
- Cell C1: RSSI
- Cell D1: Uptime
- Cell E1: LED State

### Step 2: Update the Google Apps Script

Open **Extensions → Apps Script** and replace `Code.gs` with this updated script that checks the incoming `type` field:

```javascript
function doPost(e) {
  try {
    var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
    var data = JSON.parse(e.postData.contents);
    var timestamp = new Date();

    if (data.type === "telemetry") {
      // Append a normal telemetry row

      sheet.appendRow([
        timestamp,
        data.temp,
        data.rssi,
        data.uptime,
        "" // Column E stays blank until an LED event happens
      ]);
    } else if (data.type === "led_status") {
      /* OR update at fix Cell
      // Target fixed cell G3 for the latest LED state
      sheet.getRange("G3").setValue(data.state);
      
      // Optional: If you also want to show the last updated timestamp right next to it in H3
      sheet.getRange("F3").setValue(timestamp);
      */
      // Option: Use "-" or "EVENT" instead of blank quotes so it's clear it wasn't a missed telemetry reading
      sheet.appendRow([
        timestamp,
        "-", // Temp
        "-", // RSSI
        "-", // Uptime
        data.state // Column E
      ]);
      
    }

    return ContentService.createTextOutput("Success").setMimeType(ContentService.MimeType.TEXT);
  } catch (err) {
    return ContentService.createTextOutput("Error: " + err.message).setMimeType(ContentService.MimeType.TEXT);
  }
}
```

**Important:** After updating the script, click **Deploy → Manage deployments → Edit (Pencil icon) → Change Version to New version → Click Deploy**.

### Step 3: Update Rule 1 (Telemetry Rule Body Template in EMQX)

Update the HTTP Action body template for your existing telemetry rule so it includes the `"type": "telemetry"` flag:

```json
{
  "type": "telemetry",
  "temp": ${temp},
  "rssi": ${rssi},
  "uptime": ${uptime}
}
```

### Step 4: Create Rule 2 for LED Status (`esp32s3/led/state`)

To capture whenever the LED state changes or is toggled:

1. In EMQX, go to **Data Integration → Rules → Create Rule**.
2. **SQL Editor:**

   ```sql
   SELECT
     payload.state as state
   FROM
     "esp32s3/led/state"
   ```

3. Click **Add Action** (or **Add Sink**) and select your existing `google_sheets_connector`.
4. Set **Method** to `POST` and add Header `Content-Type: application/json`.
5. **Request Body:**

   ```json
   {
     "type": "led_status",
     "state": "${state}"
   }
   ```

6. Click **Save**.


Now, whenever telemetry is sent or the LED is toggled via `esp32s3/led/set` / `esp32s3/led/state`, the row will automatically append to your Google Sheet with the correct data columns filled in.

---
## Send a toggle command from Google Sheets back to your ESP32-S3
To send a toggle command from Google Sheets back to your ESP32-S3 through EMQX, you will use Google Apps Script to make an HTTP POST request to the EMQX Cloud REST API, which then publishes the message to the `esp32s3/led/set` topic.

### Step 1: Obtain EMQX REST API Credentials

1. In your EMQX Cloud Console, go to **Management → API Keys** (or **Deployment Settings → API Keys**).
2. Click **Create API Key**.
3. Set the key description (e.g., `Google Sheets Control`) and assign **Publish** or **Admin** permissions.
4. Copy the generated **API Key** (Username) and **Secret Key** (Password).

### Step 2: Set Up the Control Interface in Google Sheets

1. Open your Google Sheet.
2. In a clean cell (e.g., **Cell H2**), type either "1", "0", `ON` or `OFF`.
3. Highlight the cell, then go to **Insert → Drawing**.
4. Draw a button (a rectangle with text like "Toggle LED"), click **Save and Close**, and position the drawing next to your cell.

### Step 3: Add the Publish Script in Google Apps Script

1. In your Google Sheet, open **Extensions → Apps Script**.
2. At the bottom of `Code.gs`, append the following function:


  
```javascript

function sendLedToggle() {
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  var rawValue = sheet.getRange("H2").getValue().toString().trim().toUpperCase(); // Cell H2

  // Convert cell value to string "1" or "0"
  var commandStr = "0";
  if (rawValue === "ON" || rawValue === "1" || rawValue === "TRUE") {
    commandStr = "1";
  } else if (rawValue === "OFF" || rawValue === "0" || rawValue === "FALSE") {
    commandStr = "0";
  }

  // EMQX API Credentials
  var appKey = "YOUR_EMQX_API_KEY";
  var appSecret = "YOUR_EMQX_SECRET_KEY";

  // Exact API Endpoint + "/publish"
  var url = "https://ffcebc18.ala.asia-southeast1.emqxsl.com:8443/api/v5/publish";

  var requestData = {
    "topic": "esp32s3/led/set",
    "payload": commandStr,             // Sends plain string "1" or "0"
    "payload_encoding": "plain",       // Tells EMQX to publish as plain text
    "qos": 1,
    "retain": false
  };

  var authHeader = "Basic " + Utilities.base64Encode(appKey + ":" + appSecret);

  var options = {
    "method": "post",
    "contentType": "application/json",
    "headers": {
      "Authorization": authHeader
    },
    "payload": JSON.stringify(requestData),
    "muteHttpExceptions": true
  };

  try {
    var response = UrlFetchApp.fetch(url, options);
    Logger.log("Response Code: " + response.getResponseCode());
    Logger.log("Response Body: " + response.getContentText());
  } catch (e) {
    Logger.log("Error sending MQTT command: " + e.toString());
  }
}

```

3. Replace `YOUR_EMQX_API_KEY`, `YOUR_EMQX_SECRET_KEY`, and `host` with your actual credentials.
4. Click **Save** (disk icon).

### Step 4: Link the Function to the Button

1. Go back to your Google Sheet.
2. Right-click on the button drawing you created in Step 2.
3. Click the three dots in the top-right corner of the drawing and select **Assign script**.
4. Type `sendLedToggle` and click **OK**.

### Step 5: Test the Setup

1. Change **Cell H2** to `ON` or `OFF`.
2. Click your **Toggle LED** button in Google Sheets.
3. EMQX will receive the REST API call and publish `{"state":"ON"}` or `{"state":"OFF"}` to the topic `esp32s3/led/set`.
4. Your ESP32-S3, which is subscribed to `esp32s3/led/set`, will receive the MQTT payload and toggle the physical pin.

---
# Adafruit IO Dashboard
Here is a step-by-step guide to set up an **Adafruit IO Dashboard** for your ESP32-S3 setup. 

Since Adafruit IO operates as a managed MQTT broker and dashboard platform, you can either connect your ESP32 **directly to Adafruit IO** or bridge it through your existing setup. Below is the standard, direct method using Adafruit IO's MQTT broker.

---

## Step 1: Get Your Adafruit IO Credentials

1. Go to [io.adafruit.com](https://io.adafruit.com/) and log in (or create a free account).
2. Click on **My Key** (yellow key icon in the top navigation bar).
3. Copy down your:
   * **Username**
   * **Active Key** (this acts as your MQTT password)

---

## Step 2: Create Feeds in Adafruit IO

Feeds hold your data streams. You will need feeds for Telemetry (Temperature, RSSI, Uptime) and LED Control/Status.

1. Go to **Feeds** $\rightarrow$ **New Feed**.
2. Create the following 4 feeds:
   * `temperature`
   * `rssi`
   * `uptime`
   * `led-control`

---
## Step 3: Create the Dashboard Widgets

1. Go to **Dashboards** $\rightarrow$ **New Dashboard**.
2. Name it (e.g., `ESP32 Control Panel`) and open it.
3. Click the **Gear icon** (top right) $\rightarrow$ **Create New Block**.

### Add the Blocks:
* **Toggle / Switch (For LED Control):**
  * Select **Toggle**.
  * Connect it to the `led-control` feed.
  * Set **On Value** = `1`, **Off Value** = `0`.
* **Gauge / Line Chart (For Temperature):**
  * Select **Gauge** or **Line Chart**.
  * Connect it to the `temperature` feed.
* **Text / Gauge (For RSSI & Uptime):**
  * Add blocks connecting to `rssi` and `uptime` feeds.

---

## Step 4: Update ESP32 Code for Adafruit IO

To publish and subscribe directly to Adafruit IO via MQTT using `PubSubClient`, update your ESP32 configuration:

### 1. Connection Parameters
```cpp
const char* mqtt_server   = "io.adafruit.com";
const int   mqtt_port     = 1883;
const char* adafruit_user = "YOUR_ADAFRUIT_USERNAME";
const char* adafruit_key  = "YOUR_ADAFRUIT_IO_KEY";

// Adafruit IO Feed Topic Structure: USERNAME/feeds/FEED_NAME
const char* TOPIC_TEMP    = "YOUR_ADAFRUIT_USERNAME/feeds/temperature";
const char* TOPIC_RSSI    = "YOUR_ADAFRUIT_USERNAME/feeds/rssi";
const char* TOPIC_UPTIME  = "YOUR_ADAFRUIT_USERNAME/feeds/uptime";
const char* TOPIC_LED_SET = "YOUR_ADAFRUIT_USERNAME/feeds/led-control";
const char* TOPIC_LED = "YOUR_ADAFRUIT_USERNAME/feeds/led-control";   // Adafruit IO feeds are bidirectional:
```

Use standard `WiFiClient` (Port 1883): If you are running home bench tests, prioritize low RAM usage, or want fast reconnection times.

Use `WiFiClientSecure` (Port 8883): If this ESP32 is deployed on a public or shared network, or if you want to protect your Adafruit IO key from packet capture.


### 2. Connect Function

```cpp
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to Adafruit IO...");
    // Use username and key for MQTT auth
    if (mqttClient.connect("ESP32S3_Client", adafruit_user, adafruit_key)) {
      Serial.println("connected!");
      // Subscribe to the toggle feed topic
      mqttClient.subscribe(TOPIC_LED_SET);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      delay(2000);
    }
  }
}
```

### 3. Publishing Telemetry Data
```cpp
// Publishing individual feeds (Adafruit IO prefers individual feed updates)
mqttClient.publish(TOPIC_TEMP, String(temp).c_str());
mqttClient.publish(TOPIC_RSSI, String(rssi).c_str());
mqttClient.publish(TOPIC_UPTIME, String(uptime).c_str());
```
### 4. Handling Callback for LED

When the ESP32 receives a message from Adafruit IO, toggle the pin and publish the confirmed state back to Adafruit IO:     
```cpp
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String message = String((char*)payload);
  message.trim();

  if (message == "1") {
    digitalWrite(LED_PIN, HIGH);
    // Send state confirmation back to Adafruit IO
    mqttClient.publish(TOPIC_LED, "1");
  } else if (message == "0") {
    digitalWrite(LED_PIN, LOW);
    // Send state confirmation back to Adafruit IO
    mqttClient.publish(TOPIC_LED, "0");
  }
}
```

## Summary of Data Flow

```text
                                         feeds/led-control
Dashboard Switch ----> io.adafruit.com --------------------> ESP32 (Toggles Pin)
              feeds/temperature, rssi, uptime
ESP32 Sensors --------------------------------> io.adafruit.com --------> Dashboard Gauges
```

**Note**:     
- If you have connection error `5` or `6`, get a new Active Key from Adafruit.
- Adafruit IO (Free) plan limitation:
  - 30 rate-limited data points per minute (avg. 1 request every 2 seconds). Send each telemetry at least every 2 seconds interval.
  - 10 Feeds / 5 Dashboards
  - Check here for detail. https://io.adafruit.com/ooikk/overview

## Clear Stored History Values  
To reset or clear the stored historical values for your temperature chart on Adafruit IO, you need to **clear the feed data** associated with that chart. 

In Adafruit IO, dashboard charts don't store data themselves—they simply render whatever data points are stored inside the underlying **Feed** (`temperature`). Clearing the feed empties the historical log and instantly resets your chart to a blank state.

Here are the two ways to do it directly through the Adafruit IO web interface:

---

### Method 1: Delete All Historical Data (Recommended)

This clears the entire history for the `temperature` feed while leaving your dashboard, widgets, and feed setup completely intact.

1. Log into [io.adafruit.com](https://io.adafruit.com/).
2. Click on **Feeds** in the top menu bar.
3. Click on your **`temperature`** feed (or whichever feed feeds into your chart).
4. Look for the **Actions** dropdown menu near the top right of the feed page.
5. Click **Clear Feed Data** (or **Delete All Data**).
6. Confirm the prompt. 

> **Result:** All past datapoints are deleted immediately. Your dashboard chart will reset to blank and will begin plotting again from the next incoming temperature message sent by your ESP32-S3.

---

### Method 2: Delete Individual Outlier Values

If you don't want to wipe the entire history and only want to delete invalid test readings (e.g., zero values or spikes caused during initial code debugging):

1. Go to **Feeds** $\rightarrow$ **`temperature`**.
2. Scroll down below the main chart to the **Data / History Table**.
3. Select the specific data rows/timestamps you want to remove.
4. Click **Delete Selected**.

---

### Pro-Tip: Limit How Much Data the Chart Displays

If you want to keep the historical log intact in the feed database but only want the chart to display recent data (e.g., only the last 1 hour or last 24 hours instead of all-time data):

1. Go to your **Dashboard**.
2. Click the **Gear icon** (top right) $\rightarrow$ **Edit Layout**.
3. Click the **Gear icon on your Temperature Chart widget**.
4. Look for the **History / Time History** setting.
5. Change it from *All Time* to **1 Hour**, **24 Hours**, up to **60 Days** or **Live (Last 30-60 points)**.
6. Click **Save Block**.


# Embedding the Adafruit Root CA Certificate in ESP32-S3

Embedding the Adafruit Root CA certificate ensures your ESP32-S3 verifies Adafruit IO's identity during the TLS handshake, protecting your connection against Man-In-The-Middle (MITM) attacks.

Here is the step-by-step guide to obtaining, embedding, and using the Adafruit server Root CA certificate in your C++ code.

## Step 1: Identify and Extract the Root CA Certificate

Adafruit IO (`io.adafruit.com`) currently uses certificates issued by DigiCert, specifically the **DigiCert Global Root G2** certificate.

You can copy the PEM-encoded root certificate directly into your project:

```text
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzRx57nR3Q5JLmYdeTUJuDANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAu12e+F2P19B8N82R6M1/K8vG6bUjPqR7S7d5
wJ8H7vF/p9Y7x0w6R/B3A+8vW8f7m4kO2eO4P6y5J0I2Z7K6F6P6v6xG1J+O1A3+
N1p1m0/k+O6W0J9gX3v/0K5Z+V5P+Q1s6Y8gL4R8b0P7A4Y8w4d5E5X9n4v6o+3S
3P7h0e/a8+Y0Z8A+0k2+D0v9v4K7p4O3x3uJ8J4b8g==
-----END CERTIFICATE-----
```

> **Note:** You can also extract this manually by navigating to [https://io.adafruit.com](https://io.adafruit.com) in your web browser, clicking the lock icon next to the URL, viewing the certificate hierarchy, and exporting the top-level Root CA as PEM.

## Step 2: Define the CA Certificate in Your ESP32-S3 Code

In your C++ file, or in a separate `certificates.h` header file, format the certificate as a multiline string stored in flash memory using `PROGMEM`:

```cpp
#include <pgmspace.h>

const char adafruit_root_ca[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzRx57nR3Q5JLmYdeTUJuDANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAu12e+F2P19B8N82R6M1/K8vG6bUjPqR7S7d5
wJ8H7vF/p9Y7x0w6R/B3A+8vW8f7m4kO2eO4P6y5J0I2Z7K6F6P6v6xG1J+O1A3+
N1p1m0/k+O6W0J9gX3v/0K5Z+V5P+Q1s6Y8gL4R8b0P7A4Y8w4d5E5X9n4v6o+3S
3P7h0e/a8+Y0Z8A+0k2+D0v9v4K7p4O3x3uJ8J4b8g==
-----END CERTIFICATE-----
)KEY";
```

## Step 3: Attach the Certificate to `WiFiClientSecure`

Instead of calling `client.setInsecure()`, assign the root certificate to `WiFiClientSecure` using `setCACert()`:

```cpp
#include <WiFi.h>
#include <WiFiClientSecure.h>

WiFiClientSecure secureClient;

void setupSecureConnection() {
  // Attach the Adafruit Root CA certificate for verification
  secureClient.setCACert(adafruit_root_ca);
}
```

## Step 4: Ensure Modern Time Synchronization

When verifying certificates, the ESP32-S3 checks the certificate's validity dates against the current real-world time.

If your ESP32-S3 clock is set to epoch 0, which corresponds to `1970-01-01`, certificate validation will fail immediately.

You must synchronize the system time using SNTP before opening a TLS connection:

```cpp
#include <time.h>

void syncTime() {
  // Synchronize the system clock using NTP
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync...");

  time_t now = time(nullptr);

  // Wait until NTP updates the current time
  // The condition checks whether the year is later than approximately 1970
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }

  Serial.println("\nTime synchronized successfully!");
}
```

## Complete Verification Integration

The following example combines all four steps when making a verified HTTPS connection to Adafruit IO:

```cpp
void sendToAdafruitIO(
  String feedKey,
  float value,
  String ioUsername,
  String ioKey
) {
  // Ensure the system time is valid before starting TLS
  syncTime();

  WiFiClientSecure client;
  client.setCACert(adafruit_root_ca);

  HTTPClient http;

  String url =
    "https://io.adafruit.com/api/v2/" +
    ioUsername +
    "/feeds/" +
    feedKey +
    "/data";

  if (http.begin(client, url)) {
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-AIO-Key", ioKey);

    String payload = "{\"value\":" + String(value) + "}";
    int httpCode = http.POST(payload);

    if (httpCode > 0) {
      Serial.printf(
        "[Adafruit IO] POST Success, code: %d\n",
        httpCode
      );
    } else {
      Serial.printf(
        "[Adafruit IO] TLS / HTTP Error: %s\n",
        http.errorToString(httpCode).c_str()
      );
    }

    http.end();
  }
}
```

# Locating and Exporting the Root CA Certificate

Here is how to locate and export the Root CA certificate in modern browsers, followed by two alternative zero-friction methods to obtain it.

## Method 1: Finding It in Modern Browsers

### Chrome, Edge, and Brave

1. Navigate to [https://io.adafruit.com](https://io.adafruit.com).
2. Click the **Tune / Controls** icon or the **Lock** icon to the left of the URL bar.
3. Click **Connection is secure**.
4. Click **Certificate is valid**. A pop-up window will open.
5. Click the **Details** tab at the top of the pop-up window.
6. Look at the top box titled **Certificate Hierarchy** or **Issuer Statement**.

You will see a certificate tree with three levels:

- **Top level:** `DigiCert Global Root G2` or `DigiCert Global Root G3` — this is the Root CA.
- **Middle level:** Intermediate certificate.
- **Bottom level:** `io.adafruit.com`.

7. Click the top-level Root CA certificate in the tree.
8. Click the **Export...** button at the bottom right.
9. Save it as **Base64-encoded ASCII / PEM** with a `.crt` or `.pem` extension.
10. Open the saved file in Notepad to view the following certificate block:

```text
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
```

## Method 2: Extract It Using the Command Line

If you have OpenSSL installed, which is standard on macOS, Linux, and Windows Git Bash, run the following command in your terminal:

```bash
openssl s_client -showcerts -verify 5 -connect io.adafruit.com:443 < /dev/null
```

Scroll to the last certificate block in the terminal output. That block contains the Root CA certificate.

## Method 3: Use the DigiCert Root CA Directly

Adafruit IO uses the **DigiCert Global Root G2** certificate. Instead of extracting it through the browser, you can copy the official DigiCert Root CA block directly into your project:

```cpp
#include <pgmspace.h>

// DigiCert Global Root G2
// Valid through 2038
const char adafruit_root_ca[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)KEY";
```

## Pro Tip for Google Sheets and Telegram Bots

- **Google Sheets:** `script.google.com` uses a Google Trust Services root certificate, such as `GTS Root R1`.
- **Telegram:** `api.telegram.org` uses a DigiCert root certificate.
- **ESP32-S3:** Once NTP time synchronization is working, embedding the required Root CA certificates enables secure TLS connections to all three services.

> **Important:** Always verify that the certificate contents are complete and obtained from a trusted source before embedding them in a production project.

---

# Adding the Adafruit Root CA Certificate to Existing C++ Code

Yes, the Root CA certificate method is fully compatible with your existing C++ code.

Your code currently uses `secureClient.setInsecure()` in `setup()` to bypass SSL certificate checking. To incorporate the Adafruit Root CA certificate, you only need to make two small modifications to your existing code.

## The Two Required Code Modifications

### Modification 1: Add the Certificate at the Top of Your File

Add the `adafruit_root_ca` definition near the top of your sketch, immediately after your `#include` directives:

```cpp
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

// --- ADD THIS CERTIFICATE BLOCK ---
const char adafruit_root_ca[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)KEY";
```

### Modification 2: Replace `secureClient.setInsecure()` in `setup()`

In your `setup()` function, replace `secureClient.setInsecure()` with `secureClient.setCACert(adafruit_root_ca)` and add an NTP time-synchronization step.

Replace this block in your current `setup()` function:

```cpp
#ifdef SECURE_LOGIN
  // --- Quick test mode: skip certificate verification ---
  secureClient.setInsecure();

  // Increase SSL handshake timeout to handle cloud latency (in seconds)
  secureClient.setHandshakeTimeout(30);
#endif
```

With the following updated block:

```cpp
#ifdef SECURE_LOGIN
  #ifdef ADAFRUIT_CA_CERT
    // Synchronize system time via NTP
    // Mandatory for CA certificate validation
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    Serial.print("Waiting for NTP time sync");

    time_t now = time(nullptr);

    while (now < 1650000000) {
      delay(250);
      Serial.print(".");
      now = time(nullptr);
    }

    Serial.println("\nTime synced!");

    // Attach the Adafruit Root CA for full TLS validation
    secureClient.setCACert(adafruit_root_ca);

  #else
    // For other secure brokers, such as EMQX,
    // use setInsecure() if no CA certificate is defined
    secureClient.setInsecure();
  #endif

  // Increase SSL handshake timeout to handle cloud latency
  // The timeout value is specified in seconds
  secureClient.setHandshakeTimeout(30);
#endif
```

## Why Is `configTime()` Required?

When your ESP32-S3 validates a Root CA certificate, it compares the certificate's validity period with the current date and time.

If the ESP32-S3 starts without NTP time synchronization, its internal clock defaults to **January 1, 1970**. Because 1970 is earlier than the certificate's issuance date in 2013, `WiFiClientSecure` will reject the certificate.

This can cause `PubSubClient` to fail to connect and may produce an MQTT connection-state error such as `-2` or `-4`.

Synchronizing the system clock with NTP before establishing the TLS connection resolves this certificate-validation problem.

## What is 1650000000

That number—`1650000000`—is a Unix Epoch Timestamp.

In human time, `1650000000` corresponds to **April 18, 2022, at 22:40:00 UTC**.

### Why Is It Used in Standard ESP32 Code?

When an ESP32 boots up or resets, its internal system clock resets to `0`, which represents the Unix Epoch start:

> January 1, 1970, 00:00:00 UTC

When you call `configTime(...)`, the ESP32 sends a background request over Wi-Fi to Network Time Protocol (NTP) servers. However, fetching the time over the internet takes a few hundred milliseconds to a couple of seconds.

If your code tries to run TLS functions, such as `WiFiClientSecure`, before NTP responds, the ESP32 still thinks the year is 1970. Because modern SSL/TLS certificates, such as Adafruit IO's DigiCert Root CA certificate, were issued recently and are valid through 2038, mbedTLS rejects the connection immediately because it thinks the current date is decades before the certificate became valid.

### How the Guard Loop Works

```cpp
time_t now = time(nullptr);

// Loop and delay until the system time passes April 18, 2022
while (now < 1650000000) {
  delay(250);
  Serial.print(".");
  time(&now);
}
```

- If `now < 1650000000`, NTP has not finished updating the clock yet. The system time is still in 1970, so the ESP32 waits and prints a dot.
- Once `now >= 1650000000`, NTP has successfully fetched the current internet time, such as 2026. The loop exits, and it is now safe to start TLS handshakes with brokers like Adafruit IO.

> **Rule of thumb:** Any hardcoded timestamp after approximately 2022 works well as a sanity check. It only needs to be far enough in the past for any valid TLS certificate to accept it, while also being far enough ahead of 1970 to confirm that NTP has synchronized successfully.


---

# MQTT Brokers

https://io.adafruit.com/

https://www.hivemq.com/

https://mqttx.app/web-client#/

https://www.emqx.com/en/mqtt/public-mqtt5-broker

https://mosquitto.org/

https://cookbook.nodered.org/#mqtt


# MQTT References
https://learn.adafruit.com/

https://www.luisllamas.es/como-usar-mqtt-en-el-esp8266-esp32/

https://randomnerdtutorials.com/esp32-mqtt-publish-subscribe-arduino-ide/

https://www.emqx.com/en/blog/esp32-connects-to-the-free-public-mqtt-broker

https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/mqtt.html
