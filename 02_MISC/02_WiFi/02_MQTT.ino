#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

//**************************************************
// Select one
//#define HIVEMQ
//#define EMQX
//#define EMQX_CLOUD   // connect to Googlesheet
#define ADAFRUIT
//**************************************************

#if defined(EMQX) || defined(EMQX_CLOUD) || defined(ADAFRUIT)
#define SECURE_LOGIN
#endif

// ----------------------------
// User configuration
// ----------------------------
const char* WIFI_SSID = "Nightingale_IoT";
const char* WIFI_PASSWORD = "1122334455";

// ----------------------------
// Public Broker Configuration
// ----------------------------
#ifdef HIVEMQ
const char* MQTT_HOST = "broker.hivemq.com";  // Free public broker
const uint16_t MQTT_PORT = 1883;              // Standard TCP port for ESP32
#endif

#ifdef EMQX_CLOUD
const char* MQTT_HOST = "ffcebc18.ala.asia-southeast1.emqxsl.com";  // Fee public broker
const uint16_t MQTT_PORT = 8883;                                    // Standard TCP port for ESP32
#endif

#ifdef EMQX
const char* MQTT_HOST = "broker.emqx.io";  //"ffcebc18.ala.asia-southeast1.emqxsl.com"; // Free public broker
const uint16_t MQTT_PORT = 8883;           // Standard TCP port for ESP32
#endif

#ifdef ADAFRUIT
const char* MQTT_HOST = "io.adafruit.com";  //
const uint16_t MQTT_PORT = 8883;            // Standard TCP port for ESP32
#endif

#ifdef SECURE_LOGIN
#ifdef ADAFRUIT
const char* MQTT_USERNAME = "ooikk";                             // YOUR_ADAFRUIT_USERNAME
const char* MQTT_PASSWORD = "aio_ySnz63FVKOkm6Np4bbHiiFy5ZWPJ";  // YOUR_ADAFRUIT_IO_KEY
#else
const char* MQTT_USERNAME = "esp32s3";      // username
const char* MQTT_PASSWORD = "esp11223344";  // password
#endif
// ----------------------------
// Global objects
// ----------------------------
WiFiClientSecure secureClient;
PubSubClient mqtt(secureClient);

#else

const char* MQTT_USERNAME = "";  // No username needed
const char* MQTT_PASSWORD = "";  // No password needed
// ----------------------------
// Global objects
// ----------------------------
WiFiClient tcpClient;
PubSubClient mqtt(tcpClient);

#endif


// ----------------------------
// MQTT topics
// ----------------------------
#ifdef ADAFRUIT

//const char* TOPIC_TELEMETRY = "esp32s3/telemetry";
//const char* TOPIC_LED_SET = "esp32s3/led/set";
//const char* TOPIC_LED_STATE = "esp32s3/led/state";

const char* TOPIC_STATUS = "ooikk/feeds/status";
const char* TOPIC_TEMP = "ooikk/feeds/temperature";
const char* TOPIC_RSSI = "ooikk/feeds/rssi";
const char* TOPIC_UPTIME = "ooikk/feeds/uptime";
const char* TOPIC_LED_SET = "ooikk/feeds/led-control";
const char* TOPIC_LED_STATE = "ooikk/feeds/led-state";  // Adafruit IO feeds are bidirectional:

#else
const char* TOPIC_STATUS = "esp32s3/status";
const char* TOPIC_TELEMETRY = "esp32s3/telemetry";
const char* TOPIC_LED_SET = "esp32s3/led/set";
const char* TOPIC_LED_STATE = "esp32s3/led/state";
#endif

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



bool ledState = true;
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
#ifdef ADAFRUIT
      mqtt.publish(TOPIC_LED_STATE, "1", true);
#else
      mqtt.publish(TOPIC_LED_STATE, "{\"state\":\"1\"}");
#endif
      Serial.println("LED turned ON");
    } else if (turnOff) {
      ledState = false;
      digitalWrite(LED_PIN, LOW);
#ifdef ADAFRUIT
      mqtt.publish(TOPIC_LED_STATE, "0", true);
#else
      mqtt.publish(TOPIC_LED_STATE, "{\"state\":\"0\"}");
#endif
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

  // Build a completely unique Client ID with random suffix
  String clientId = "esp32s3-";
  clientId += WiFi.macAddress();
  clientId.replace(":", "");
  //clientId += "-";
  //clientId += String(random(1000, 9999));

  Serial.printf("Attempting MQTT connection as client ID: %s\n", clientId.c_str());

  // Connect with Last Will and Testament.
  // If the device disconnects unexpectedly, broker publishes "offline".
  bool connected = mqtt.connect(
    clientId.c_str(),
#ifdef SECURE_LOGIN
    MQTT_USERNAME,
    MQTT_PASSWORD,
#endif
    TOPIC_STATUS,
    0,         // Will QoS
    true,      // Will retain
    "offline"  // Will message
  );

  if (connected) {
    Serial.println("MQTT connected");

    // Publish online status.
    mqtt.publish(TOPIC_STATUS, "online", true);

    // Subscribe to command topics.
    mqtt.subscribe(TOPIC_LED_SET, 1);

    // Report current LED state.
#ifdef ADAFRUIT
    mqtt.publish(TOPIC_LED_STATE, ledState ? "1" : "0", true);
#else
    mqtt.publish(TOPIC_LED_STATE, ledState ? "{\"state\":\"1\"}" : "{\"state\":\"0\"}", true);
#endif

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
  int temperature = random(10, 40);
#ifdef ADAFRUIT
  // Publishing individual feeds (Adafruit IO prefers individual feed updates)
  mqtt.publish(TOPIC_STATUS, "online", true);  // true: retained
  delay(2000);                                 // prevent THROTTLE WARNING
  mqtt.publish(TOPIC_TEMP, String(temperature).c_str());
  delay(2000);
  mqtt.publish(TOPIC_RSSI, String((long)WiFi.RSSI()).c_str());
  delay(2000);
  mqtt.publish(TOPIC_UPTIME, String((unsigned long long)(millis() / 1000)).c_str());
  delay(2000);
  Serial.printf("Published Telemetry** Temp: %d C RSSI: %ld Uptime: %llu LED State: %s\n",
                temperature,
                (long)WiFi.RSSI(),
                ((unsigned long long)(millis() / 1000)),
                ledState ? "1" : "0");

#else
  char payload[128];
  snprintf(
    payload,
    sizeof(payload),
    "{\"temp\":%d,\"rssi\":%ld,\"uptime_sec\":%llu}",
    temperature,
    (long)WiFi.RSSI(),
    (unsigned long long)(millis() / 1000));

  bool ok = mqtt.publish(TOPIC_TELEMETRY, payload);

  if (ok) {
    Serial.printf("Published telemetry: %s\n", payload);
  } else {
    Serial.println("Telemetry publish failed");
  }
#endif
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

#ifdef SECURE_LOGIN
  // --- Quick test mode: skip certificate verification ---
  secureClient.setInsecure();

  // Increase SSL handshake timeout to handle cloud latency (in seconds)
  secureClient.setHandshakeTimeout(30);
#endif

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