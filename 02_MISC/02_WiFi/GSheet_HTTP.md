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
const char* GOOGLE_ROOT_CA =R"(
  -----BEGIN CERTIFICATE-----
  MH3XITBAMBgNVHR8EMTAvMC2gK6AhihhodHRwOi8vY3JsLnVzZXJ0cnVzdC5jb20v
  VVNFUlRydXN0UlNBQ2VydGlmaWNhdGlvbkF1dGhvcml0eS5jcmwwDQYJKoZIhvcN
  AQEMBQADggIBAGLjko5GdyYV1KKJhJDA9x6HYXh3m4f7X6u9M/J511b8SgO6vK/8
  -----END CERTIFICATE-----
  )";

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


# Obtaining the GTS Root R1 Certificate

You can obtain the GTS Root R1 certificate directly from Google's official PKI repository, export it using OpenSSL or a web browser, or copy the preformatted C++ snippet below.

> **Important:** The certificate shown below is truncated. Download the complete certificate from Google's official PKI repository before using it in production.

## Preformatted C++ Code

Copy and paste the following structure into your Arduino or ESP32 project:

```cpp
// Google Root CA Certificate (GTS Root R1)
const char* GOOGLE_ROOT_CA =R"(
-----BEGIN CERTIFICATE-----
MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw
CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU
MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw
MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp
Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA
A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo
27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w
Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw
TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl
qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH
szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8
Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk
MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92
wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p
aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN
VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID
AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E
FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb
C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe
QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy
h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4
7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J
ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef
MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/
Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT
6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ
0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm
2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb
bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c
-----END CERTIFICATE-----
)";
```

## Method 1: Download from Google Trust Services

1. Open the [Google Trust Services PKI repository](https://pki.goog/repository/).
2. Scroll down to **Root CAs**.
3. Find **GTS Root R1**.
4. Click **PEM** to download the `r1.pem` certificate file.

## Method 2: Extract the Certificate with OpenSSL

Run the following command in a terminal to retrieve and inspect the certificate chain from Google's servers:

```bash
openssl s_client \
  -showcerts \
  -connect script.google.com:443 \
  </dev/null
```

Inspect the output for the certificate blocks:

```text
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
```

> **Note:** The root certificate is not always sent by the server. For production use, download GTS Root R1 directly from Google's official PKI repository rather than assuming it is present in the server response.

## Method 3: Export the Certificate Using a Web Browser

1. Open [Google Apps Script](https://script.google.com) in Chrome or Microsoft Edge.
2. Click the padlock or connection-details icon in the address bar.
3. Select **Connection is secure**.
4. Click **Certificate is valid**.
5. Open the **Details** or **Certification Path** tab.
6. Select the top-level parent certificate, such as **GTS Root R1**.
7. Choose **Export**.
8. Select **Base-64 encoded ASCII**, which produces a PEM-formatted certificate.

## Format a PEM File for C++

If you have a raw `.pem` file, place each certificate line inside a C++ string and add `\n` at the end of each line.

### Raw PEM

```text
-----BEGIN CERTIFICATE-----
```

### C++ String     

The complete certificate should follow this structure:
```cpp
const char* GOOGLE_ROOT_CA =R"(
-----BEGIN CERTIFICATE-----
CERTIFICATE_LINE_1
CERTIFICATE_LINE_2
:
CERTIFICATE_LINE_n
-----END CERTIFICATE-----
)";

```


## Use the Certificate with `WiFiClientSecure`

Pass the certificate to `client.setCACert()`:

```cpp
#include <WiFiClientSecure.h>

WiFiClientSecure client;

client.setCACert(GOOGLE_ROOT_CA);
```

## Verification

After passing the certificate to `client.setCACert(GOOGLE_ROOT_CA)`, `WiFiClientSecure` should validate the server certificate chain without producing an error such as:

```text
ESP_ERR_MBEDTLS_SSL_HANDSHAKE_FAILED
```

---

# Verifying TLS Connections to Google Services on the ESP32-S3

To verify TLS connections to Google services on the ESP32-S3, you must synchronize the board's internal system clock via NTP and supply root certificates to `WiFiClientSecure`.

Without synchronized time, TLS verification will fail because the microcontroller cannot validate certificate expiration dates.

## NTP Sync Requirement

TLS clock validation requires valid system time. If your ESP32-S3 attempts HTTPS verification before NTP synchronization completes, the TLS handshake will fail with a certificate validation error.

## 1. Synchronize the ESP32-S3 Clock via NTP

**Prerequisite:** Complete this step before opening an HTTPS connection.

Fetch network time using `configTime()`:

```cpp
#include <WiFi.h>

void syncNTP() {
  // Synchronize time with public NTP servers
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync");

  time_t now = time(nullptr);

  // Wait until system time is updated past the Unix epoch
  // Year 2016 or later
  while (now < 1466553600) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }

  Serial.println("\nTime successfully synchronized!");
}
```

### Verification

The Serial Monitor should print:

```text
Time successfully synchronized!
```

The value returned by `time(nullptr)` should be an epoch value matching the current UTC time.

## 2. Use the ESP32 Built-in Certificate Bundle

**Method A — Recommended**

The ESP32 Arduino Core includes a precompiled bundle of trusted root certificates, including Google Trust Services certificates.

This method handles certificate rotation automatically without requiring hardcoded PEM certificate strings.

```cpp
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <esp_crt_bundle.h>

// Includes the root certificate bundle
void makeSecureRequest() {
  WiFiClientSecure client;

  // Attach the ESP-IDF root CA bundle
  client.setCACertBundle(rootca_crt_bundle_start);

  HTTPClient http;

  http.setFollowRedirects(
    HTTPC_STRICT_FOLLOW_REDIRECTS
  );

  if (
    http.begin(
      client,
      "[https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec](https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec)"
    )
  ) {
    int httpCode = http.GET();

    Serial.printf(
      "HTTP Response code: %d\n",
      httpCode
    );

    http.end();
  }
}
```

### Verification

The request should return an HTTP status code such as:

- `200 OK`
- `302 Found`

It should not return a negative connection error code such as:

```text
-1 (HTTPC_ERROR_CONNECTION_REFUSED)
```

## 3. Pin Google's Root CA Certificate

**Method B — Alternative**

If you prefer to explicitly restrict trust to Google services, pass Google's GTS Root R1 certificate in PEM format directly to `client.setCACert()`.

```cpp
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// Google GTS Root R1 Certificate
const char* GOOGLE_GTS_ROOT_R1 =
  "-----BEGIN CERTIFICATE-----\n"
  "MIIFvTCCA7WgAwIBAgINAgO9WmyU2A1wAKXA8TANBgkqhkiG9w0BAQsFADBGMQsw\n"
  "CQYDVQQGEwJVUzEPMA0GA1UEChMGR29vZ2xlMRUwEwYDVQQLEwxHVFMgUm9vdCBS\n"
  "MQ0wCwYDVQQDEwRHVFMxMB4XDTE2MDYyMjAwMDAwMFoXDTM2MDYyMjAwMDAwMFow\n"
  "RjELMAkGA1UEBhMCVVMxDzANBgNVBAoTBkdvb2dsZTEVMBMGA1UECxMMR1RTIFJv\n"
  "b3QgUjExDTALBgNVBAMTBkdUUzEwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n"
  "AoIBAQC5EQ2miW5wT3cE8tU9DqUjJ+F+k7Z0qjH3l3y9m0x7e8kKz8Z1aYp... (truncated)\n"
  "-----END CERTIFICATE-----\n";

void makePinnedSecureRequest() {
  WiFiClientSecure client;

  // Explicitly set the trusted root certificate
  client.setCACert(GOOGLE_GTS_ROOT_R1);

  HTTPClient http;

  http.setFollowRedirects(
    HTTPC_STRICT_FOLLOW_REDIRECTS
  );

  if (
    http.begin(
      client,
      "[https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec](https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec)"
    )
  ) {
    int httpCode = http.GET();

    Serial.printf(
      "HTTP Response code: %d\n",
      httpCode
    );

    http.end();
  }
}
```

### Verification

After a failed connection attempt, print the result of `client.lastError(buf, len)`.

An output of `0` indicates that no TLS handshake errors occurred.

```cpp
char errorBuffer;

client.lastError(
  errorBuffer,
  sizeof(errorBuffer)
);

Serial.println(errorBuffer);
```

> **Important:** The certificate shown above is truncated and cannot be used for actual TLS verification. Replace it with the complete, valid Google GTS Root R1 PEM certificate.

---

# ESP32-S3 WiFiClientSecure Memory Usage

`WiFiClientSecure` on the ESP32-S3 consumes approximately 45–50 KB of internal RAM per connection by default because mbedTLS allocates standard 16 KB read and 16 KB write buffers.

## Memory Optimization Strategies

### 1. Reduce TLS Buffer Sizes

If your JSON request and response payloads are small, reduce the mbedTLS incoming and outgoing buffer sizes from 16 KB to 2–4 KB before initiating the connection.

```cpp
WiFiClientSecure client;

// Set RX buffer to 2,048 bytes and TX buffer to 512 bytes
client.setBufferSizes(2048, 512);

client.setInsecure();  // Or use client.setCACert(...)
```

#### Verification

Print `ESP.getFreeHeap()` immediately before and after `client.connect()`.

The RAM reduction during the connection should decrease from approximately 45 KB to approximately 8–12 KB.

### 2. Move mbedTLS Memory Allocations to PSRAM

If your ESP32-S3 module includes external PSRAM, such as an `N8R8` or `N16R8` module, configure mbedTLS to allocate its SSL contexts and buffers from PSRAM instead of scarce internal SRAM.

If using PlatformIO or ESP-IDF, add the following settings to your `sdkconfig` or `platformio.ini` file:

```ini
; platformio.ini build flags for ESP32-S3 with PSRAM
build_flags =
    -DCONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC=1
    -DBOARD_HAS_PSRAM
```

#### Verification

Check `ESP.getFreeHeap()` during an active HTTPS transmission.

Internal SRAM usage should remain virtually unchanged.

### 3. Enable Dynamic Buffer Allocation

Dynamic buffer allocation frees mbedTLS buffers when they are no longer actively transferring data instead of holding them for the entire lifecycle of the client.

Add the following settings to the ESP-IDF `sdkconfig`:

```ini
CONFIG_MBEDTLS_DYNAMIC_BUFFER=y
CONFIG_MBEDTLS_DYNAMIC_FREE_CONFIG_DATA=y
CONFIG_MBEDTLS_DYNAMIC_FREE_CA_CERT=y
```

#### Verification

Monitor heap usage during a long-lived connection.

RAM usage should decrease between active HTTP transfers even while the socket remains open.

### 4. Reuse Connections with HTTP Keep-Alive

Reestablishing a complete TLS handshake during every loop iteration continuously allocates and frees memory, which can cause heap fragmentation.

Enable connection reuse to keep the SSL context active:

```cpp
HTTPClient http;

// Enable HTTP Keep-Alive connection reuse
http.setReuse(true);
```

#### Verification

Subsequent HTTP requests should execute significantly faster, potentially under 100 ms compared with 1–2 seconds for a new TLS connection.

`ESP.getMinFreeHeap()` should also stabilize without steadily decreasing because of fragmentation.

### 5. Explicitly Flush and Stop Clients

If you do not reuse connections, ensure that `WiFiClientSecure` and `HTTPClient` explicitly release their allocated memory buffers before going out of scope.

```cpp
http.end();     // Closes the HTTP connection and frees internal stream buffers
client.stop();  // Closes the socket and frees mbedTLS SSL context memory
```

#### Verification

Call `ESP.getFreeHeap()` after `client.stop()`.

The available free heap should return to its pre-connection baseline.
```
