#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

const char* WIFI_SSID = "Nightingale_IoT";
const char* WIFI_PASS = "1122334455";
const char* TEST_URL  = "https://example.com";

WiFiClientSecure client;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.setDebugOutput(true);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to Wi-Fi");
  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < 20000) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWi-Fi connection failed.");
    return;
  }

  Serial.printf("\nWi-Fi connected: %s\n",
                WiFi.localIP().toString().c_str());

  // LAB A ONLY: bypass server authentication.
  client.setInsecure();

  HTTPClient https;
  https.setTimeout(15000);

  Serial.printf("GET %s\n", TEST_URL);

  if (!https.begin(client, TEST_URL)) {
    Serial.println("HTTPClient begin failed.");
    return;
  }

  int code = https.GET();

  if (code > 0) {
    Serial.printf("HTTP status: %d\n", code);
    Serial.println("Request completed without CA validation.");
  } else {
    Serial.printf("Request failed: %s (%d)\n",
                  HTTPClient::errorToString(code).c_str(),
                  code);
  }

  https.end();
  client.stop();
}

void loop() {}