#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <esp_ota_ops.h>

const char* WIFI_SSID =
  "YOUR_WIFI_SSID";

const char* WIFI_PASSWORD =
  "YOUR_WIFI_PASSWORD";

const char* OTA_HOSTNAME =
  "esp32s3-ota";  // Do not add ".local"

const char* OTA_PASSWORD =
  "ChangeThis-OTA-Password!";

const char* FW_VERSION =
  "1.0.0";

uint32_t lastProgressLog =
  0;

uint32_t lastHeartbeat =
  0;

void printPartitionInfo() {
  const esp_partition_t* running =
    esp_ota_get_running_partition();

  const esp_partition_t* next =
    esp_ota_get_next_update_partition(
      nullptr
    );

  if (running) {
    Serial.printf(
      "Running partition: %s\n",
      running->label
    );
  }

  if (next) {
    Serial.printf(
      "Next OTA partition: %s, "
      "capacity: %lu bytes\n",
      next->label,
      (unsigned long)next->size
    );
  } else {
    Serial.println(
      "ERROR: No inactive OTA application partition."
    );
  }

  Serial.printf(
    "Flash: %lu bytes, PSRAM: %lu bytes\n",
    (unsigned long)ESP.getFlashChipSize(),
    (unsigned long)ESP.getPsramSize()
  );
}

void connectWiFi() {
  WiFi.mode(
    WIFI_STA
  );

  WiFi.setHostname(
    OTA_HOSTNAME
  );

  WiFi.setAutoReconnect(
    true
  );

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  Serial.print(
    "Connecting to Wi-Fi"
  );

  const uint32_t started =
    millis();

  while (
    WiFi.status() != WL_CONNECTED
  ) {
    delay(250);
    Serial.print(".");

    if (
      millis() - started > 30000
    ) {
      Serial.println(
        "\nWi-Fi timeout. Restarting."
      );

      delay(1000);
      ESP.restart();
    }
  }

  WiFi.setSleep(
    false
  );

  Serial.println();
  Serial.print(
    "IP address: "
  );
  Serial.println(
    WiFi.localIP()
  );
}

void setup() {
  Serial.begin(
    115200
  );

  delay(1000);

  // Do not use an unbounded while (!Serial).
  // OTA must work without a USB host connected.
  Serial.printf(
    "\nFirmware version: %s\n",
    FW_VERSION
  );

  Serial.printf(
    "Build: %s %s\n",
    __DATE__,
    __TIME__
  );

  printPartitionInfo();
  connectWiFi();

  ArduinoOTA.setHostname(
    OTA_HOSTNAME
  );

  ArduinoOTA.setPort(
    3232
  );

  ArduinoOTA.setPassword(
    OTA_PASSWORD
  );

  ArduinoOTA.setMdnsEnabled(
    true
  );

  ArduinoOTA.setTimeout(
    15000
  );

  ArduinoOTA.onStart([]() {
    lastProgressLog =
      0;

    Serial.println(
      "\nOTA started."
    );

    // Put project hardware into
    // a safe state here.
    // Keep this callback short.
  });

  ArduinoOTA.onProgress(
    [](unsigned int progress,
       unsigned int total) {
      const uint32_t now =
        millis();

      if (
        total > 0 &&
        (
          now - lastProgressLog >=
            1000 ||
          progress == total
        )
      ) {
        lastProgressLog =
          now;

        const unsigned int percent =
          (unsigned int)(
            (
              (uint64_t)progress *
              100
            ) /
            total
          );

        Serial.printf(
          "OTA: %u%%\n",
          percent
        );
      }
    }
  );

  ArduinoOTA.onEnd([]() {
    Serial.println(
      "OTA completed. "
      "Restarting into new firmware."
    );

    // ArduinoOTA performs the restart
    // after this callback.
  });

  ArduinoOTA.onError(
    [](ota_error_t error) {
      Serial.printf(
        "OTA error %u: ",
        (unsigned int)error
      );

      switch (error) {
        case OTA_AUTH_ERROR:
          Serial.println(
            "Authentication failed."
          );
          break;

        case OTA_BEGIN_ERROR:
          Serial.println(
            "Could not begin update."
          );
          break;

        case OTA_CONNECT_ERROR:
          Serial.println(
            "Could not connect to uploader."
          );
          break;

        case OTA_RECEIVE_ERROR:
          Serial.println(
            "Firmware reception failed."
          );
          break;

        case OTA_END_ERROR:
          Serial.println(
            "Firmware finalization failed."
          );
          break;

        default:
          Serial.println(
            "Unknown error."
          );
          break;
      }

      if (
        Update.hasError()
      ) {
        Update.printError(
          Serial
        );
      }
    }
  );

  // Starts the OTA listener and the
  // mDNS Arduino service.
  // A separate MDNS.begin() is not needed.
  ArduinoOTA.begin();

  Serial.printf(
    "ArduinoOTA ready: %s.local\n",
    OTA_HOSTNAME
  );
}

void loop() {
  // Call frequently.
  // Avoid long blocking operations.
  ArduinoOTA.handle();

  if (
    millis() - lastHeartbeat >=
    10000
  ) {
    lastHeartbeat =
      millis();

    Serial.printf(
      "Firmware %s, Wi-Fi status %d\n",
      FW_VERSION,
      (int)WiFi.status()
    );
  }

  delay(2);
}