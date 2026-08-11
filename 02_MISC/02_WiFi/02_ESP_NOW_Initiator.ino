#include <WiFi.h>
#include <esp_now.h>

// ⚠️ REPLACE WITH BOARD B's MAC ADDRESS
//uint8_t boardBMac[] = { 0xAC, 0xA7, 0x04, 0xE0, 0x4E, 0x64 }; // COM3
uint8_t boardBMac[] = { 0x44, 0x1B, 0xF6, 0xD6, 0x3E, 0x30 };  // COM4

// Shared Data Structures
typedef struct struct_command {
  int command_id;    // e.g., 1 = Toggle LED, 2 = Read Sensor
  bool trigger_state;
} struct_command;

typedef struct struct_response {
  int response_from; // Board ID
  float temp_reading;
  bool led_status;
} struct_response;

struct_command outgoingCmd;
struct_response incomingData;
esp_now_peer_info_t peerInfo;

// Delivery Confirmation Callback (Compatible with ESP32 Core v2.x and v3.x)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
#else
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#endif
  Serial.print("Transmission to Board B: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivered" : "Failed");
}

// Data Received Callback
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingDataPtr, int len) {
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len) {
#endif
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));

  Serial.println("\n📥 [Board A] Received Reply from Board B:");
  Serial.printf("  Sender ID:    Board #%d\n", incomingData.response_from);
  Serial.printf("  Temp Sensor:  %.2f °C\n", incomingData.temp_reading);
  Serial.printf("  LED Status:   %s\n", incomingData.led_status ? "ON" : "OFF");
  Serial.println("----------------------------------------");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register BOTH callbacks
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Register Board B as a Peer
  memcpy(peerInfo.peer_addr, boardBMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add Board B as peer");
    return;
  }

  Serial.println("Board A (Initiator) Ready!");
}

void loop() {
  static int cmdCount = 0;
  
  // Prepare command
  outgoingCmd.command_id = ++cmdCount;
  outgoingCmd.trigger_state = (cmdCount % 2 == 0);

  Serial.printf("\n📤 [Board A] Sending Command: #%d to Board B...\n", outgoingCmd.command_id);
  Serial.printf("📤 [Board A] Sending Trigger State: %s to Board B...\n", outgoingCmd.trigger_state ? "HIGH" : "LOW");

  // Transmit command to Board B
  esp_now_send(boardBMac, (uint8_t *) &outgoingCmd, sizeof(outgoingCmd));

  delay(2000); // Send request every 2 seconds
}