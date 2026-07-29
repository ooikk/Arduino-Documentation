#include <WiFi.h>
#include <WebServer.h>

// STA Credentials (Your Home Router)
const char* sta_ssid = "Nightingale_IoT";
const char* sta_password = "98297824";

// AP Credentials (ESP32's own network)
const char* ap_ssid = "ESP32-S3-Mixed";
const char* ap_password = "12345678";

WebServer server(80);

void handleRoot() {
  String html = "<h1>ESP32-S3 AP+STA Mode</h1>";
  html += "<h3>Station (STA) Info:</h3>";
  html += "<p>Connected to: " + String(WiFi.SSID()) + "</p>";
  html += "<p>Router IP: " + WiFi.localIP().toString() + "</p>";
  
  html += "<h3>Access Point (AP) Info:</h3>";
  html += "<p>AP SSID: " + String(ap_ssid) + "</p>";
  html += "<p>AP IP: " + WiFi.softAPIP().toString() + "</p>";
  html += "<p>Clients connected to AP: " + String(WiFi.softAPgetStationNum()) + "</p>";
  
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  
  // 1. Start Access Point FIRST
  WiFi.mode(WIFI_AP_STA); // Mandatory for mixed mode!
  Serial.println("Starting AP...");
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("AP SSID: ");
  Serial.println(ap_ssid);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // 2. Connect to Home Router (STA)
  Serial.println("Connecting to STA...");
  WiFi.begin(sta_ssid, sta_password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to Router!");
    Serial.print("STA IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to Router. AP is still running.");
  }

  // 3. Start Web Server
  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();
}