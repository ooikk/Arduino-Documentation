#include <WiFi.h>
#include <WebServer.h>

const char* ap_ssid = "ESP32-S3-AP";
const char* ap_password = "12345678";  // Must be at least 8 characters

WebServer server(80);

void handleRoot() {
  String html = "<h1>Welcome to ESP32-S3 SoftAP!</h1>";
  html += "<p>You are connected to the ESP32-S3 directly.</p>";
  html += "<p>Connected Clients: " + String(WiFi.softAPgetStationNum()) + "</p>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);

  Serial.println("Configuring Access Point...");

  // Start SoftAP
  // Parameters: SSID, Password, Channel, Hidden, Max Connections
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password, 1, 0, 4);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP SSID: ");
  Serial.println(ap_ssid);
  Serial.print("AP IP address: ");
  Serial.println(myIP);  // Usually 192.168.4.1

  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP Server started on AP");

}

void loop() {
  server.handleClient();
    // Count how many devices are connected to our ESP32 hotspot
  int stationCount = WiFi.softAPgetStationNum();
  
  Serial.print("Devices currently connected to my Hotspot: ");
  Serial.println(stationCount);
  delay(5000);
}