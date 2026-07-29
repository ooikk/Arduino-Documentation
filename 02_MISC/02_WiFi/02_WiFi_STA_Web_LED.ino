#include <WiFi.h>
#include <WebServer.h>

const char* ssid     = "Nightingale_IoT";
const char* password = "98297824";

WebServer server(80);
const int LED_PIN = 2; // Built-in LED on most S3 dev boards
bool ledState = LOW;

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>ESP32-S3 Web Control</title></head>";
  html += "<body style='font-family:sans-serif; text-align:center; margin-top:50px;'>";
  html += "<h2>ESP32-S3 Wi-Fi Web Server</h2>";
  html += "<p>LED Status: <b>" + String(ledState ? "ON" : "OFF") + "</b></p>";
  html += "<a href='/toggle'><button style='padding:10px 20px; font-size:16px;'>Toggle LED</button></a>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleToggle() {
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState);
  server.sendHeader("Location", "/");
  server.send(303); // Redirect back to root
  Serial.print("LED State: ");
  Serial.println(ledState);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected!");
  Serial.print("Open in Browser: http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.begin();
}

void loop() {
  server.handleClient();
}