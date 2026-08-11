#include <WiFi.h>
#include <esp_now.h>

// ⚠️ REPLACE WITH BOARD A's MAC ADDRESS
uint8_t boardAMac[] = { 0xAC, 0xA7, 0x04, 0xE0, 0x4E, 0x64 };  // COM3
//uint8_t boardAMac[] = { 0x44, 0x1B, 0xF6, 0xD6, 0x3E, 0x30 };  // COM4

// Shared Data Structures
typedef struct struct_command {
  int command_id;
  bool trigger_state;
} struct_command;

typedef struct struct_response {
  int response_from;
  float temp_reading;
  bool led_status;
} struct_response;

struct_command incomingCmd;
struct_response outgoingReply;
esp_now_peer_info_t peerInfo;

const int LED_PIN = 2;  // Built-in LED

// Delivery Confirmation Callback (Compatible with ESP32 Core v2.x and v3.x)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
#else
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#endif
  Serial.print("Reply Delivery to Board A: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingDataPtr, int len) {
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len) {
#endif
  memcpy(&incomingCmd, incomingDataPtr, sizeof(incomingCmd));

  Serial.println("\n📥 [Board B] Command Received from Board A!");
  Serial.printf("  Command ID: %d | Requested State: %s\n",
                incomingCmd.command_id,
                incomingCmd.trigger_state ? "HIGH" : "LOW");

  // 1. Execute hardware action based on command
  digitalWrite(LED_PIN, incomingCmd.trigger_state ? HIGH : LOW);

  // 2. Prepare reply payload
  outgoingReply.response_from = 2;
  outgoingReply.temp_reading = 24.0 + (random(-100, 100) / 10.0);  // Simulated sensor
  outgoingReply.led_status = incomingCmd.trigger_state;

  // 3. Immediately reply back to Board A
  Serial.println("📤 [Board B] Sending status ACK back to Board A...\n");
  Serial.printf("Response from: #%d\n", outgoingReply.response_from);
  Serial.printf("Temperature: %.2f\n", outgoingReply.temp_reading);
  Serial.printf("LED Status: %s\n", outgoingReply.led_status ? "HIGH" : "LOW");

  esp_now_send(boardAMac, (uint8_t *)&outgoingReply, sizeof(outgoingReply));
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Register Board A as a Peer
  memcpy(peerInfo.peer_addr, boardAMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add Board A as peer");
    return;
  }

  Serial.println("Board B (Responder) Listening...");
}

void loop() {
  // Loop remains empty; responses are event-triggered in OnDataRecv
}
