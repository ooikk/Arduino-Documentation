# Connection Options for ESP32-S3 with ThingsBoard

[ThingsBoard](https://thingsboard.io/) supports several architecture patterns for connecting an ESP32-S3 board to the platform.

```text
+---------------------------------------------------------------------------------+
|                           CONNECTION ARCHITECTURES                               |
+---------------------------------------------------------------------------------+
|                                                                                 |
|  [ Direct Connection ]                                                          |
|  ESP32-S3 --- Wi-Fi: MQTT / HTTP / CoAP ---> ThingsBoard Cloud (PaaS)           |
|                                                                                 |
|  [ IoT Gateway Connection: Local Sensor Network ]                               |
|  Sensor Nodes --- BLE / ESP-NOW ---> ESP32-S3 Gateway ---> ThingsBoard Cloud    |
|                                                                                 |
|  [ External Gateway Connection ]                                                |
|  ESP32-S3 Node --- BLE / UART / RS485 ---> Raspberry Pi Gateway --->            |
|                                             ThingsBoard                          |
+---------------------------------------------------------------------------------+
```

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

If a deployment contains multiple low-power sensor nodes or legacy protocols, the ESP32-S3 can be used inside a gateway architecture.

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
| `PubSubClient` | Nick O'Leary | MQTT communication |
| `ArduinoJson` | Benoit Blanchon | JSON telemetry payload generation |

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
