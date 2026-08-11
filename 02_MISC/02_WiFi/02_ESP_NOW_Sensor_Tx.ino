/*
==Encrypted ESP-NOW with ESP32-S3 Deep Sleep for battery-powered sensor nodes==

This code wakes up every 10 seconds (configurable), reads a simulated temperature sensor, 
increments a persistent boot counter stored in RTC memory, transmits the encrypted payload 
to the receiver board, and goes back to sleep immediately.
*/

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ====================================================================
// DEEP SLEEP & RTC CONFIGURATION
// ====================================================================
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 10          /* Time ESP32 will go to sleep (in seconds) */

// Persistent RTC memory variable (retained across deep sleep cycles)
RTC_DATA_ATTR int bootCount = 0;

// ====================================================================
// ENCRYPTION & PEER CONFIGURATION
// ====================================================================
// ⚠️ REPLACE WITH YOUR RECEIVER BOARD'S MAC ADDRESS
//uint8_t receiverAddress[] = { 0xAC, 0xA7, 0x04, 0xE0, 0x4E, 0x64 }; // COM3
uint8_t receiverAddress[] = { 0x44, 0x1B, 0xF6, 0xD6, 0x3E, 0x30 };  // COM4


// 16-Byte AES Keys (MUST match receiver exactly)
static const char PMK_KEY[] = "16BytePMKKey1234";
static const char LMK_KEY[] = "16ByteLMKKey5678";

// Payload Structure
typedef struct struct_sensor_data {
  int boot_number;
  float temperature;
  float battery_voltage;
} struct_sensor_data;

struct_sensor_data sensorPayload;
esp_now_peer_info_t peerInfo;

// Flag to track transmission status
volatile bool sendCompleted = false;

// Delivery Confirmation Callback (Compatible with ESP32 Core v2.x and v3.x)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
#else
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#endif

  Serial.print("Encrypted Delivery Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success (ACK)" : "Fail");
  sendCompleted = true;
}

void setup() {
  Serial.begin(115200);

  // 1. Increment persistent boot counter
  bootCount++;
  Serial.printf("\n--- Wakeup #%d ---\n", bootCount);

  // 2. Read Sensor Data (Keep code fast!)
  sensorPayload.boot_number = bootCount;
  sensorPayload.temperature = 22.0 + (random(-100, 100) / 10.0);  // Simulated sensor
  sensorPayload.battery_voltage = 3.72+ (random(-10, 10) / 100.0);  // Simulated ADC reading

  // 3. Initialize Wi-Fi Station Mode & Pin Channel
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);  // Fixed Channel 1 required for AES

  // 4. Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed! Sleeping...");
    goToSleep();
  }

  // 5. Set Primary Master Key (PMK)
  esp_now_set_pmk((uint8_t *)PMK_KEY);

  // 6. Register Send Callback
  esp_now_register_send_cb(OnDataSent);

  // 7. Add Encrypted Peer
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 1;     // Must match explicit channel set above
  peerInfo.encrypt = true;  // Enable AES-128
  memcpy(peerInfo.lmk, LMK_KEY, 16);

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer! Sleeping...");
    goToSleep();
  }

  // 8. Transmit Encrypted Payload
  Serial.printf("Sending Payload: Boot #%d | Temp: %.2f°C | Battery: %.2fV\n",
                sensorPayload.boot_number, sensorPayload.temperature, sensorPayload.battery_voltage);

  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)&sensorPayload, sizeof(sensorPayload));

  if (result != ESP_OK) {
    Serial.println("Send Initiation Failed!");
    goToSleep();
  }

  // 9. Wait for hardware ACK callback with a short 500ms safeguard timeout
  uint32_t startTime = millis();
  while (!sendCompleted && (millis() - startTime < 500)) {
    delay(1);  // Short pause to keep CPU waiting efficiently
  }

  // 10. Enter Deep Sleep immediately
  goToSleep();
}

void goToSleep() {
  Serial.println("Entering Deep Sleep now...");
  Serial.flush();  // Ensure serial logs are printed before power down

  // Enable timer wakeup
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  // Turn off Wi-Fi radio explicitly for maximum power saving
  esp_wifi_stop();

  // Enter Deep Sleep
  esp_deep_sleep_start();
}

void loop() {
  // Never reached! Execution re-enters setup() on wakeup.
}
