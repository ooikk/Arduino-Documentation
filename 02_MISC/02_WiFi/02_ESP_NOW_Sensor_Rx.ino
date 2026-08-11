/*
==Encrypted ESP-NOW with ESP32-S3 Deep Sleep for battery-powered sensor RECEIVER nodes==

1. Always-On Encrypted Receiver (Gateway)
 Because ESP-NOW AES-128 encryption requires hardware key handshake verification, 
 the receiver MUST register the sender's MAC address as an encrypted peer with the 
 exact same PMK and LMK keys.
2. Low-Power "Synchronized Window" Receiver (Optional ESP_DEEPSLEEP)
 If your receiver must also run on a battery and use Deep Sleep:
 - Both boards use the same timer interval (e.g., wake up every 10 seconds).
 - On wake-up, the receiver keeps its radio active for a 100 ms listening window.
 - If data is received (or the window expires), the receiver logs the data and returns to sleep.
*/

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define ESP_DEEPSLEEP
#ifdef ESP_DEEPSLEEP
#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP (10 - 3)  // Sleep 10 seconds (Matches sender interval)
#define LISTEN_WINDOW 13000 //13 sec  150       // Listen for 150ms before giving up
volatile bool dataReceived = false;
#endif


// ⚠️ REPLACE WITH YOUR SENDER NODE'S MAC ADDRESS
uint8_t senderAddress[] = { 0xAC, 0xA7, 0x04, 0xE0, 0x4E, 0x64 };  // COM3
//uint8_t senderAddress[] = { 0x44, 0x1B, 0xF6, 0xD6, 0x3E, 0x30 };  // COM4

// 16-Byte AES Keys (MUST MATCH SENDER EXACTLY)
static const char PMK_KEY[] = "16BytePMKKey1234";
static const char LMK_KEY[] = "16ByteLMKKey5678";

// Payload Structure (MUST MATCH SENDER EXACTLY)
typedef struct struct_sensor_data {
  int boot_number;
  float temperature;
  float battery_voltage;
} struct_sensor_data;

struct_sensor_data incomingSensorData;
esp_now_peer_info_t peerInfo;

// Receive Callback (Fires instantly when decrypted packet arrives)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingDataPtr, int len) {
  const uint8_t *mac = recv_info->src_addr;
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len) {
#endif

  memcpy(&incomingSensorData, incomingDataPtr, sizeof(incomingSensorData));

  Serial.println("==================================================");
  Serial.printf("📥 Decrypted Packet Received from MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.printf("  Boot / Transmission Count : #%d\n", incomingSensorData.boot_number);
  Serial.printf("  Temperature               : %.2f °C\n", incomingSensorData.temperature);
  Serial.printf("  Battery Voltage           : %.2f V\n", incomingSensorData.battery_voltage);
  Serial.println("==================================================\n");
#ifdef ESP_DEEPSLEEP
  dataReceived = true;
#endif
}

#ifdef ESP_DEEPSLEEP
void goToSleep() {
  Serial.println("Entering Deep Sleep now...");
  Serial.flush();  // Ensure serial logs are printed before power down  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_wifi_stop();
  esp_deep_sleep_start();
}
#endif

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 1. Set Wi-Fi Station Mode & Pin to Channel 1 (Must match sender)
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  // 2. Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
#ifdef ESP_DEEPSLEEP
    goToSleep();
#else
    return;
#endif
  }

  // 3. Set Primary Master Key (PMK)
  esp_now_set_pmk((uint8_t *)PMK_KEY);

  // 4. Register Receive Callback
  esp_now_register_recv_cb(OnDataRecv);

  // 5. Register Deep-Sleep Sender as an Encrypted Peer
  // (Required for ESP32 hardware AES to decrypt frames from this specific MAC)
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, senderAddress, 6);
  peerInfo.channel = 1;     // Fixed Channel 1
  peerInfo.encrypt = true;  // Enable AES-128 Decryption
  memcpy(peerInfo.lmk, LMK_KEY, 16);

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add encrypted sender peer!");
#ifdef ESP_DEEPSLEEP
    goToSleep();
#else
    return;
#endif
  }
  Serial.println("Encrypted ESP-NOW Gateway Listening on Channel 1...");
#ifdef ESP_DEEPSLEEP
  // 100ms Synchronized RX Listening Window
  uint32_t startWindow = millis();
  while (!dataReceived && (millis() - startWindow < LISTEN_WINDOW)) {
    delay(1);
  }

  if (dataReceived) {
    Serial.printf("📥 Rx Boot #%d | Temp: %.1f°C | Batt: %.2fV\n",
                  incomingSensorData.boot_number,
                  incomingSensorData.temperature,
                  incomingSensorData.battery_voltage);
  } else {
    Serial.println("⚠️ Listen window timed out. No packet caught.");
  }

  goToSleep();
#endif
}

void loop() {
  // Main loop remains free for processing, MQTT bridging, SD logging, etc.
}