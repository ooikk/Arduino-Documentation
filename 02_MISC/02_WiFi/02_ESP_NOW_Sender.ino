#include <WiFi.h>
#include <esp_now.h>

#define ESP_DEEPSLEEP
#ifdef ESP_DEEPSLEEP

#include <esp_wifi.h>
#define SLEEP_TIME_SEC 10  // Sleep duration in seconds
/* 
Fixed Channel Channel Setting: Passing channel = 0 forces ESP32 Wi-Fi to scan channels 1 to 11. 
Explicitly hardcoding channel 1 via esp_wifi_set_channel(1, ...) cuts active wake time by up to 80%.
*/
#define PEER_CHANNEL 1

// RTC RAM: This variable survives deep sleep resets
RTC_DATA_ATTR int bootCounter = 0;
// Volatile flag updated by the transmission callback
volatile bool deliveryComplete = false;
#else
#define PEER_CHANNEL 0
#endif


// REPLACE WITH YOUR RECEIVER'S MAC ADDRESS
uint8_t receiverAddress[] = { 0xAC, 0xA7, 0x04, 0xE0, 0x4E, 0x64 };

// Data structure to send (Must match Receiver structure)
typedef struct struct_message {
  int counter;
  float temperature;
  bool state;
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

// Send Callback Function
// Receive Callback Function (Compatible with ESP32 Core v2.x and v3.x)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
// Send Callback Function (Updated for ESP32 Arduino Core 3.x)
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
#else
// Send Callback Function (Updated for ESP32 Arduino Core 2.x)
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#endif

  Serial.print("Last Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");

#ifdef ESP_DEEPSLEEP
  // Signal setup() that packet transmission is finished
  deliveryComplete = true;
#endif
}

#ifdef ESP_DEEPSLEEP
void goToSleep() {
  Serial.printf("Entering deep sleep for %d seconds...\n", SLEEP_TIME_SEC);
  Serial.flush();

  esp_sleep_enable_timer_wakeup(SLEEP_TIME_SEC * 1000000ULL);
  esp_wifi_stop();  // Shut down Wi-Fi PHY for lowest power consumption
  esp_deep_sleep_start();
}
#endif

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  delay(100);

#ifdef ESP_DEEPSLEEP
  esp_wifi_set_channel(PEER_CHANNEL, WIFI_SECOND_CHAN_NONE);
  bootCounter++;
#endif

  Serial.print("My MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");

#ifdef ESP_DEEPSLEEP
    goToSleep();
#else
    return;
#endif
  }

  // Register send callback
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = PEER_CHANNEL;  // 0 means current Wi-Fi channel
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
#ifdef ESP_DEEPSLEEP
    goToSleep();
#else
    return;
#endif
  }

#ifdef ESP_DEEPSLEEP
  // 5. Prepare payload
  myData.counter = bootCounter;
  myData.temperature = 25.4 + random(-100, 100) / 10.0;
  myData.state = true;

  // 6. Queue transmission
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)&myData, sizeof(myData));

  if (result != ESP_OK) {
    Serial.println("Error queuing ESP-NOW frame");
    goToSleep();
  }

  Serial.printf("Sent packet #%d\n", myData.counter);
  Serial.printf("Sent temperature %.2f °C\n", myData.temperature);
  Serial.printf("Sent state %d\n", myData.state);
  Serial.printf("Sent state: %s\n", myData.state ? "TRUE" : "FALSE");

  // 7. CRITICAL: Block execution until OnDataSent sets flag (or 250ms safety timeout)
  uint32_t startTime = millis();
  while (!deliveryComplete && (millis() - startTime < 250)) {
    delay(1);
  }

  // 8. Transmission verified over-the-air; proceed to sleep
  goToSleep();
#else
  Serial.println("Sender Ready!");
#endif
}

void loop() {

#ifndef ESP_DEEPSLEEP

  // Populate message data
  myData.counter++;
  myData.temperature = 25.0 + random(-100, 100) / 10.0;
  myData.state = true;

  // Send packet to peer
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)&myData, sizeof(myData));

  if (result == ESP_OK) {
    Serial.printf("Sent packet #%d\n", myData.counter);
    Serial.printf("Sent temperature %.2f °C\n", myData.temperature);
    Serial.printf("Sent state %d\n", myData.state);
    Serial.printf("Sent state: %s\n", myData.state ? "TRUE" : "FALSE");
  } else {
    Serial.println("Error sending the data");
  }

  delay(2000);  // Send data every 2 seconds
#endif
}
