#include <WiFi.h>
#include <esp_now.h>

#define ESP_DEEPSLEEP
#ifdef ESP_DEEPSLEEP
#include <esp_wifi.h>

#define LISTEN_TIMEOUT_MS 12000 // Stay awake for 12 seconds listening for a packet
#define SLEEP_TIME_SEC (10-3)   // Deep sleep duration in seconds
#define PEER_CHANNEL 1
// Flag to notify setup() when data arrives
volatile bool messageReceived = false;
#endif

// Data structure to receive (Must match Sender structure)
typedef struct struct_message {
  int counter;
  float temperature;
  bool state;
  char message[180];    // Fixed array buffer (keeps total struct under 250 bytes)
} struct_message;

struct_message incomingData;

// Receive Callback Function (Compatible with ESP32 Core v2.x and v3.x)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingDataPtr, int len) {
  const uint8_t *mac = recv_info->src_addr; // Core 3.x uses arrow operator
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len) {
#endif
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));

  Serial.println("--- New Message Received ---");
  /*
    Serial.printf("From: %02x:%02x:%02x:%02x:%02x:%02x\n",
                  recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
                  recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);
  */
  Serial.printf("From: %02x:%02x:%02x:%02x:%02x:%02x\n",
                  mac[0], mac[1], mac[2],mac[3], mac[4], mac[5]);
  Serial.printf("Bytes received: %d\n", len);
  Serial.printf("Counter: %d\n", incomingData.counter);
  Serial.printf("Temperature: %.2f °C\n", incomingData.temperature);
  Serial.printf("State: %s\n", incomingData.state ? "TRUE" : "FALSE");
  Serial.printf("message: %s\n", incomingData.message);
  Serial.println();

#ifdef ESP_DEEPSLEEP
// Set flag so setup() knows we received data
  messageReceived = true;
#endif
}

#ifdef ESP_DEEPSLEEP
void goToSleep() {
  Serial.printf("Entering deep sleep for %d seconds...\n", SLEEP_TIME_SEC);
  Serial.flush();
  
  esp_wifi_stop(); // Shut down Wi-Fi PHY radio for lowest power draw
  esp_sleep_enable_timer_wakeup(SLEEP_TIME_SEC * 1000000ULL);
  esp_deep_sleep_start();
}
#endif

void setup() {
  Serial.begin(115200);
  delay(1000);

// 1. Set Wi-Fi STA mode and lock to Channel 1 (Must match sender channel!)
  WiFi.mode(WIFI_STA);
  delay(100);

#ifdef ESP_DEEPSLEEP
  esp_wifi_set_channel(PEER_CHANNEL, WIFI_SECOND_CHAN_NONE);
#endif

  Serial.print("My MAC Address: ");
  Serial.println(WiFi.macAddress());

// 2. Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
#ifdef ESP_DEEPSLEEP    
    goToSleep();
#else
    return;
#endif
  }

// 3. Register callback
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Receiver Listening...");

#ifdef ESP_DEEPSLEEP   
// 4. Wait until a message arrives OR timeout expires
  uint32_t startTime = millis();
  while (!messageReceived && (millis() - startTime < LISTEN_TIMEOUT_MS)) {
    delay(10);
  }
  if (!messageReceived) {
    Serial.println("Listen timeout reached without receiving data.");
  }

// 5. Go back to sleep
  goToSleep();
#endif

}

void loop() {
  // Nothing needed here. Incoming packets trigger OnDataRecv automatically.
}