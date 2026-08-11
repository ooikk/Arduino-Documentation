/*
This is the center gateway
Auto sync the wake up time of all sensor nodes
*/
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define PEER_CHANNEL 1
#define GATEWAY_SLEEP_SEC 10    // Gateway sleep duration (Must match Sensor Nodes)
#define LISTEN_WINDOW_MS 12000  // wait for > 10secs for auto sync // Active window (1.5s) to collect packets from all nodes
#define WAKEUP_BUFFER_SEC 1     // Wake up 3 secs earlier

// 16-Byte Encryption Keys (MUST MATCH ALL SENSOR NODES)
static const char *PMK_KEY = "PMK_KEY_12345678";
static const char *LMK_KEY = "LMK_KEY_12345678";

// ⚠️ List all Sensor Node MAC Addresses here
uint8_t knownNodes[][6] = {
  { 0xAC, 0xA7, 0x04, 0xE0, 0x4E, 0x64 },  // Node 1 (COM3)
  { 0xE8, 0x3D, 0xC1, 0xFA, 0x69, 0x40 },  // Node 2 (COM7)
  //{ 0x30, 0xAE, 0xA4, 0x77, 0x55, 0x88 }  // Node 3 (Add as many as needed)
};

// Auto-calculate number of registered nodes
const size_t TOTAL_NODES = sizeof(knownNodes) / sizeof(knownNodes[0]);
int numNode = 0;  // Reset to 0 at each wake up from deep sleep
uint32_t currentMilliSecs = 0;
uint32_t startWakeUpmilliSecs = 0;

// Array of flags to track receipt status of each node
volatile bool dataReceived[TOTAL_NODES] = { false };

// Preserved across deep sleep cycles
RTC_DATA_ATTR uint32_t gatewayBootCount = 0;

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

struct_sensor_data incomingSensorData;
struct_command outgoingCommand;

// Helper: Check if all registered nodes have sent their data
bool allNodesReceived() {
  for (size_t i = 0; i < TOTAL_NODES; i++) {
    if (!dataReceived[i]) return false;
  }
  return true;
}

// Function to pre-register multiple nodes at once
void registerAllNodePeers(const uint8_t macList[][6], size_t count) {
  for (size_t i = 0; i < count; i++) {
    const uint8_t *mac = macList[i];

    if (!esp_now_is_peer_exist(mac)) {
      esp_now_peer_info_t peerInfo = {};
      memcpy(peerInfo.peer_addr, mac, 6);
      peerInfo.channel = PEER_CHANNEL;
      peerInfo.encrypt = true;
      memcpy(peerInfo.lmk, LMK_KEY, 16);

      esp_err_t result = esp_now_add_peer(&peerInfo);
      if (result == ESP_OK) {
        Serial.printf("✅ Pre-registered Node [%02X:%02X:%02X:%02X:%02X:%02X]\n",
                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      } else {
        Serial.printf("❌ Failed to register [%02X:%02X:%02X:%02X:%02X:%02X] - Err: %d\n",
                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], result);
      }
    }
  }
}

// Receive Callback
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingDataPtr, int len) {
  const uint8_t *mac = recv_info->src_addr;
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len) {
#endif
  if (len == sizeof(struct_sensor_data)) {
    memcpy(&incomingSensorData, incomingDataPtr, sizeof(incomingSensorData));

    Serial.printf("📥 Received from Node #%d [%02X:%02X:%02X:%02X:%02X:%02X]\n",
                  incomingSensorData.node_id,
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.printf("Temperature: %.2f C\n", incomingSensorData.temperature);
    Serial.printf("Humidity: %.2f %\n", incomingSensorData.humidity);
    Serial.printf("Vcc: %.2f mV\n", incomingSensorData.vcc_mv);

    // 1. Register node as encrypted peer
    //ensurePeerRegistered(mac);

    // 2. Prepare Command Response
    outgoingCommand.target_node_id = incomingSensorData.node_id;
    //outgoingCommand.sleep_duration_sec = GATEWAY_SLEEP_SEC;  // Re-sync node sleep time
    currentMilliSecs = millis() - startWakeUpmilliSecs;
    outgoingCommand.sleep_duration_sec = GATEWAY_SLEEP_SEC - currentMilliSecs / 1000;  // Re-sync node sleep time

    outgoingCommand.relay_state = (incomingSensorData.boot_count % 2 == 0);

    Serial.printf("------Data sent to node: %d-----\n", outgoingCommand.target_node_id);
    Serial.printf("Sleep Duration: %d sec | Relay State: %s \n",
                  outgoingCommand.sleep_duration_sec,
                  outgoingCommand.relay_state ? "ON" : "OFF");

    // 3. Immediate Encrypted Reply
    esp_now_send(mac, (uint8_t *)&outgoingCommand, sizeof(outgoingCommand));
    // 4. Match MAC address to mark specific node index as received

    for (size_t i = 0; i < TOTAL_NODES; i++) {
      if (memcmp(mac, knownNodes[i], 6) == 0) {
        dataReceived[i] = true;
        numNode++;
        //Serial.printf("Status: Node %d/%d received.\n", (int)i + 1, (int)TOTAL_NODES);
        Serial.printf("Status: Node %d/%d received.\n", numNode, (int)TOTAL_NODES);
        break;
      }
    }
  }
}

//void goToSleep(uint32_t seconds) {
void goToSleep(int seconds) {  
  currentMilliSecs = millis() - startWakeUpmilliSecs;
  seconds = seconds - WAKEUP_BUFFER_SEC - (int) currentMilliSecs / 1000;
  if (seconds < 1) seconds = 1;
  Serial.printf("Gateway going to deep sleep for %d seconds...\n\n", seconds);
  Serial.flush();
  esp_wifi_stop();

  esp_sleep_enable_timer_wakeup((seconds - 0) * 1000000ULL);  // wake up 1 second earlier

  //esp_sleep_enable_timer_wakeup(((seconds - WAKEUP_BUFFER_SEC) * 1000ULL - currentMilliSecs) * 1000ULL);  // wake up 1 second earlier
  esp_deep_sleep_start();
}

void setup() {

  startWakeUpmilliSecs = millis();

  Serial.begin(115200);
  gatewayBootCount++;

  Serial.printf("\n--- Gateway Wakeup #%d ---\n", gatewayBootCount);

  // 1. Lock Wi-Fi Channel
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(PEER_CHANNEL, WIFI_SECOND_CHAN_NONE);


  // 2. Init ESP-NOW & PMK
  if (esp_now_init() != ESP_OK) {
    goToSleep(GATEWAY_SLEEP_SEC);
  }
  /*
  delay(100);
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
*/
  esp_now_set_pmk((uint8_t *)PMK_KEY);
  esp_now_register_recv_cb(OnDataRecv);

  // ⚠️ PRE-REGISTER ALL SENSOR NODES IN ONE CALL
  registerAllNodePeers(knownNodes, TOTAL_NODES);

  // 3. Keep radio open for the active listening window
  uint32_t startTime = millis();
  while (!allNodesReceived() && (millis() - startTime < LISTEN_WINDOW_MS)) {
    delay(10);  // Process incoming ESP-NOW callbacks
  }
  if (allNodesReceived()) {
    Serial.println("\n🎉 All nodes successfully received!");
  } else {
    Serial.println("\n⚠️ Timed out waiting for remaining nodes.");
  }

  // 4. Return to Deep Sleep
  goToSleep(GATEWAY_SLEEP_SEC);
}

void loop() {
  // Unused
}
