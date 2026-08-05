#include <WiFi.h>
#include <WebServer.h>

// Replace with your network credentials
const char* ssid = "Nightingale_IoT";
const char* password = "1122334455";

WebServer server(80); // Create a web server on port 80

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>ESP32-S3 STA Mode</h1>";
  html += "<p>IP Address: " + WiFi.localIP().toString() + "</p>";
  html += "<p>RSSI: " + String(WiFi.RSSI()) + " dBm</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Define routes
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP Server started");
}

void loop() {
  server.handleClient(); // Handle incoming web requests
}
