/*
https://thingsboard.io/docs/paas/user-guide/connectivity-guide/?
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define MQTTS_TLS  // ##

#ifdef MQTTS_TLS
#include <WiFiClientSecure.h>  // ## 1. Added for SSL/TLS support
#endif


// --- Network & ThingsBoard Configuration ---
const char* WIFI_SSID = "Nightingale_IoT";
const char* WIFI_PASS = "1122334455";
const char* TB_HOST = "thingsboard.cloud";
#ifdef MQTTS_TLS
const int TB_PORT = 8883;  // ## 2. Changed from 1883 to 8883
#else
const int TB_PORT = 1883;
#endif
const char* TOKEN = "YOUR_THINGSBOARD_ACCESS_TOKEN";  //"YOUR_THINGSBOARD_ACCESS_TOKEN";



#ifdef MQTTS_TLS

// Method 3: ISRG Root X1

const char* TB_ROOT_CA = R"(
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
)";

#endif

// --- Native ThingsBoard MQTT Topics ---
const char* TB_TELEMETRY_TOPIC = "v1/devices/me/telemetry";
const char* TB_ATTRIBUTES_TOPIC = "v1/devices/me/attributes";
const char* TB_RPC_REQUEST_TOPIC = "v1/devices/me/rpc/request/+";

/********************************************************************
TOPIC_STATUS = "status";
TOPIC_TEMP = "temperature";
TOPIC_RSSI = "rssi";
TOPIC_UPTIME = "uptime";
TOPIC_LED_SET = "led-control";
TOPIC_LED_STATE = "led-state";
TOPIC_BUTTON = "button"; 
********************************************************************/

// --- Hardware Pins ---
#define LED_PIN 1     // Onboard LED
#define BUTTON_PIN 2  // Boot button on ESP32-S3 (Pull-up)

#ifdef MQTTS_TLS
WiFiClientSecure espClient;  // ## 4. Changed from WiFiClient to WiFiClientSecure
#else
WiFiClient espClient;
#endif

PubSubClient client(espClient);

unsigned long lastTelemetryTime = 0;
const long telemetryInterval = 20000;  // Publish telemetry every 10 seconds
bool lastButtonState = HIGH;

// Flags to safely defer publishing outside the callback
volatile bool pendingLedUpdate = false;
volatile bool targetLedState = false;

//#define RPC_ACK  // Do not need this
#ifdef RPC_ACK
String pendingRequestId = "";  // Stores RPC Request ID safely
#endif

// Helper function to send client attribute update for led-state
void publishLedState(bool isOn) {
  StaticJsonDocument<100> doc;
  doc["led-state"] = isOn ? "ON" : "OFF";
  char buffer[100];
  serializeJson(doc, buffer);
  client.publish(TB_ATTRIBUTES_TOPIC, buffer);
  Serial.printf("Send led-state: %s\n", isOn ? "ON" : "OFF");
}

// Callback function triggered when ThingsBoard sends commands to ESP32
void callback(char* topic, byte* payload, unsigned int length) {
  // Guard against zero-length or NULL payloads
  if (length == 0 || payload == NULL) return;

  // Copy raw byte buffer to a isolated memory String to prevent pointer dereference
  String jsonString = "";
  jsonString.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) {
    jsonString += (char)payload[i];
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  if (error) return;

  bool commandFound = false;
  bool state = false;

  // Extract values using JsonVariant type checks instead of raw string pointers
  if (doc.containsKey("led-control")) {
    commandFound = true;
    JsonVariant val = doc["led-control"];
    if (val.is<bool>()) state = val.as<bool>();
    else if (val.is<int>()) state = (val.as<int>() == 1);
    else state = (val.as<String>().equalsIgnoreCase("ON") || val.as<String>().equalsIgnoreCase("true"));
  } else if (doc.containsKey("shared") && doc["shared"].containsKey("led-control")) {
    commandFound = true;
    JsonVariant val = doc["shared"]["led-control"];
    if (val.is<bool>()) state = val.as<bool>();
    else if (val.is<int>()) state = (val.as<int>() == 1);
    else state = (val.as<String>().equalsIgnoreCase("ON") || val.as<String>().equalsIgnoreCase("true"));
  } else if (doc.containsKey("method") && doc["method"].as<String>() == "led-control") {
    commandFound = true;
    JsonVariant val = doc["params"];
    if (val.is<bool>()) state = val.as<bool>();
    else if (val.is<int>()) state = (val.as<int>() == 1);
    else state = (val.as<String>().equalsIgnoreCase("ON") || val.as<String>().equalsIgnoreCase("true"));
  }

  if (commandFound) {
    digitalWrite(LED_PIN, state ? HIGH : LOW);
    targetLedState = state;
    pendingLedUpdate = true;  // Set flag instead of calling client.publish directly
#ifdef RPC_ACK
    // --- ADD THIS RPC ACKNOWLEDGEMENT ---
    // Capture RPC Request ID if incoming message is an RPC call
    String topicStr = String(topic);
    if (topicStr.startsWith("v1/devices/me/rpc/request/")) {
      pendingRequestId = topicStr.substring(27);
    } else {
      pendingRequestId = "";
    }
#endif
  }
}

void reconnect() {

  // 1. If Wi-Fi dropped while running, reconnect to Wi-Fi first
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.printf("Re-connecting to WiFi.");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.printf(".");
    }
    Serial.printf("\n**Connect to WiFi**\n");
  }
  // 2. Once Wi-Fi is up, connect to ThingsBoard MQTT and subscribe
  while (!client.connected()) {
    if (client.connect("ESP32S3_Client", TOKEN, NULL)) {
      Serial.printf("\n++Connect to Client++\n");
      // 1. Publish client attribute: status = "online"
      client.publish(TB_ATTRIBUTES_TOPIC, "{\"status\":\"online\"}");

      // 2. Publish initial LED state attribute
      publishLedState(digitalRead(LED_PIN) == HIGH);

      // 3. Subscribe to Shared Attribute updates and RPC requests from TB
      client.subscribe(TB_ATTRIBUTES_TOPIC);
      client.subscribe(TB_RPC_REQUEST_TOPIC);
    } else {
      delay(5000);
      Serial.printf(".");
    }
  }
  Serial.printf("\n**Connect to Client**\n");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to WiFi.");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.printf(".");
  }
  Serial.printf("\n**Connect to WiFi**\n");

#ifdef MQTTS_TLS
  // ## 5. Assign Root CA Certificate for verification
  espClient.setCACert(TB_ROOT_CA);
  Serial.printf("Assign Root CA Certificate for verification\n");
  // Note for rapid testing only (disables certificate verification):
  // espClient.setInsecure();
#endif

  Serial.printf("Setup ThingBoards Server\n");
  client.setServer(TB_HOST, TB_PORT);
  client.setBufferSize(1024);  // Prevents default 128-byte overflow
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Safely publish LED state back to TB outside the callback buffer
  if (pendingLedUpdate) {
    pendingLedUpdate = false;
#ifdef RPC_ACK
    // Send RPC response back to TB if request ID exists
    if (pendingRequestId.length() > 0) {
      String responseTopic = "v1/devices/me/rpc/response/" + pendingRequestId;
      client.publish(responseTopic.c_str(), targetLedState ? "{\"led-control\":true}" : "{\"led-control\":false}");
      pendingRequestId = "";
    }
#endif
    // Publish state attribute
    publishLedState(targetLedState);
    Serial.printf("Update LED State to ThingsBoard: %s\n", targetLedState ? "HIGH" : "LOW");
  }


  unsigned long currentMillis = millis();

  // --- 1. Periodic Telemetry (temperature, rssi, uptime) ---
  if (currentMillis - lastTelemetryTime >= telemetryInterval) {
    lastTelemetryTime = currentMillis;

    StaticJsonDocument<200> doc;
    doc["temperature"] = 22.0 + (random(-100, 100) / 10.0);
    doc["rssi"] = WiFi.RSSI();
    doc["uptime"] = millis() / 1000;

    char buffer[200];
    serializeJson(doc, buffer);
    client.publish(TB_TELEMETRY_TOPIC, buffer);
    Serial.printf("Send Telemetry data to TB: %s\n", buffer);

    //Serial.print("Send Telemetry data to TB: ");
    //serializeJson(doc, Serial);
    //Serial.println();
  }

  // --- 2. Button State Detection (Pressed / Released) ---
  bool currentButtonState = digitalRead(BUTTON_PIN);
  if (currentButtonState != lastButtonState) {
    delay(50);  // Debounce delay
    if (digitalRead(BUTTON_PIN) == currentButtonState) {
      StaticJsonDocument<100> doc;
      doc["button"] = (currentButtonState == LOW) ? "PRESSED" : "RELEASED";

      char buffer[100];
      serializeJson(doc, buffer);
      //client.publish(TB_TELEMETRY_TOPIC, buffer);
      // FIX: Publish to TB_ATTRIBUTES_TOPIC instead of TB_TELEMETRY_TOPIC
      client.publish(TB_ATTRIBUTES_TOPIC, buffer);

      lastButtonState = currentButtonState;
      //Serial.printf("Current Button State: %s\n", (currentButtonState == LOW) ? "PRESSED" : "RELEASED");
      Serial.printf("Current Button State: %s\n", buffer);
      if (currentButtonState == HIGH) {
        client.publish(TB_ATTRIBUTES_TOPIC, "{\"status\":\"online\"}");
        Serial.printf("Status: Online\n");
      } else {
        client.publish(TB_ATTRIBUTES_TOPIC, "{\"status\":\"offline\"}");
        Serial.printf("Status: Offline\n");
      }
    }
  }
}
