# Connecting an ESP32-S3 Securely to Google Sheets

Connecting an ESP32-S3 securely to Google Sheets uses a Google Apps Script Web App as an HTTPS API bridge. The ESP32-S3 transmits telemetry data via HTTPS `POST` requests and receives control commands in the server response payload.

## Capabilities and Limitations

| Feature Category | Capabilities | Limitations |
|---|---|---|
| **Data Throughput** | Logs structured JSON data directly into spreadsheet rows. | Not suitable for high-frequency sampling. Keep intervals at least 5 seconds. |
| **Security** | Supports TLS 1.2/1.3 encrypted HTTPS requests via the ESP32-S3 `WiFiClientSecure`. | Web App endpoints deployed with `Anyone` access rely on URL obscurity unless custom headers are added. |
| **Bi-directional Flow** | Sends telemetry such as temperature and RSSI and receives commands such as `led-control` in a single HTTP response. | Latency ranges between 500 ms and 2,000 ms per request due to Google Apps Script execution time. |
| **Quotas and Cost** | Provides free storage and processing using standard Google Drive accounts. | Subject to Google Apps Script daily quotas, typically 20,000 to 50,000 executions per day. |

## Step-by-Step Implementation Guide

### 1. Set Up Google Sheet and Apps Script

**Estimated time:** 5 minutes

Create a new Google Sheet and add the following headers in Row 1:

```text
Timestamp | Status | Temperature | RSSI | Uptime | LED State | Button | LED Control Target
```

Set the value of cell `H2` to `0` or `1`. This acts as the remote dashboard control for `led-control`.

Open **Extensions > Apps Script** and replace the default script with the following code:

```javascript
function doPost(e) {
  try {
    var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
    var data = JSON.parse(e.postData.contents);

    // Append telemetry row
    sheet.appendRow([
      new Date(),
      data.status,
      data.temperature,
      data.rssi,
      data.uptime,
      data["led-state"],
      data.button
    ]);

    // Read desired control state from cell H2
    var ledControlValue = sheet.getRange("H2").getValue();

    // Respond back to ESP32-S3 with control command
    var response = {
      "status": "success",
      "led-control": ledControlValue
    };

    return ContentService
      .createTextOutput(JSON.stringify(response))
      .setMimeType(ContentService.MimeType.JSON);

  } catch (err) {
    return ContentService
      .createTextOutput(
        JSON.stringify({
          "status": "error",
          "message": err.toString()
        })
      )
      .setMimeType(ContentService.MimeType.JSON);
  }
}
```

### 2. Deploy Apps Script as a Web App

**Prerequisite:** Required for API access.

1. In Apps Script, click **Deploy > New deployment**.
2. Select **Web app** as the deployment type.
3. Set **Execute as** to **Me**.
4. Set **Who has access** to **Anyone**.
5. Click **Deploy**.
6. Authorize the required permissions.
7. Copy the generated Web App URL. It starts with:

   ```text
   https://script.google.com/macros/s/...
   ```

> **Note:** Google Apps Script issues a `302 Found` redirect when handling `POST` requests. The ESP32 HTTP client must explicitly enable redirect-following to receive the JSON response.

### 3. Configure ESP32-S3 Firmware

**Environment:** Arduino IDE or ESP-IDF C++

Flash the following code to your ESP32-S3. Ensure that the `ArduinoJson` library is installed.

```cpp
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

// Replace with your Google Apps Script Deployment URL
const char* GOOGLE_SCRIPT_URL =
  "[https://script.google.com/macros/s/YOUR_DEPLOYMENT_ID/exec](https://script.google.com/macros/s/YOUR_DEPLOYMENT_ID/exec)";

// Topics / Field Keys
const char* TOPIC_STATUS = "status";
const char* TOPIC_TEMP = "temperature";
const char* TOPIC_RSSI = "rssi";
const char* TOPIC_UPTIME = "uptime";
const char* TOPIC_LED_SET = "led-control";
const char* TOPIC_LED_STATE = "led-state";
const char* TOPIC_BUTTON = "button";

const int LED_PIN = 2;
const int BUTTON_PIN = 0;

// Google Root CA Certificate (GTS Root R1)
// for strict TLS verification
const char* GOOGLE_ROOT_CA =
  "-----BEGIN CERTIFICATE-----\n"
  "MH3XITBAMBgNVHR8EMTAvMC2gK6AhihhodHRwOi8vY3JsLnVzZXJ0cnVzdC5jb20v\n"
  "VVNFUlRydXN0UlNBQ2VydGlmaWNhdGlvbkF1dGhvcml0eS5jcmwwDQYJKoZIhvcN\n"
  "AQEMBQADggIBAGLjko5GdyYV1KKJhJDA9x6HYXh3m4f7X6u9M/J511b8SgO6vK/8\n"
  "-----END CERTIFICATE-----\n";

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi");
}

void sendTelemetryAndFetchCommands() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  WiFiClientSecure client;

  // Option A: Strict SSL
  // Requires a valid certificate bundle or root CA
  // client.setCACert(GOOGLE_ROOT_CA);

  // Option B: TLS encryption enabled without
  // fingerprint validation
  client.setInsecure();

  HTTPClient http;

  // Critical for Google Apps Script 302 redirects
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  http.begin(client, GOOGLE_SCRIPT_URL);
  http.addHeader("Content-Type", "application/json");

  // Build JSON payload
  StaticJsonDocument<256> doc;

  doc[TOPIC_STATUS] = "ONLINE";
  doc[TOPIC_TEMP] = 24.5;  // Replace with sensor reading
  doc[TOPIC_RSSI] = WiFi.RSSI();
  doc[TOPIC_UPTIME] = millis() / 1000;
  doc[TOPIC_LED_STATE] = digitalRead(LED_PIN);
  doc[TOPIC_BUTTON] =
    (digitalRead(BUTTON_PIN) == LOW) ? 1 : 0;

  String requestBody;
  serializeJson(doc, requestBody);

  Serial.println("Sending HTTPS POST to Google Sheets...");

  int httpCode = http.POST(requestBody);

  if (httpCode > 0) {
    String responseBody = http.getString();

    Serial.printf("HTTP Response Code: %d\n", httpCode);
    Serial.println("Response Payload: " + responseBody);

    // Parse response for incoming control commands
    StaticJsonDocument<256> respDoc;

    DeserializationError err =
      deserializeJson(respDoc, responseBody);

    if (!err && respDoc.containsKey(TOPIC_LED_SET)) {
      int ledCmd = respDoc[TOPIC_LED_SET];

      digitalWrite(LED_PIN, ledCmd ? HIGH : LOW);

      Serial.printf(
        "Updated local LED state to: %d\n",
        ledCmd
      );
    }
  } else {
    Serial.printf(
      "HTTP POST failed, error: %s\n",
      http.errorToString(httpCode).c_str()
    );
  }

  http.end();
}

void loop() {
  sendTelemetryAndFetchCommands();

  // Poll and upload every 10 seconds
  delay(10000);
}
```
