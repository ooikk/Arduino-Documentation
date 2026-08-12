#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define WIFI_CHANNEL 1
#define BROADCAST_INTERVAL_MS 10000 // 10-second total cycle time

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct struct_message {
  uint32_t msg_id;
  uint32_t next_broadcast_ms; // Remaining ms until the NEXT broadcast
  float temperature;
  char command[16];
} struct_message;

struct_message txData;
esp_now_peer_info_t peerInfo;
RTC_DATA_ATTR uint32_t msgCounter = 0; // Retain counter across deep sleep

void goToSleepMs(int32_t sleepMs) {
  const int32_t MIN_SLEEP_MS = 100;
  if (sleepMs < MIN_SLEEP_MS) sleepMs = MIN_SLEEP_MS;

  Serial.printf("Broadcaster entering deep sleep for %d ms...\n\n", sleepMs);
  Serial.flush();

  esp_wifi_stop();
  esp_sleep_enable_timer_wakeup((uint64_t)sleepMs * 1000ULL);
  esp_deep_sleep_start();
}

void setup() {
  uint32_t startMs = millis();
  Serial.begin(115200);

  // Initialize Wi-Fi in Station Mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    goToSleepMs(BROADCAST_INTERVAL_MS);
  }

  // Register broadcast peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = WIFI_CHANNEL;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  // Calculate elapsed active awake time before transmission
  uint32_t activeElapsedMs = millis() - startMs;

  // Prepare Payload
  txData.msg_id = ++msgCounter;
  txData.temperature = 24.2f + (rand() % 40) / 10.0f;
  snprintf(txData.command, sizeof(txData.command), "SYNC_STATE");
  
  // Inform receivers how many milliseconds remain until the NEXT broadcast window
  txData.next_broadcast_ms = BROADCAST_INTERVAL_MS - activeElapsedMs;

  // Transmit Broadcast
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&txData, sizeof(txData));
  
  if (result == ESP_OK) {
    Serial.printf("[%u] Broadcast Sent! Next broadcast in: %u ms\n", 
                  txData.msg_id, txData.next_broadcast_ms);
  } else {
    Serial.println("Broadcast send failed!");
  }

  // Calculate Broadcaster's remaining sleep duration
  uint32_t totalAwakeMs = millis() - startMs;
  int32_t broadcasterSleepMs = (int32_t)BROADCAST_INTERVAL_MS - (int32_t)totalAwakeMs;

  goToSleepMs(broadcasterSleepMs);
}

void loop() {
  // Never reached due to deep sleep
}