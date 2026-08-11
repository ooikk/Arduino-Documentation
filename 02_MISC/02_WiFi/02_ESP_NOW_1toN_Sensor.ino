/*
This is one of N nodes
*/

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define NODE_ID 1           // Unique ID for this sensor node
#define PEER_CHANNEL 1      // Must match Gateway channel
#define SLEEP_SEC 10        // 10-second deep sleep cycle
#define ACK_TIMEOUT_MS 150  // Reply wait window in milliseconds

// 16-Byte Encryption Keys (MUST MATCH GATEWAY)
static const char *PMK_KEY = "PMK_KEY_12345678";
static const char *LMK_KEY = "LMK_KEY_12345678";

// ⚠️ REPLACE WITH YOUR GATEWAY'S MAC ADDRESS
//uint8_t gatewayMac[] = { 0xAC, 0xA7, 0x04, 0xE0, 0x4E, 0x64 }; // COM3
uint8_t gatewayMac[] = { 0x44, 0x1B, 0xF6, 0xD6, 0x3E, 0x30 };  // COM4


// Preserved across deep sleep cycles
RTC_DATA_ATTR uint32_t bootCount = 0;
RTC_DATA_ATTR uint16_t currentSleepSec = SLEEP_SEC;

typedef struct struct_sensor_data {
  uint8_t node_id;
  float temperature;
  float humidity;
  uint16_t vcc_mv;
  uint32_t boot_count;
} struct_sensor_data;

typedef struct struct_command {
  uint8_t target_node_id;
  uint16_t sleep_duration_sec;
  bool relay_state;
} struct_command;

struct_sensor_data sensorPayload;
struct_command incomingCommand;

volatile bool commandReceived = false;

// Transmission Status Callback
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
#else
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#endif
  Serial.printf("Tx Status to Gateway: %s\n", status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Receive Callback (Compatible with ESP32 Core v2.x and v3.x)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingDataPtr, int len) {
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len) {
#endif
  if (len == sizeof(struct_command)) {
    memcpy(&incomingCommand, incomingDataPtr, sizeof(incomingCommand));
    if (incomingCommand.target_node_id == NODE_ID) {
      commandReceived = true;
    }
  }
}

void goToSleep(uint32_t seconds) {
  Serial.printf("Node #%d going to deep sleep for %d seconds...\n\n", NODE_ID, seconds);
  Serial.flush();
  esp_wifi_stop();
  esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  bootCount++;

  Serial.printf("\n--- Node #%d Wakeup #%d ---\n", NODE_ID, bootCount);

  // 1. Set Wi-Fi Channel
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(PEER_CHANNEL, WIFI_SECOND_CHAN_NONE);


  // 2. Init ESP-NOW & PMK
  if (esp_now_init() != ESP_OK) {
    goToSleep(currentSleepSec);
  }
  /*
  delay(100);
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
*/
  esp_now_set_pmk((uint8_t *)PMK_KEY);
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // 3. Register Gateway Encrypted Peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, gatewayMac, 6);
  peerInfo.channel = PEER_CHANNEL;
  peerInfo.encrypt = true;
  memcpy(peerInfo.lmk, LMK_KEY, 16);

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add Gateway peer");
    goToSleep(currentSleepSec);
  }

  // 4. Sample Sensor Data
  sensorPayload.node_id = NODE_ID;
  sensorPayload.temperature = 24.5 + (random(-20, 20) / 10.0);
  sensorPayload.humidity = 60.0 + (random(-50, 50) / 10.0);
  sensorPayload.vcc_mv = 3300 + random(-200, 200);
  sensorPayload.boot_count = bootCount;

  // 5. Transmit Encrypted Payload to Gateway
  esp_now_send(gatewayMac, (uint8_t *)&sensorPayload, sizeof(sensorPayload));
  Serial.printf("------Data sent:------\n");
  Serial.printf("Temperature: %.2f C\n", sensorPayload.temperature);
  Serial.printf("Humidity: %.2f %\n", sensorPayload.humidity);
  Serial.printf("Vcc: %.2f mV\n", sensorPayload.vcc_mv);

  // 6. Brief Listening Window for Gateway Reply (150 ms)
  uint32_t startTime = millis();
  while (!commandReceived && (millis() - startTime < ACK_TIMEOUT_MS)) {
    delay(1);
  }

  // 7. Process Gateway Reply (if received)
  if (commandReceived) {
    Serial.printf("📥 Command Received! Node ID: %d\n", incomingCommand.target_node_id);
    Serial.printf("Gateway Sleep Sync: %d sec | Relay State: %s\n", incomingCommand.sleep_duration_sec, incomingCommand.relay_state ? "ON" : "OFF");
    if (incomingCommand.sleep_duration_sec > 0) {
      currentSleepSec = incomingCommand.sleep_duration_sec;
    }
  } else {
    Serial.println("⚠️ No Gateway reply within 150ms window.");
  }

  // 8. Return to Deep Sleep for 60 Seconds
  goToSleep(currentSleepSec);
}

void loop() {
  // Unused
}
