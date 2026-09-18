#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define SECURE_CA_CERT
#define USE_doGet  // Make sure Google Sheet define doGet(e) function


const char* WIFI_SSID = "Nightingale_IoT";
const char* WIFI_PASS = "1122334455";

// Replace with your Google Apps Script Deployment URL
const char* GOOGLE_SCRIPT_URL =
  "https://script.google.com/macros/s/AKfycbydgeeWgM2g6HYUMR8AaBWpeg9ZSPPTAR2sfZzpXy1892bVAxEETOaLP5QWiIR205yK/exec";

// Topics / Field Keys
const char* TOPIC_STATUS = "status";
const char* TOPIC_TEMP = "temperature";
const char* TOPIC_RSSI = "rssi";
const char* TOPIC_UPTIME = "uptime";
const char* TOPIC_LED_SET = "led-control";
const char* TOPIC_LED_STATE = "led-state";
const char* TOPIC_BUTTON = "button";


// --- Hardware Pins ---
const int LED_PIN = 1;
const int BUTTON_PIN = 2;  // Boot button on ESP32-S3 (Pull-up)

#ifdef SECURE_CA_CERT
// Google Root CA Certificate (GTS Root R1)
// for strict TLS verification
const char* GOOGLE_ROOT_CA = R"(
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

#endif

#ifndef USE_doGet

void sendTelemetryAndFetchCommands() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected");
    return;
  }

  StaticJsonDocument<256> doc;
  doc["type"] = "telemetry";
  doc[TOPIC_STATUS] = "ONLINE";
  doc[TOPIC_TEMP] = random(100, 500) / 10.0f;
  doc[TOPIC_RSSI] = WiFi.RSSI();
  doc[TOPIC_UPTIME] = millis() / 1000UL;
  doc[TOPIC_LED_STATE] = digitalRead(LED_PIN);
  doc[TOPIC_BUTTON] = (digitalRead(BUTTON_PIN) == LOW) ? 1 : 0;

  String requestBody;
  serializeJson(doc, requestBody);
  Serial.println("[TELEMETRY] Send Payload: " + requestBody);

  WiFiClientSecure postClient;
#ifdef SECURE_CA_CERT
  postClient.setCACert(GOOGLE_ROOT_CA);
#else
  postClient.setInsecure();
#endif

  HTTPClient postHttp;
  postHttp.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  postHttp.setReuse(false);
  postHttp.setConnectTimeout(15000);
  postHttp.setTimeout(20000);

  if (!postHttp.begin(postClient, GOOGLE_SCRIPT_URL)) {
    Serial.println("[TELEMETRY] Cannot start POST");
    return;
  }

  postHttp.addHeader("Content-Type", "application/json");
  int postCode = postHttp.POST(requestBody);
  Serial.printf("[TELEMETRY] POST response: %d\n", postCode);

  String responseBody;
  int finalCode = postCode;

  if (postCode == 302) {
    String redirectUrl = postHttp.getLocation();
    postHttp.end();

    if (!redirectUrl.startsWith("https://script.googleusercontent.com/")) {
      Serial.println("[TELEMETRY] Missing or unexpected redirect URL");
      return;
    }

    WiFiClientSecure getClient;
#ifdef SECURE_CA_CERT
    getClient.setCACert(GOOGLE_ROOT_CA);
#else
    getClient.setInsecure();
#endif

    HTTPClient getHttp;

    getHttp.setReuse(false);
    getHttp.setConnectTimeout(15000);
    getHttp.setTimeout(20000);

    if (!getHttp.begin(getClient, redirectUrl)) {
      Serial.println("[TELEMETRY] Cannot start response GET");
      return;
    }

    finalCode = getHttp.GET();
    if (finalCode == 200) {
      responseBody = getHttp.getString();
    }
    getHttp.end();

  } else {
    if (postCode == 200) {
      responseBody = postHttp.getString();
    }
    postHttp.end();
  }

  Serial.printf("[TELEMETRY] Final response: %d\n", finalCode);
  if (finalCode != 200) {
    Serial.println("[TELEMETRY] Failed to fetch command");
    return;
  }

  Serial.println("Server Response: " + responseBody);

  StaticJsonDocument<192> responseDoc;
  DeserializationError err = deserializeJson(responseDoc, responseBody);

  if (err) {
    Serial.printf("[COMMAND] Invalid JSON: %s\n", err.c_str());
    return;
  }

  if (responseDoc["status"] != "success") {
    Serial.println("[COMMAND] Apps Script reported an error");
    return;
  }

  if (!responseDoc["led-control"].is<int>()) {
    Serial.println("[COMMAND] led-control must be a number, 0 or 1");
    return;
  }

  int targetLedState = responseDoc["led-control"].as<int>();
  if (targetLedState != 0 && targetLedState != 1) {
    Serial.println("[COMMAND] led-control must be 0 or 1");
    return;
  }

  digitalWrite(LED_PIN, targetLedState ? HIGH : LOW);
  Serial.printf("[COMMAND] LED state updated to: %d\n", targetLedState);
}

#else

void sendTelemetryCommands() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected");
    return;
  }

  //WiFiClientSecure postClient;
  WiFiClientSecure client;

#ifdef SECURE_CA_CERT
  // Option A: Strict SSL
  // Requires a valid certificate bundle or root CA
  // postClient.setCACert(GOOGLE_ROOT_CA);
  client.setCACert(GOOGLE_ROOT_CA);
#else
  // Option B: TLS encryption enabled without
  // fingerprint validation
  // postClient.setInsecure();
  client.setInsecure();
#endif


  HTTPClient http;

  //http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);

  // Google Apps Script usually redirects; strict may fail to follow cross-host redirects.
  //http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  http.setReuse(false);
  http.setConnectTimeout(15000);
  http.setTimeout(20000);

  if (!http.begin(client, GOOGLE_SCRIPT_URL)) {
    Serial.println("[TELEMETRY] http.begin() failed");
    return;
  }


  //http.begin(postClient, GOOGLE_SCRIPT_URL);
  //http.begin(client, GOOGLE_SCRIPT_URL);
  http.addHeader("Content-Type", "application/json");

  // Build telemetry JSON
  StaticJsonDocument<256> doc;
  doc["type"] = "telemetry";
  doc[TOPIC_STATUS] = "ONLINE";

  float temperature = random(100, 500) / 10.0f;

  // Store temperature as a JSON number, not a temporary string
  doc[TOPIC_TEMP] = temperature;
  doc[TOPIC_RSSI] = WiFi.RSSI();
  doc[TOPIC_UPTIME] = millis() / 1000UL;
  doc[TOPIC_LED_STATE] = digitalRead(LED_PIN);
  doc[TOPIC_BUTTON] =
    (digitalRead(BUTTON_PIN) == LOW) ? 1 : 0;

  String requestBody;
  serializeJson(doc, requestBody);

  int httpCode = http.POST(requestBody);
  if (httpCode == 302 || httpCode == 200) {  //HTTP_CODE_OK == 200, HTTP_CODE_FOUND == 302
    Serial.printf("[TELEMETRY] Uploaded successfully (HTTP %d)\n", httpCode);
    Serial.println("[TELEMETRY] Send Payload: " + requestBody);
  } else {
    Serial.printf("[TELEMETRY] Upload failed, http code: %d\n", httpCode);
  }
  http.end();
}

void fetchCommands() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected");
    return;
  }

  WiFiClientSecure client;

#ifdef SECURE_CA_CERT
  // Option A: Strict SSL
  // Requires a valid certificate bundle or root CA
  // postClient.setCACert(GOOGLE_ROOT_CA);
  client.setCACert(GOOGLE_ROOT_CA);
#else
  // Option B: TLS encryption enabled without
  // fingerprint validation
  // postClient.setInsecure();
  client.setInsecure();
#endif

  // Read the response from doGet(e)

  HTTPClient http;


  //http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  http.setRedirectLimit(3);
  http.setReuse(false);
  http.setConnectTimeout(15000);
  http.setTimeout(20000);

  if (!http.begin(client, GOOGLE_SCRIPT_URL)) {
    Serial.println("[COMMAND] http.begin() failed");
    return;
  }

  //const char* headerKeys[] = { "Location" };
  //http.collectHeaders(headerKeys, 1);

  int httpCode = http.GET();

  Serial.printf("[COMMAND] Initial HTTP code: %d\n", httpCode);

  String responseBody;
  if (httpCode == HTTP_CODE_OK) {
    // Included in case Google returns 200 directly
    responseBody = http.getString();
    http.end();
    client.stop();
  } else if (httpCode == HTTP_CODE_FOUND || httpCode == HTTP_CODE_SEE_OTHER || httpCode == HTTP_CODE_TEMPORARY_REDIRECT || httpCode == HTTP_CODE_PERMANENT_REDIRECT) {
    // Google Apps Script ContentService redirect

    // const char* headerKeys[] = { "Location" };
    // http.collectHeaders(headerKeys, 1);

    //String redirectUrl = http.header("Location");   // to use this method, need to execute collectHeaders() first

    String redirectUrl = http.getLocation();

    Serial.println("[COMMAND] Redirect location: " + redirectUrl);
    // Close the first request before using the one-time URL
    http.end();
    client.stop();

    if (redirectUrl.length() == 0) {
      Serial.println("[COMMAND] Redirect URL is empty");
      return;
    }

    // Use completely new objects for redirected request
    WiFiClientSecure redirectClient;

#ifdef SECURE_CA_CERT
    redirectClient.setCACert(GOOGLE_ROOT_CA);
#else
    redirectClient.setInsecure();
#endif

    HTTPClient redirectHttp;

    if (!redirectHttp.begin(redirectClient, redirectUrl)) {
      Serial.println("[COMMAND] Redirect http.begin() failed");
      return;
    }

    redirectHttp.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);

    redirectHttp.setReuse(false);
    redirectHttp.setConnectTimeout(15000);
    redirectHttp.setTimeout(20000);
    redirectHttp.addHeader("Connection", "close");

    int finalHttpCode = redirectHttp.GET();

    Serial.printf("[COMMAND] Redirected HTTP code: %d\n", finalHttpCode);

    if (finalHttpCode == HTTP_CODE_OK) {
      responseBody = redirectHttp.getString();
    } else if (finalHttpCode > 0) {
      String errorBody = redirectHttp.getString();

      Serial.printf("[COMMAND] Redirected HTTP error: %d\n", finalHttpCode);

      if (errorBody.length() > 0) {
        Serial.println("[COMMAND] Google error response:");
        Serial.println(errorBody);
      }

      redirectHttp.end();
      redirectClient.stop();
      return;
    } else {
      Serial.printf("[COMMAND] Redirect GET failed: %s (HTTP %d)\n", HTTPClient::errorToString(finalHttpCode).c_str(), finalHttpCode);
      redirectHttp.end();
      redirectClient.stop();
      return;
    }

    redirectHttp.end();
    redirectClient.stop();
  } else if (httpCode > 0) {
    String errorBody = http.getString();
    Serial.printf("[COMMAND] Initial HTTP error: %d\n", httpCode);

    if (errorBody.length() > 0) {
      Serial.println(errorBody);
    }
    http.end();
    client.stop();
    return;
  } else {
    Serial.printf("[COMMAND] Initial GET failed: %s (HTTP %d)\n", HTTPClient::errorToString(httpCode).c_str(), httpCode);
    http.end();
    client.stop();
    return;
  }


  //Serial.println("[COMMAND] Server response: " + responseBody);
  Serial.printf("[COMMAND] Server response (HTTP %d): ", httpCode);
  Serial.println(responseBody);
  StaticJsonDocument<128> responseDoc;
  DeserializationError err = deserializeJson(responseDoc, responseBody);

  if (!err && responseDoc.containsKey("led-control")) {
    // check for none integer
    if (!responseDoc["led-control"].is<int>()) {
      Serial.println("[COMMAND] led-control must be a number, 0 or 1");

      /*
      Serial.print("[COMMAND] led-control is not an integer. Received: ");
      serializeJson(ledValue, Serial);
      Serial.println();
*/
      return;
    }

    int targetLedState = responseDoc["led-control"];
    if (targetLedState != 0 && targetLedState != 1) {
      Serial.printf("[COMMAND] Invalid led-control value: %d\n", targetLedState);
      return;
    } else {
      digitalWrite(LED_PIN, targetLedState ? HIGH : LOW);
      Serial.printf("[COMMAND] LED state updated to: %d\n", targetLedState);
    }
  } else {
    Serial.println("[COMMAND] Failed to parse led-control from response. Error Message: ");
    Serial.println(err.c_str());
  }
}
#endif

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
  Serial.println(WiFi.localIP());
}



void loop() {

#ifdef USE_doGet
  sendTelemetryCommands();
  fetchCommands();
#else
  sendTelemetryAndFetchCommands();
  // Poll and upload every 10 seconds
#endif
  delay(10000);
}