#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define WIFI_CHANNEL 1
#define WAKEUP_BUFFER_MS 150         // Wake up 150 ms early to catch broadcast
#define MISS_SYNC_TIMEOUT_MS 15000   // Stay awake if broadcast is missed (>15s)

typedef struct struct_message {
  uint32_t msg_id;
  uint32_t next_broadcast_ms;
  float temperature;
  char command[16];
} struct_message;

struct_message rxData;
volatile bool packetReceived = false;
int32_t calculatedSleepMs = 0;

void goToSleepMs(int32_t sleepMs) {
  const int32_t MIN_SLEEP_MS = 10;
  if (sleepMs < MIN_SLEEP_MS) sleepMs = MIN_SLEEP_MS;

  Serial.printf("Receiver sleeping for %d ms (Buffer: %d ms)...\n\n", 
                sleepMs, WAKEUP_BUFFER_MS);
  Serial.flush();

  esp_wifi_stop();
  esp_sleep_enable_timer_wakeup((uint64_t)sleepMs * 1000ULL);
  esp_deep_sleep_start();
}

// ESP32 Arduino Core v3.x Callback Signature
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
  memcpy(&rxData, incomingData, sizeof(rxData));

  // Subtract wakeup margin from the broadcaster's remaining cycle time
  int32_t targetSleepMs = (int32_t)rxData.next_broadcast_ms - WAKEUP_BUFFER_MS;

  calculatedSleepMs = targetSleepMs;
  packetReceived = true;
}

/* 
// For ESP32 Arduino Core v2.x, use this signature instead:
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&rxData, incomingData, sizeof(rxData));
  calculatedSleepMs = (int32_t)rxData.next_broadcast_ms - WAKEUP_BUFFER_MS;
  packetReceived = true;
}
*/

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Receiver active. Listening for broadcast sync signal...");
}

void loop() {
  // 1. If packet was received, enter synchronized deep sleep immediately
  if (packetReceived) {
    Serial.printf("Sync Received | Msg ID: %u | Temp: %.1f C | Cmd: %s\n",
                  rxData.msg_id, rxData.temperature, rxData.command);
    goToSleepMs(calculatedSleepMs);
  }

  // 2. If initial boot or missed sync, keep listening in awake state
  static uint32_t lastWarningMs = 0;
  if (millis() - lastWarningMs > 5000) {
    lastWarningMs = millis();
    Serial.printf("Listening... Awake time: %.2f sec\n", millis() / 1000.0f);
  }

  delay(10);
}