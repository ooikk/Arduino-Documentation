#include <WiFi.h>

// Credentials for the ESP32's own hotspot
const char* ap_ssid = "ESP32-S3-Hotspot";
const char* ap_password = "12345678";

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== ESP32-S3 in ACCESS POINT (AP) MODE ===");
  
  // 1. Create the Wi-Fi Hotspot (No router needed!)
  // Explicitly set the mode before starting the AP
  WiFi.mode(WIFI_AP); 
  // Parameters: SSID, Password, Channel, Hide SSID, Max Clients
  WiFi.softAP(ap_ssid, ap_password, 1, 0, 4);
  
  // 2. Print the IP address of the hotspot (Usually 192.168.4.1)
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("Access Point Started! SSID: ");
  Serial.println(ap_ssid);
  Serial.print("Hotspot created! IP Address: ");
  Serial.println(myIP);
  Serial.println("Go to your phone's Wi-Fi settings and connect to 'ESP32-S3-Hotspot'");
}

void loop() {
  // Count how many devices are connected to our ESP32 hotspot
  int stationCount = WiFi.softAPgetStationNum();
  
  Serial.print("Devices currently connected to my Hotspot: ");
  Serial.println(stationCount);
  
  if (stationCount > 0) {
    Serial.println(" -> Someone is connected to me!");
  } else {
    Serial.println(" -> Waiting for someone to connect...");
  }
  
  delay(3000);
}