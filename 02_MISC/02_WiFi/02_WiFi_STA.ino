#include <WiFi.h>

// Your home router credentials
const char* ssid = "Nightingale_IoT";
const char* password = "1122334455";

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== ESP32-S3 in STATION (STA) MODE ===");
   // Explicitly set the mode before connecting
  WiFi.mode(WIFI_STA); 
  // 1. Connect to the existing router
  WiFi.begin(ssid, password);
  
  Serial.print("Connecting to router");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected to Home Router!");
  
  // 2. Print the IP address assigned by your router
  Serial.print("ESP32 IP Address on your network: ");
  Serial.println(WiFi.localIP()); 
  Serial.println("(You can ping this IP from your PC)");
}

void loop() {
  // Just keep the connection alive and print signal strength every 5 seconds
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected! Signal Strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("Lost connection! Trying to reconnect...");
    WiFi.begin(ssid, password);
  }
  delay(5000); 
}
