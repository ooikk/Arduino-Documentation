#include <WiFi.h>
#include <esp_wifi.h>

// --- Configuration ---
const unsigned long CHANNEL_SWITCH_INTERVAL = 3000;  // Time in ms to stay on one channel
const int MAX_CHANNEL = 13;                          // Max 2.4GHz channel (adjust to 11 for US)

// --- Global Variables ---
int current_channel = 1;
unsigned long last_channel_switch = 0;

void setup() {
  Serial.begin(115200);
  delay(2000);  // Allow time for Serial monitor to connect
  Serial.println("\n\n========================================");
  Serial.println(" ESP32-S3 Wi-Fi Sniffer (Management Frames)");
  Serial.println("========================================\n");

  // 1. Initialize Wi-Fi in Station mode, but DO NOT connect to any AP
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // 2. Enable Promiscuous Mode
  esp_wifi_set_promiscuous(true);

  // 3. Register the callback function to handle incoming packets
  esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous_callback);

  // 4. Set the initial Wi-Fi channel
  esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);

  Serial.printf("[*] Sniffer started. Listening on Channel %d...\n", current_channel);
  Serial.println("[*] Filtering for Beacon and Probe Request frames only.\n");
}

void loop() {
  // Simple Channel Hopping Logic
  // Wi-Fi networks operate on different channels; a sniffer must hop to see everything.
  if (millis() - last_channel_switch > CHANNEL_SWITCH_INTERVAL) {
    current_channel = (current_channel % MAX_CHANNEL) + 1;
    esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);

    Serial.printf("\n--- Hopped to Channel %02d ---\n", current_channel);
    last_channel_switch = millis();
  }

  // Yield to the watchdog timer
  delay(10);
}

/**
 * @brief Callback function triggered when a Wi-Fi packet is received in promiscuous mode.
 * 
 * @param buf Pointer to the received packet buffer
 * @param type The type of packet (mgmt, data, ctrl, etc.)
 */

void wifi_promiscuous_callback(void *buf, wifi_promiscuous_pkt_type_t type) {
  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
  const uint8_t *payload = ppkt->payload;
  uint16_t len = ppkt->rx_ctrl.sig_len;

  if (len < 24) return;

  uint8_t frame_control = payload[0];
  uint8_t frame_type = (frame_control >> 2) & 0x03;
  uint8_t frame_subtype = (frame_control >> 4) & 0x0F;

  if (frame_type != 0) return;
  if (frame_subtype != 8 && frame_subtype != 4) return;

  const uint8_t *transmitter_mac = payload + 10;

  // --- Extract SSID (Information Elements) ---
  // The start of Information Elements depends on the frame subtype.
  // The MAC header is always 24 bytes for management frames.
  uint16_t ie_offset = 24;

  if (frame_subtype == 8) {
    // BEACON frames have 12 bytes of fixed parameters before the IEs:
    // Timestamp (8 bytes) + Beacon Interval (2 bytes) + Capability Info (2 bytes)
    ie_offset += 12;
  }
  // PROBE REQUEST (subtype 4) has 0 bytes of fixed parameters, so ie_offset remains 24.

  String ssid = "";
  if (len > ie_offset) {
    // Iterate through the Information Elements
    while (ie_offset + 2 <= len) {
      uint8_t ie_id = payload[ie_offset];
      uint8_t ie_len = payload[ie_offset + 1];

      // Prevent buffer over-read
      if (ie_offset + 2 + ie_len > len) break;

      // The SSID is always Information Element ID 0
      if (ie_id == 0) {
        if (ie_len > 0) {
          char ssid_buf[33] = { 0 };  // Max SSID length is 32 bytes + null terminator
          uint8_t copy_len = (ie_len > 32) ? 32 : ie_len;
          memcpy(ssid_buf, payload + ie_offset + 2, copy_len);

          // SANITIZE: Replace non-printable ASCII characters with '?'
          // This prevents Serial Monitor garbage from high-bit UTF-8 or control chars
          for (uint8_t i = 0; i < copy_len; i++) {
            if (ssid_buf[i] < 32 || ssid_buf[i] > 126) {
              ssid_buf[i] = '?';
            }
          }
          ssid = String(ssid_buf);
        } else {
          // If length is 0, it's a hidden network broadcasting an empty SSID
          ssid = "Hidden Network";
        }
        break;  // Found the SSID, stop searching
      }
      ie_offset += 2 + ie_len;
    }
  }

  // Format and print the output
  const char *frame_type_str = (frame_subtype == 8) ? "BEACON " : "PROBE  ";

  if (ssid.isEmpty()) {
    ssid = (frame_subtype == 4) ? "Broadcast (Wildcard)" : "Hidden Network";
  }

  Serial.printf("[%s] MAC: %02X:%02X:%02X:%02X:%02X:%02X | SSID: %s\n",
                frame_type_str,
                transmitter_mac[0], transmitter_mac[1], transmitter_mac[2],
                transmitter_mac[3], transmitter_mac[4], transmitter_mac[5],
                ssid.c_str());
  //if (ssid == "Nightingale_IoT")
  //  Serial.println("+++++++++++++++++++++++++++++++");
}
