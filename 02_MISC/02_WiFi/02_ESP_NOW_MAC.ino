#include <WiFi.h>
// for option 2
#include <esp_wifi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_MODE_STA);
  delay(100);  // Give the Wi-Fi driver time to power on and load registers

  // Option 1: Standard Arduino approach
  Serial.print("Receiver MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Option 2: Direct ESP-IDF hardware query (bypasses standard wrapper)
  uint8_t rawMac[6];
  esp_wifi_get_mac(WIFI_IF_STA, rawMac);
  Serial.printf("Direct MAC:   %02X:%02X:%02X:%02X:%02X:%02X\n", 
                rawMac[0], rawMac[1], rawMac[2], rawMac[3], rawMac[4], rawMac[5]);

}

void loop() {
  // Nothing here
}
