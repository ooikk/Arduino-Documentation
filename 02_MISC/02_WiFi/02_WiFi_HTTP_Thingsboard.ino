/*
https://thingsboard.io/docs/paas/user-guide/connectivity-guide/?
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define SECURE_HTTPS

#ifdef SECURE_HTTPS
#include <WiFiClientSecure.h>
WiFiClientSecure myClient;
#else
WiFiClient myClient;
#endif

// Network Settings
const char* WIFI_SSID = "Nightingale_IoT";
const char* WIFI_PASS = "1122334455";
const char* TB_HOST = "thingsboard.cloud";          // Replace with your IP/Host
const char* ACCESS_TOKEN = "YOUR_DEVICE_ACCESS_TOKEN";  //"YOUR_DEVICE_ACCESS_TOKEN";

#ifdef SECURE_HTTPS
const int TB_PORT = 443;
String protocol = "https://";
#else
const int TB_PORT = 80;
String protocol = "http://";
#endif

#ifdef SECURE_HTTPS
// -----------------------------------------------------------------------------
// Root CA Certificate
//
// Paste the complete root CA certificate when USE_INSECURE_SSL is false.
// -----------------------------------------------------------------------------
const char* ROOT_CA = R"pem(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)pem";

#endif

// Key Definitions
const char* TOPIC_STATUS = "status";
const char* TOPIC_TEMP = "temperature";
const char* TOPIC_RSSI = "rssi";
const char* TOPIC_UPTIME = "uptime";
const char* TOPIC_LED_SET = "led-control";
const char* TOPIC_LED_STATE = "led-state";
const char* TOPIC_BUTTON = "button";



// Hardware Pins
const int LED_PIN = 1;
const int BUTTON_PIN = 2;  // ESP32 BOOT button

bool lastBtnState = HIGH;
bool currentLedState = false;
unsigned long lastTelemetryTime = 0;
const long telemetryInterval = 10000;  // Publish telemetry every 10 seconds

unsigned long lastPollSharedAttributesTime = 0;
const long pollSharedAttributesInterval = 1000;  // Poll share attribute every 1 seconds


void postAttributes() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = protocol + String(TB_HOST) + "/api/v1/" + String(ACCESS_TOKEN) + "/attributes";
  http.begin(myClient, url);

  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<128> doc;
  if (currentLedState == true) {
    doc[TOPIC_STATUS] = "online";
  } else {
    doc[TOPIC_STATUS] = "offline";
  }

  doc[TOPIC_LED_STATE] = currentLedState ? "ON" : "OFF";

  String payload;
  serializeJson(doc, payload);

  int code = http.POST(payload);
  Serial.printf("[%s] Attributes POST code: %d Payload: %s\n", protocol, code, payload.c_str());
  http.end();
}

void postTelemetry(float temp, long rssi, unsigned long uptime, const char* buttonVal) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = protocol + String(TB_HOST) + "/api/v1/" + String(ACCESS_TOKEN) + "/telemetry";

  http.begin(myClient, url);

  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> doc;
  doc[TOPIC_TEMP] = temp;
  doc[TOPIC_RSSI] = rssi;
  doc[TOPIC_UPTIME] = uptime;
  doc[TOPIC_BUTTON] = buttonVal;

  String payload;
  serializeJson(doc, payload);

  int code = http.POST(payload);
  Serial.printf("[%s] Telemetry POST code: %d Payload:%s\n", protocol, code, payload.c_str());
  http.end();
}

void pollSharedAttributes() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = protocol + String(TB_HOST) + "/api/v1/" + String(ACCESS_TOKEN) + "/attributes?sharedKeys=" + String(TOPIC_LED_SET);
  http.begin(myClient, url);


  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    // Received payload: {"shared":{"led-control":false}}
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, payload);

    if (!err && doc.containsKey("shared") && doc["shared"].containsKey(TOPIC_LED_SET)) {
      bool targetState = doc["shared"][TOPIC_LED_SET].as<bool>();
      if (targetState != currentLedState) {
        currentLedState = targetState;
        digitalWrite(LED_PIN, currentLedState ? HIGH : LOW);
        Serial.printf("[%s] Received Raw Shared Attributes payload: %s\n", protocol, payload.c_str());
        Serial.printf("[%s] LED updated to: %s\n", protocol, currentLedState ? "HIGH" : "LOW");
        postAttributes();  // Notify TB of updated status
      }
    }
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

#ifdef SECURE_HTTPS
  Serial.printf("Setup CA Cert\n");
  myClient.setCACert(ROOT_CA);  // Strict validation
#else
  //myClient.setInsecure();;  // Skip SSL cert validation
#endif

  postAttributes();  // Send initial state
}

void loop() {
  // 1. Instant Button Telemetry
  bool btnState = digitalRead(BUTTON_PIN);
  if (btnState != lastBtnState) {
    delay(50);  // Debounce
    if (digitalRead(BUTTON_PIN) == btnState) {
      lastBtnState = btnState;
      postTelemetry(24.5, WiFi.RSSI(), millis() / 1000, (btnState == LOW) ? "PRESSED" : "RELEASED");
    }
  }

  // 2. Periodic Telemetry
  if (millis() - lastTelemetryTime > telemetryInterval) {
    lastTelemetryTime = millis();
    float simulatedTemp = 22.0 + (random(-10, 10) / 10.0);
    postTelemetry(simulatedTemp, WiFi.RSSI(), millis() / 1000, (lastBtnState == LOW) ? "PRESSED" : "RELEASED");
  }
  // 3. Periodic Attribute Polling
  if (millis() - lastPollSharedAttributesTime > pollSharedAttributesInterval) {
    lastPollSharedAttributesTime = millis();
    pollSharedAttributes();
  }
}
