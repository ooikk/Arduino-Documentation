# ESP32 Communication with Telegram

An ESP32 can communicate with Telegram through a Telegram Bot. The ESP32 does not communicate directly with the Telegram phone app using Bluetooth or Wi-Fi Direct. Messages normally pass through Telegram's cloud servers using the HTTPS-based Bot API.

See the [Telegram Bot API documentation](https://core.telegram.org/bots/api).

```mermaid
flowchart LR
    ESP["ESP32 + sensors/outputs"]
    NET["Wi-Fi / Ethernet / Cellular"]
    TG["Telegram Bot API"]
    APP["Telegram app"]
    CLOUD["Optional cloud server"]

    ESP --> NET
    NET --> TG
    TG <--> APP
    ESP <--> CLOUD
    CLOUD <--> TG
```

## 1. Basic Communication Structure

A Telegram bot is created using `@BotFather`, which provides a secret bot token.

The ESP32 then sends HTTPS requests such as:

```text
[https://api.telegram.org/bot](https://api.telegram.org/bot)<BOT_TOKEN>/sendMessage
```

Typical information includes:

| Field | Meaning |
|---|---|
| `BOT_TOKEN` | Identifies and authenticates the bot. |
| `chat_id` | Identifies the permitted user, group, or channel. |
| `text` | Message content. |
| `update_id` | Identifies each received Telegram event. |

Telegram accepts HTTPS `GET` or `POST` requests. Parameters may be provided as URL query parameters, form data, JSON, or multipart form data for file uploads.

Responses are JSON objects containing fields such as:

- `ok`
- `result`
- `description`
- `error_code`

See the [Telegram Bot API request format](https://core.telegram.org/bots/api#making-requests).

## 2. ESP32-to-Telegram Communication Options

### Option A — One-Way Notification

The ESP32 sends information to Telegram but does not read commands.

```mermaid
flowchart LR
    Sensor["Sensor or event"] --> ESP["ESP32"]
    ESP -->|"HTTPS sendMessage"| TG["Telegram"]
    TG --> User["User"]
```

Example applications:

- High-temperature warning.
- Water-leak alarm.
- Door-open notification.
- Motion detection.
- Machine fault alert.
- Power-failure notification.
- Wi-Fi reconnection or reboot report.
- Periodic sensor report.

Typical API methods include:

- `sendMessage`
- `sendPhoto`
- `sendDocument`
- `sendLocation`

This is the easiest and most reliable starting point because the ESP32 only makes outgoing connections.

### Option B — Two-Way Control Using Polling

The ESP32 sends notifications and periodically asks Telegram whether new messages or commands have arrived.

```mermaid
sequenceDiagram
    participant U as Telegram user
    participant T as Telegram server
    participant E as ESP32

    U->>T: /led_on
    E->>T: getUpdates
    T-->>E: Command JSON
    E->>E: Validate user and control LED
    E->>T: sendMessage confirmation
    T-->>U: LED turned ON
```

Telegram provides the `getUpdates` method for this purpose. It supports short polling and long polling.

Each processed message should be acknowledged logically by advancing the offset beyond its `update_id`. Otherwise, the ESP32 may process the same update again.

`getUpdates` cannot be used while a webhook is active.

See the [Telegram getUpdates documentation](https://core.telegram.org/bots/api#getupdates).

Suitable commands include:

```text
/start
/status
/temperature
/led_on
/led_off
/restart
```

#### Advantages

- No public server is required.
- Works with an ESP32 behind a home router.
- Simple to implement in the Arduino IDE.
- Suitable for small personal IoT projects.

#### Limitations

- The ESP32 must repeatedly contact Telegram.
- Frequent polling increases network traffic and power consumption.
- Long polling can block other firmware activity if implemented poorly.
- Commands are not guaranteed to arrive instantly.
- Update IDs must be managed carefully to prevent duplicate actions.

For ESP32-S3 projects, this is generally the best first two-way architecture.

### Option C — Inline-Button Control

Instead of typing commands, the bot presents buttons:

```text
Living Room Light

[ ON ] [ OFF ]
[ STATUS ] [ AUTO ]
```

The ESP32 receives a `callback_query` when the user presses a button.

Applications include:

- Lighting control.
- Fan-speed selection.
- Irrigation duration.
- Alarm arming.
- Motor direction.
- Operating-mode selection.
- Alarm acknowledgement.

This approach teaches:

- JSON construction.
- Inline keyboard formatting.
- Callback handling.
- State-machine design.
- User-interface feedback.

The ESP32 should update or acknowledge the Telegram message after processing the button so the user knows whether the command succeeded.

### Option D — Webhook Through a Cloud Server

Telegram pushes every new command to a public HTTPS server. The server then communicates with the ESP32.

```mermaid
flowchart TD
    User["Telegram user"] --> TG["Telegram"]
    TG -->|"HTTPS webhook"| Server["Cloud server"]
    Server -->|"MQTT or HTTPS command"| ESP["ESP32"]
    ESP -->|"Result or telemetry"| Server
    Server --> TG
```

Possible server platforms include:

- Google Apps Script.
- Node-RED.
- Python Flask or FastAPI.
- Node.js.
- Firebase.
- AWS Lambda.
- Cloudflare Workers.
- Home Assistant.
- Raspberry Pi with a public endpoint.

Telegram sends webhook updates as HTTPS `POST` requests.

A webhook can include a secret token in the `X-Telegram-Bot-Api-Secret-Token` header so the server can verify that the request belongs to the configured webhook.

See the [Telegram setWebhook documentation](https://core.telegram.org/bots/api#setwebhook).

A webhook directly to an ESP32 is usually impractical because:

- The ESP32 is normally behind router NAT.
- It does not have a public HTTPS address.
- Certificate and server management are difficult.
- Internet providers may block incoming connections.
- Exposing a microcontroller directly to the Internet increases risk.

Use webhooks when managing multiple devices or when commands must be routed through a proper backend.

### Option E — MQTT-to-Telegram Gateway

The ESP32 uses MQTT while a server converts between MQTT topics and Telegram messages.

```mermaid
flowchart LR
    ESP["ESP32"] <--> Broker["MQTT broker"]
    Broker <--> Bridge["Node-RED / server"]
    Bridge <--> TG["Telegram Bot"]
    TG <--> User["Telegram user"]
```

Example MQTT topics:

```text
home/bathroom/temperature
home/bathroom/leak
home/bathroom/fan/set
home/bathroom/fan/state
```

#### Advantages

- Telegram code is removed from the ESP32.
- Multiple ESP32 devices can be supported.
- Telemetry and commands are separated cleanly.
- Easier integration with dashboards, databases, and automation.
- MQTT retained messages can store the latest desired state.

This is an educational architecture for advanced IoT classes because it introduces:

- Message brokers.
- Topic design.
- Publish/subscribe communication.
- Cloud integration.

### Option F — Google Sheets or Apps Script Bridge

Since you are already working with ESP32 and Google Sheets, you can extend the same architecture:

```mermaid
flowchart LR
    ESP["ESP32"] -->|"HTTPS telemetry"| GAS["Google Apps Script"]
    GAS --> Sheets["Google Sheets"]
    GAS --> TG["Telegram alert"]
    TG --> User["Telegram user"]
```

Examples:

- Log temperature in Google Sheets.
- Send a Telegram warning when temperature exceeds a limit.
- Record who acknowledged an alarm.
- Retrieve the last command from a sheet.
- Generate a daily operating summary.

This teaches:

- HTTP `POST`.
- JSON.
- Cloud scripting.
- Data logging.
- Notification rules.

However, Apps Script redirects and execution limits must be handled carefully.

## 3. Internet Connection Choices

Telegram communication requires Internet access, but the ESP32 can reach the Internet through different hardware.

| Connection | Hardware | Suitable Application |
|---|---|---|
| Wi-Fi | Built into most ESP32 boards. | Home automation and classroom projects. |
| Ethernet | W5500, LAN8720, or Ethernet ESP32 board. | Industrial or fixed equipment. |
| Cellular | SIM7600, A7670, SIM7000, or similar modem. | Remote monitoring without Wi-Fi. |
| Wi-Fi gateway | ESP32 connects through another router or device. | Portable installations. |
| LoRa-to-gateway | LoRa sensor communicates with an Internet-connected gateway. | Long-range field sensing. |

The Telegram protocol remains HTTPS regardless of whether the underlying Internet connection is Wi-Fi, Ethernet, or cellular.

## 4. Arduino Implementation Choices

### Direct HTTPS Requests

Use:

```cpp
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
```

#### Advantages

- Students learn the actual Bot API.
- Full control over request formatting and error handling.
- No dependency on a Telegram-specific library.

#### Disadvantages

- More code is required.
- URL encoding, JSON parsing, and file uploads require care.
- Polling and update-offset management must be implemented manually.

This approach is best for advanced learning.

### Universal Arduino Telegram Bot

The library supports:

- Receiving and sending messages.
- Reply keyboards.
- Inline keyboards.
- Photos.
- Locations.
- Long polling.

It uses an SSL client and ArduinoJson.

See the [Universal Arduino Telegram Bot repository](https://github.com/witnessmenow/Universal-Arduino-Bot).

Good for:

- Introductory projects.
- LED control.
- Sensor monitoring.
- ESP32-CAM.
- Quick prototypes.

### AsyncTelegram2

[AsyncTelegram2](https://github.com/cotestatnt/AsyncTelegram2) is intended for applications where Telegram communication should not block the rest of the firmware.

This is useful when the ESP32 must continue servicing displays, sensors, motors, or time-critical tasks while checking Telegram.

Good for:

- Multitasking systems.
- FreeRTOS-based projects.
- Responsive displays.
- Multiple sensors.
- Projects where blocking HTTPS calls create delays.

### CTBot

[CTBot](https://github.com/shurillu/CTBot) provides a relatively simple interface for ESP8266 and ESP32. It supports commands and keyboard-based interaction.

### Recommended Teaching Sequence

For a new advanced ESP32-S3 lesson, teach the following sequence:

1. Raw `HTTPClient` for understanding the protocol.
2. A Telegram library for rapid implementation.
3. An asynchronous or cloud architecture for the final project.

## 5. Recommended Learning Projects

| Level | Project | Communication Learned | Main Engineering Concepts |
|---:|---|---|---|
| 1 | Push-button Telegram notifier | ESP32 → Telegram | GPIO input, debounce, HTTPS, `sendMessage`. |
| 2 | Temperature and humidity monitor | ESP32 → Telegram | Sensor interface, formatting, and thresholds. |
| 3 | Remote LED and relay controller | Telegram → ESP32 | `getUpdates`, JSON parsing, and command validation. |
| 4 | Inline-button home controller | Two-way interactive | Callback queries, state machines, and UI feedback. |
| 5 | ESP32-CAM security alert | Photo upload | Motion sensing, JPEG buffers, and multipart transfer. |
| 6 | Water-leak monitoring system | Event and acknowledgement | Alarm latching, escalation, and recovery. |
| 7 | Google Sheets plus Telegram | ESP32 → cloud → Telegram | REST API, data logging, and rule processing. |
| 8 | MQTT multi-device control centre | Telegram ↔ server ↔ ESP32 | MQTT, device identity, and scalable architecture. |
| 9 | Cellular remote alarm | ESP32 + modem → Telegram | AT commands, cellular reliability, and reconnection. |
| 10 | Dependable IoT capstone | Full two-way system | TLS, NVS, watchdog, retries, OTA, and security. |


**Learning Roadmap and Practical Projects**     

| Project Level | Application Idea | Hardware Required | Key Communication Concepts Learned |
|---|---|---|---|
| **Beginner** | Remote Relay Controller | ESP32, relay module, and LED. | Basic polling, parsing text command strings such as `/turn_on` and `/status`, and filtering by an allowlisted chat ID. |
| **Intermediate** | Environmental Alarm System | ESP32 with a DHT22 or BME280 sensor. | Threshold-triggered conditions, formatting HTML or Markdown messages, and sending automated alerts. |
| **Intermediate** | Interactive Control Dashboard | ESP32 and status LEDs. | Building inline keyboards and interactive tap buttons inside a Telegram chat. |
| **Advanced** | ESP32-CAM Motion Security | ESP32-CAM and PIR motion sensor. | HTTP `multipart/form-data` binary image transfer for sending JPEG photos when motion is detected. |
| **Hybrid** | MQTT-to-Telegram Gateway | ESP32 and Raspberry Pi running n8n or Node-RED. | Decoupled telemetry over MQTT, webhook handling, and multi-node automation. |


## Best Introductory Project

A good first project is an ESP32 temperature monitor with LED control.

### Functions

- `/start` displays available commands.
- `/status` returns temperature, RSSI, uptime, and LED state.
- `/led_on` and `/led_off` control an LED.
- A high-temperature condition automatically sends an alert.
- Only an authorised `chat_id` can issue commands.
- An inline keyboard provides `ON`, `OFF`, and `STATUS` buttons.

This single project covers most essential Telegram communication concepts without requiring excessive hardware.

## Best Advanced Project

### Smart Equipment Monitoring and Maintenance Assistant

Possible functions include:

- Periodically measure temperature, current, and vibration.
- Report abnormal conditions to a maintenance Telegram group.
- Allow an authorised person to acknowledge an alarm.
- Log data to Google Sheets or an MQTT database.
- Send a photo from an ESP32-CAM.
- Report firmware version, reset reason, and uptime.
- Permit safe configuration changes through Telegram.
- Store settings and processed command IDs in NVS.
- Use a watchdog and reconnection state machine.

## 6. Security and Reliability Practices

- Never publish the bot token in GitHub repositories or screenshots.
- Restrict commands using an allowlist of authorised `chat_id` values.
- Use TLS certificate validation; avoid `client.setInsecure()` in a finished system.
- Synchronise the ESP32 system time before validating TLS certificates.
- Do not accept arbitrary text as GPIO numbers, durations, or configuration values.
- Store or correctly advance the last `update_id` to prevent repeated commands.
- Confirm the actual output state after a command instead of merely replying `"success"`.
- Add timeouts, retry limits, and Wi-Fi reconnection handling.
- Avoid using Telegram as a safety-critical emergency-stop system.
- Require additional confirmation for dangerous commands such as reboot, door unlock, motor operation, or OTA update.
- Keep Telegram processing separate from time-critical control loops.

Telegram retains uncollected bot updates for no longer than 24 hours. Polling and webhook reception are mutually exclusive.

See the [Telegram update delivery documentation](https://core.telegram.org/bots/api#getting-updates).

## Recommended Advanced ESP32-S3 Lesson

For an advanced ESP32-S3 class, combine Projects 3, 4, and 6 into one lesson:

```text
Secure two-way Telegram control
+ Sensor alarms
+ Inline buttons
+ Command authorisation
```
---
# Google Apps Script: Option F vs. Option D

The two options use the same Google Apps Script platform, but Apps Script plays a different role:

- **Option F:** The ESP32 initiates communication by sending telemetry to Google Apps Script. Apps Script logs it and optionally sends a Telegram alert.
- **Option D:** Telegram initiates communication by sending a webhook to Google Apps Script. Apps Script stores the command, and the ESP32 later retrieves it.

The important point is that Google Apps Script normally cannot directly push a command into an ESP32 behind a router. The ESP32 must still poll Google Apps Script, MQTT, Firebase, or another command service.

## 1. Communication-Flow Difference

### Option F — ESP32 Telemetry Bridge

```mermaid
sequenceDiagram
    participant E as ESP32
    participant G as Google Apps Script
    participant S as Google Sheets
    participant T as Telegram

    E->>G: POST telemetry
    G->>S: Append sensor data
    G->>G: Evaluate alarm condition
    G->>T: sendMessage if alarm
    G-->>E: JSON response
```

The ESP32 is the initiator.

Typical ESP32 payload:

```json
{
  "source": "esp32",
  "type": "telemetry",
  "deviceId": "ESP32_01",
  "temperature": 45.5,
  "rssi": -73,
  "uptime": 100,
  "led-state": 0
}
```

### Option D — Telegram Webhook Command Bridge

```mermaid
sequenceDiagram
    participant U as Telegram user
    participant T as Telegram
    participant G as Google Apps Script
    participant E as ESP32

    U->>T: /led_on
    T->>G: POST webhook update
    G->>G: Store pending command
    G-->>T: HTTP success
    E->>G: GET pending command
    G-->>E: led-control = 1
    E->>E: Switch LED ON
    E->>G: POST acknowledgement
    G->>T: Command completed
    T-->>U: LED is ON
```

Telegram is the first initiator, but the ESP32 still initiates its own connection to collect the command.

## 2. Main Code Differences

| Function | Option F: Telemetry Bridge | Option D: Webhook Bridge |
|---|---:|---:|
| ESP32 sends telemetry | Yes | Optional |
| ESP32 contacts Telegram directly | No | No |
| ESP32 polls Google Apps Script for commands | Optional | Usually required |
| Google Apps Script receives ESP32 JSON | Yes | Optional |
| Google Apps Script receives Telegram JSON | No | Yes |
| Google Apps Script parses `update_id` and message | No | Yes |
| Google Apps Script checks Telegram `chat_id` | Only for alert destination | Required for command authorisation |
| Google Apps Script stores pending commands | Usually not | Yes |
| Google Apps Script sends Telegram messages | Alarm notification | Command response and acknowledgement |
| Telegram webhook configured | No | Yes |
| Bot token stored in ESP32 | No | No |
| Bot token stored in Google Apps Script | Yes | Yes |

## 3. Option F Code Structure

### ESP32 Code

The existing ESP32 design is already close to Option F:

```cpp
void sendTelemetry() {
  JsonDocument doc;

  doc["source"] = "esp32";
  doc["type"] = "telemetry";
  doc["deviceId"] = "ESP32_01";
  doc["status"] = "ONLINE";
  doc["temperature"] = temperature;
  doc["rssi"] = WiFi.RSSI();
  doc["uptime"] = millis() / 1000;
  doc["led-state"] = digitalRead(LED_PIN);

  String payload;
  serializeJson(doc, payload);

  WiFiClientSecure client;
  client.setCACert(GOOGLE_ROOT_CA);

  HTTPClient http;

  http.setFollowRedirects(
      HTTPC_FORCE_FOLLOW_REDIRECTS
  );

  http.begin(client, GAS_URL);
  http.addHeader(
      "Content-Type",
      "application/json"
  );

  int httpCode = http.POST(payload);
  String response = http.getString();

  Serial.printf(
      "HTTP response: %d\n",
      httpCode
  );

  Serial.println(response);

  http.end();
}
```

The ESP32:

1. Reads sensors.
2. Constructs telemetry JSON.
3. Sends it to Google Apps Script using `POST`.
4. Parses the Google Apps Script response.
5. Optionally applies a command returned with that response.

Example response:

```json
{
  "status": "success",
  "led-control": 1
}
```

### Google Apps Script Code

```javascript
const SHEET_ID = "YOUR_SHEET_ID";

function doPost(e) {
  try {
    const data = JSON.parse(e.postData.contents);

    if (data.source !== "esp32") {
      return jsonResponse({
        status: "error",
        message: "Unknown source"
      });
    }

    const sheet =
      SpreadsheetApp
        .openById(SHEET_ID)
        .getSheetByName("Telemetry");

    sheet.appendRow([
      new Date(),
      data.deviceId,
      data.status,
      data.temperature,
      data.rssi,
      data.uptime,
      data["led-state"]
    ]);

    if (Number(data.temperature) >= 45) {
      sendTelegram(
        "High-temperature warning\n" +
        "Device: " + data.deviceId + "\n" +
        "Temperature: " + data.temperature + " °C"
      );
    }

    return jsonResponse({
      status: "success",
      "led-control": readLedControlFromSheet()
    });

  } catch (error) {
    return jsonResponse({
      status: "error",
      message: error.message
    });
  }
}
```

Telegram is only used as an output notification channel:

```javascript
function sendTelegram(message) {
  const properties =
    PropertiesService.getScriptProperties();

  const token =
    properties.getProperty("BOT_TOKEN");

  const chatId =
    properties.getProperty("CHAT_ID");

  const url =
    "[https://api.telegram.org/bot](https://api.telegram.org/bot)" +
    token +
    "/sendMessage";

  const payload = {
    chat_id: chatId,
    text: message
  };

  UrlFetchApp.fetch(url, {
    method: "post",
    contentType: "application/json",
    payload: JSON.stringify(payload),
    muteHttpExceptions: true
  });
}
```

## 4. Option D Code Structure

Option D requires Google Apps Script to understand the Telegram webhook JSON and convert Telegram commands into commands that the ESP32 can retrieve.

### Telegram Webhook JSON

When the user sends `/led_on`, Google Apps Script receives something similar to:

```json
{
  "update_id": 123456789,
  "message": {
    "message_id": 20,
    "from": {
      "id": 11223344
    },
    "chat": {
      "id": 11223344,
      "type": "private"
    },
    "text": "/led_on"
  }
}
```

### Google Apps Script Webhook Handler

```javascript
const AUTHORISED_CHAT_ID = "11223344";

function doPost(e) {
  try {
    const data =
      JSON.parse(e.postData.contents || "{}");

    // Telegram webhook message
    if (data.update_id !== undefined) {
      return handleTelegramUpdate(data, e);
    }

    // ESP32 telemetry or acknowledgement
    if (data.source === "esp32") {
      return handleEsp32Request(data);
    }

    return jsonResponse({
      status: "error",
      message: "Unknown request"
    });

  } catch (error) {
    return jsonResponse({
      status: "error",
      message: error.message
    });
  }
}
```

Google Apps Script runs `doPost(e)` when it receives an HTTP `POST`. The request body is available through `e.postData.contents`.

See the [Google Apps Script web-app documentation](https://developers.google.com/apps-script/guides/web).

### Parse Telegram Commands

```javascript
function handleTelegramUpdate(update, e) {
  const message = update.message;

  if (!message || !message.text) {
    return jsonResponse({
      ok: true
    });
  }

  const chatId = String(message.chat.id);
  const command =
    message.text.trim().toLowerCase();

  if (chatId !== AUTHORISED_CHAT_ID) {
    sendTelegramTo(
      chatId,
      "Unauthorised user."
    );

    return jsonResponse({
      ok: true
    });
  }

  switch (command) {
    case "/led_on":
      queueCommand(
        "ESP32_01",
        "SET_LED",
        1
      );

      sendTelegramTo(
        chatId,
        "LED ON command queued. " +
        "Waiting for ESP32."
      );
      break;

    case "/led_off":
      queueCommand(
        "ESP32_01",
        "SET_LED",
        0
      );

      sendTelegramTo(
        chatId,
        "LED OFF command queued. " +
        "Waiting for ESP32."
      );
      break;

    case "/status":
      sendStoredStatus(
        chatId,
        "ESP32_01"
      );
      break;

    default:
      sendTelegramTo(
        chatId,
        "Available commands:\n" +
        "/led_on\n" +
        "/led_off\n" +
        "/status"
      );
  }

  return jsonResponse({
    ok: true
  });
}
```

### Store the Command

For a simple single-device demonstration, Script Properties can be used:

```javascript
function queueCommand(
  deviceId,
  command,
  value
) {
  const commandObject = {
    commandId: Utilities.getUuid(),
    deviceId: deviceId,
    command: command,
    value: value,
    state: "PENDING",
    timestamp: new Date().toISOString()
  };

  PropertiesService
    .getScriptProperties()
    .setProperty(
      "COMMAND_" + deviceId,
      JSON.stringify(commandObject)
    );
}
```

For multiple ESP32 devices, a Google Sheet command queue is better:

| Command ID | Device ID | Command | Value | State | Created | Completed |
|---|---|---|---:|---|---|---|
| `abc-123` | `ESP32_01` | `SET_LED` | `1` | `PENDING` | `14:20:10` | |
| `abc-124` | `ESP32_02` | `SET_FAN` | `0` | `DONE` | `14:21:15` | `14:21:18` |

## 5. ESP32 Command Polling for Option D

The ESP32 does not call Telegram's `getUpdates`. Instead, it asks Google Apps Script for a pending command:

```text
GET GAS_URL?action=getCommand&deviceId=ESP32_01
```

### ESP32 Polling Code

```cpp
void checkPendingCommand() {
  String url =
      String(GAS_URL) +
      "?action=getCommand" +
      "&deviceId=ESP32_01";

  WiFiClientSecure client;
  client.setCACert(GOOGLE_ROOT_CA);

  HTTPClient http;

  http.setFollowRedirects(
      HTTPC_STRICT_FOLLOW_REDIRECTS
  );

  http.begin(client, url);

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String response = http.getString();

    JsonDocument doc;

    DeserializationError error =
        deserializeJson(doc, response);

    if (!error && doc["status"] == "command") {
      String command =
          doc["command"];

      int value =
          doc["value"];

      String commandId =
          doc["commandId"];

      if (command == "SET_LED" &&
          (value == 0 || value == 1)) {

        digitalWrite(
            LED_PIN,
            value ? HIGH : LOW
        );

        sendCommandAcknowledgement(
            commandId,
            true,
            value
        );
      }
    }
  }

  http.end();
}
```

### Google Apps Script `doGet()`

```javascript
function doGet(e) {
  const action =
    e.parameter.action || "";

  if (action === "getCommand") {
    return getPendingCommand(
      e.parameter.deviceId
    );
  }

  return jsonResponse({
    status: "error",
    message: "Unknown action"
  });
}
```

```javascript
function getPendingCommand(deviceId) {
  const value =
    PropertiesService
      .getScriptProperties()
      .getProperty(
        "COMMAND_" + deviceId
      );

  if (!value) {
    return jsonResponse({
      status: "no-command"
    });
  }

  const command =
    JSON.parse(value);

  if (command.state !== "PENDING") {
    return jsonResponse({
      status: "no-command"
    });
  }

  return jsonResponse({
    status: "command",
    commandId: command.commandId,
    command: command.command,
    value: command.value
  });
}
```

## 6. ESP32 Acknowledgement

It is better not to tell the Telegram user `"LED is ON"` immediately after Google Apps Script receives `/led_on`.

At that point, Google Apps Script has only queued the command.

The sequence should be:

1. Telegram user sends `/led_on`.
2. Google Apps Script replies: `"Command queued."`
3. ESP32 retrieves and executes the command.
4. ESP32 sends an acknowledgement.
5. Google Apps Script sends: `"ESP32 confirmed: LED is ON."`

### ESP32 Acknowledgement Payload

```json
{
  "source": "esp32",
  "type": "command-ack",
  "deviceId": "ESP32_01",
  "commandId": "abc-123",
  "success": true,
  "led-state": 1
}
```

### Google Apps Script Request Handler

```javascript
function handleEsp32Request(data) {
  if (data.type === "telemetry") {
    return processTelemetry(data);
  }

  if (data.type === "command-ack") {
    return processCommandAcknowledgement(data);
  }

  return jsonResponse({
    status: "error",
    message: "Unknown ESP32 message type"
  });
}
```

```javascript
function processCommandAcknowledgement(data) {
  PropertiesService
    .getScriptProperties()
    .deleteProperty(
      "COMMAND_" + data.deviceId
    );

  const result = data.success
    ? "Command completed successfully."
    : "ESP32 failed to execute the command.";

  sendTelegram(
    result +
    "\nDevice: " + data.deviceId +
    "\nLED state: " + data["led-state"]
  );

  return jsonResponse({
    status: "acknowledgement-recorded"
  });
}
```

## 7. Combined Google Apps Script Design

You do not necessarily need two separate Google Apps Script deployments.

One deployment can support both options by examining the incoming JSON:

```javascript
function doPost(e) {
  const data =
    JSON.parse(e.postData.contents || "{}");

  if (data.update_id !== undefined) {
    return handleTelegramUpdate(
      data,
      e
    );
  }

  if (data.source === "esp32") {
    return handleEsp32Request(data);
  }

  return jsonResponse({
    status: "error",
    message: "Unrecognised request"
  });
}
```

The complete combined flow becomes:

```mermaid
flowchart TD
    ESP["ESP32"]
    GAS["Google Apps Script"]
    Sheet["Google Sheets"]
    TG["Telegram"]

    ESP -->|"POST telemetry/ack"| GAS
    GAS -->|"Log telemetry"| Sheet
    GAS -->|"Send alert/result"| TG
    TG -->|"Webhook command"| GAS
    GAS -->|"Store command"| Sheet
    ESP -->|"GET pending command"| GAS
```

This is the most logical extension of an existing Google Sheets project.

## 8. Webhook Setup Requirement

For Option D, register the Google Apps Script deployment URL with Telegram:

```text
[https://api.telegram.org/bot](https://api.telegram.org/bot)<BOT_TOKEN>/setWebhook?url=<GAS_WEB_APP_URL>
```

After registration:

- Telegram sends updates to Google Apps Script.
- The ESP32 must stop using Telegram's `getUpdates`.
- Google Apps Script becomes the only Telegram update receiver.

Telegram does not permit `getUpdates` and webhook delivery to operate simultaneously.

See the [Telegram webhook documentation](https://core.telegram.org/bots/api#setwebhook).

Check the webhook state with:

```text
[https://api.telegram.org/bot](https://api.telegram.org/bot)<BOT_TOKEN>/getWebhookInfo
```

## 9. Relevance of the Current 302 Redirect Issue

For both designs, an ESP32 calling a Google Apps Script web app can encounter a redirect from:

```text
script.google.com
```

to:

```text
script.googleusercontent.com
```

Google documents that `ContentService` output is served from a different URL for security reasons.

See the [Google Content Service documentation](https://developers.google.com/apps-script/reference/content/content-service).

The difference is:

| Design | Redirect Handling Applies To |
|---|---|
| Option F | ESP32 telemetry `POST` response. |
| Option D | ESP32 command `GET` and acknowledgement `POST`. |

The Telegram-to-Google Apps Script webhook does not use the ESP32 redirect-handling code.

For the ESP32 implementation, automatic redirect handling is preferable for `GET` requests:

```cpp
http.setFollowRedirects(
    HTTPC_STRICT_FOLLOW_REDIRECTS
);
```

For the Google Apps Script `POST` request:

```cpp
http.setFollowRedirects(
    HTTPC_FORCE_FOLLOW_REDIRECTS
);
```

This avoids depending on `http.getLocation()`, which showed inconsistent behavior in the observed implementation.

## Recommendation

Keep the existing Option F structure and add the Option D functions:

1. Continue posting telemetry from the ESP32 to Google Apps Script.
2. Add Telegram webhook processing to `doPost()`.
3. Add a command queue in Google Sheets or Script Properties.
4. Let the ESP32 call `doGet()` every 3–5 seconds.
5. Add an ESP32 command acknowledgement.
6. Let Google Apps Script send the final execution result to Telegram.

Therefore, Option D does not replace the existing Google Sheets communication. It adds a Telegram-to-Google Apps Script command path on top of it.
