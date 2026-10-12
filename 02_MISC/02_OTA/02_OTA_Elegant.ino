#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <Update.h>
#include <esp_ota_ops.h>

#if !defined(ELEGANTOTA_USE_ASYNC_WEBSERVER) || ELEGANTOTA_USE_ASYNC_WEBSERVER != 1
#error "Enable async mode in libraries/ElegantOTA/src/ElegantOTA.h"
#endif


const char* WIFI_SSID = "Nightingale_IoT";

const char* WIFI_PASSWORD = "1122334455";

const char* WEB_HOSTNAME = "esp32s3-webota";

const char* OTA_USERNAME = "admin";

const char* OTA_PASSWORD = "ChangeThis-WebOTA-Password!";

const char* FW_VERSION = "2.0.0";

AsyncWebServer server(80);

uint32_t lastProgressLog = 0;

uint32_t lastHeartbeat = 0;

void printPartitionInfo() {
  const esp_partition_t* running =
    esp_ota_get_running_partition();

  const esp_partition_t* next =
    esp_ota_get_next_update_partition(
      nullptr);

  if (running) {
    Serial.printf(
      "Running partition: %s\n",
      running->label);
  }

  if (next) {
    Serial.printf(
      "Next OTA partition: %s, "
      "capacity: %lu bytes\n",
      next->label,
      (unsigned long)next->size);
  } else {
    Serial.println(
      "ERROR: No inactive OTA application partition.");
  }

  Serial.printf(
    "Flash: %lu bytes, PSRAM: %lu bytes\n",
    (unsigned long)ESP.getFlashChipSize(),
    (unsigned long)ESP.getPsramSize());
}

void connectWiFi() {
  WiFi.mode(
    WIFI_STA);

  WiFi.setHostname(
    WEB_HOSTNAME);

  WiFi.setAutoReconnect(
    true);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD);

  Serial.print(
    "Connecting to Wi-Fi");

  const uint32_t started =
    millis();

  while (
    WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");

    if (
      millis() - started > 30000) {
      Serial.println(
        "\nWi-Fi timeout. Restarting.");

      delay(1000);
      ESP.restart();
    }
  }

  WiFi.setSleep(
    false);

  Serial.println();
}

void setup() {
  Serial.begin(
    115200);

  delay(1000);

  Serial.printf(
    "\nFirmware version: %s\n",
    FW_VERSION);

  Serial.printf(
    "Build: %s %s\n",
    __DATE__,
    __TIME__);

  printPartitionInfo();
  connectWiFi();

  if (
    MDNS.begin(
      WEB_HOSTNAME)) {
    MDNS.addService(
      "http",
      "tcp",
      80);

    Serial.printf(
      "mDNS: http://%s.local/update\n",
      WEB_HOSTNAME);
  } else {
    Serial.println(
      "mDNS failed; use the IP address.");
  }

  server.on(
    "/",
    HTTP_GET,
    [](AsyncWebServerRequest* request) {
      String page;

      page.reserve(
        350);

      page +=
        "<!doctype html><html><body>";

      page +=
        "<h2>ESP32-S3 Web OTA</h2>";

      page +=
        "<p>Firmware version: ";

      page +=
        FW_VERSION;

      page +=
        "</p><p>Build: ";

      page +=
        __DATE__;

      page +=
        " ";

      page +=
        __TIME__;

      page +=
        "</p><p><a href='/update'>"
        "Upload firmware</a></p>";

      page +=
        "</body></html>";

      AsyncWebServerResponse* response =
        request->beginResponse(
          200,
          "text/html",
          page);

      response->addHeader(
        "Cache-Control",
        "no-store");

      request->send(
        response);
    });

  ElegantOTA.begin(
    &server,
    OTA_USERNAME,
    OTA_PASSWORD);

  ElegantOTA.setAutoReboot(
    true);

  ElegantOTA.onStart([]() {
    lastProgressLog =
      0;

    Serial.println(
      "\nWeb OTA started.");

    // Put project hardware into
    // a safe state here.
    // Do not perform lengthy work
    // inside asynchronous callbacks.
  });

  ElegantOTA.onProgress(
    [](size_t current,
       size_t total) {
      const uint32_t now =
        millis();

      if (
        now - lastProgressLog >= 1000 || (total > 0 && current == total)) {
        lastProgressLog =
          now;

        if (
          total > 0) {
          const unsigned int percent =
            (unsigned int)((
                             (uint64_t)current * 100)
                           / total);

          Serial.printf(
            "Web OTA: %u%%, %lu bytes\n",
            percent,
            (unsigned long)current);
        } else {
          Serial.printf(
            "Web OTA received: %lu bytes\n",
            (unsigned long)current);
        }
      }
    });

  ElegantOTA.onEnd(
    [](bool success) {
      Serial.println(
        success
          ? "Web OTA succeeded. "
            "Restart scheduled."
          : "Web OTA failed.");

      if (
        !success && Update.hasError()) {
        Update.printError(
          Serial);
      }
    });

  server.begin();

  Serial.print(
    "Open http://");

  Serial.print(
    WiFi.localIP());

  Serial.println(
    "/update");
}

void loop() {
  // Required for ElegantOTA's scheduled
  // automatic reboot.
  ElegantOTA.loop();

  if (
    millis() - lastHeartbeat >= 10000) {
    lastHeartbeat = millis();

    Serial.printf("****ElehantOTA****\n");
    Serial.printf("Firmware %s, Wi-Fi status %d\n", FW_VERSION, (int)WiFi.status());
    Serial.printf("Temperature %.1f deg\n", (float)random(200, 400) / 10.0f);
  }


  // Add short, non-blocking application work here.
  delay(2);
}