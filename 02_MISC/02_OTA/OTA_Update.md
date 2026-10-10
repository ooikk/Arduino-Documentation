# ESP32-S3 Over-the-Air Updating

OTA—Over-the-Air updating—lets you replace the ESP32-S3 firmware through Wi-Fi after the first USB upload.

You can leave the board installed in your project and update its program without reconnecting a USB data cable.

These instructions target:

- Arduino IDE 2.3.2.
- Arduino-ESP32 core 3.0.7.
- ESP32-S3-WROOM-1 N16R8.

Both sketches are complete and use the corresponding APIs.

> **Note:** The versioned APIs were checked, but the sketches have not been compiled or hardware-tested in this environment.

## Important Library Correction

`AsyncElegantOTA` is deprecated.

Use `ElegantOTA` with asynchronous mode enabled.

An asynchronous server is useful for larger web projects, but the ESP32-S3 also supports ordinary synchronous Web OTA. Async mode is not an ESP32-S3 requirement.

See the [AsyncElegantOTA repository](https://github.com/ayushsharma82/AsyncElegantOTA?utm_source=chatgpt.com).

## 1. OTA Methods

During a normal firmware OTA update:

1. The current firmware connects to Wi-Fi and starts an OTA service.
2. Your computer sends the new compiled application.
3. The ESP32 writes it into the inactive application partition in flash.
4. After successful validation, it selects that partition for the next boot.
5. The ESP32 restarts and runs the new firmware.

This requires:

- Two application slots: `ota_0` and `ota_1`.
- An `otadata` partition recording which application should boot.

The running application is not overwritten during the transfer.

See the [ESP-IDF OTA documentation](https://docs.espressif.com/projects/esp-idf/en/v5.1.4/esp32s3/api-reference/system/ota.html?utm_source=chatgpt.com).

| Item | ArduinoOTA | Browser OTA Using ElegantOTA |
|---|---|---|
| Upload interface | Arduino IDE network port. | Browser page at `/update`. |
| What you upload | Click **Upload**; the IDE compiles and transfers the firmware. | Export the compiled application `.bin`, then select it in the browser. |
| Discovery | mDNS advertises an Arduino OTA service. | Enter the IP address; `.local` hostname is optional. |
| Authentication | OTA password. | Web username and password. |
| Transfer | UDP invitation/authentication, followed by TCP firmware transfer. | HTTP upload. |
| Extra libraries | None beyond the ESP32 core. | ElegantOTA; async mode also requires AsyncTCP and ESPAsyncWebServer. |
| Best fit | Frequent development from Arduino IDE. | Updating an installed device from a browser. |

`ArduinoOTA` provides a network upload endpoint. It is not a wireless replacement for the USB Serial Monitor.

See the [ArduinoOTA library](https://github.com/espressif/arduino-esp32/blob/master/libraries/ArduinoOTA/library.properties?utm_source=chatgpt.com).

These examples update application firmware.

They do not:

- Update files on the SD card.
- Replace the partition table.
- Replace the bootloader.

Keep OTA support in every new firmware image. If you upload a sketch without the OTA service, subsequent updates will require USB again.

## 2. Install Board Package and Libraries

In Arduino IDE 2.3.2:

1. Open **File → Preferences**.
2. Add this URL to **Additional Boards Manager URLs**:

   [https://espressif.github.io/arduino-esp32/package_esp32_index.json](https://espressif.github.io/arduino-esp32/package_esp32_index.json)

3. Open **Boards Manager**.
4. Search for **esp32 by Espressif Systems**.
5. Select version **3.0.7**.
6. Install it.
7. Select:

   ```text
   Tools → Board → esp32 → ESP32S3 Dev Module
   ```

See the [Arduino-ESP32 installation documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html?utm_source=chatgpt.com).

### Library Setup

| Library or Header | Installation |
|---|---|
| `WiFi.h` | Included with the ESP32 board package. |
| `ESPmDNS.h` | Included with the ESP32 board package. |
| `ArduinoOTA.h` | Included with the ESP32 board package. |
| `Update.h` and `esp_ota_ops.h` | Included with the ESP32 board package. |
| ElegantOTA 3.1.7 by Ayush Sharma | Install using Library Manager for Example B. |
| Async TCP 3.3.7 by ESP32Async | Install using Library Manager for Example B. |
| ESP Async WebServer 3.7.3 by ESP32Async | Install using Library Manager for Example B. |

You do not need to install a separate `ArduinoOTA` library from Library Manager. Use the implementation supplied by Espressif.

The async dependency versions above are the versions recommended in ElegantOTA's documentation.

See the [Arduino-ESP32 libraries README](https://github.com/espressif/arduino-esp32/blob/master/libraries/README.md?utm_source=chatgpt.com).

### Enable ElegantOTA Async Mode

For ElegantOTA, enable async mode before compiling Example B:

1. Find the Sketchbook location in Arduino Preferences.
2. Open this file below that location:

   ```text
   libraries/ElegantOTA/src/ElegantOTA.h
   ```

3. Change the default macro from `0` to `1`:

   ```cpp
   #ifndef ELEGANTOTA_USE_ASYNC_WEBSERVER
     #define ELEGANTOTA_USE_ASYNC_WEBSERVER 1
   #endif
   ```

4. Save the file.

Defining this macro only in the `.ino` file is insufficient because ElegantOTA's separate `.cpp` file must also compile in async mode.

Updating or reinstalling ElegantOTA may overwrite this change.

See the [ElegantOTA async-mode documentation](https://docs.elegantota.pro/getting-started/async-mode?utm_source=chatgpt.com).

Avoid duplicate installations of old and new asynchronous libraries, especially archived `me-no-dev` versions.

## 3. Select ESP32-S3 Board Settings

For an ESP32-S3-WROOM-1 N16R8 with 16 MB flash and 8 MB PSRAM, using its native USB connector:

| Tools Setting | Selection | Reason |
|---|---|---|
| Board | `ESP32S3 Dev Module` | Correct chip family. |
| USB CDC On Boot | Enabled | Sends Serial output through native USB. |
| USB DFU On Boot | Disabled | DFU is USB firmware updating, not Wi-Fi OTA. |
| USB Firmware MSC On Boot | Disabled, if shown. | Not needed for these examples. |
| USB Mode | Hardware CDC and JTAG. | Straightforward native USB serial configuration. |
| Upload Mode | UART0 / Hardware CDC. | Matches the selected USB mode. |
| Flash Mode | QIO 80 MHz. | WROOM-1 N16R8 uses quad flash. |
| Flash Size | 16 MB (128 Mb). | Matches N16R8 hardware. |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS). | Contains two 3 MB OTA application slots. |
| PSRAM | OPI PSRAM. | N16R8 uses octal PSRAM. |
| CPU Frequency | 240 MHz, default. | Leave unchanged initially. |
| Erase All Flash Before Sketch Upload | Disabled. | Normal setting. |
| Upload Speed | 460800 initially. | Lower it if USB or UART upload fails. |

Flash mode and PSRAM mode describe different memory interfaces.

N16R8's OPI PSRAM does not mean that its flash should be configured as OPI. Espressif lists WROOM-1 N16R8 as QSPI flash with OPI PSRAM.

See the [Arduino-ESP32 troubleshooting documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/troubleshooting.html?utm_source=chatgpt.com).

The selected [core 3.0.7 partition table](https://github.com/espressif/arduino-esp32/blob/3.0.7/tools/partitions/app3M_fat9M_16MB.csv) allocates 3,145,728 bytes per application slot.

Your firmware must fit in one slot. It cannot use the entire 16 MB.

For other ESP32-S3 modules, match Flash Size, Flash Mode, and PSRAM settings to the actual hardware.

OTA itself does not require PSRAM.

For smaller flash devices, suitable OTA schemes include:

- Default 4MB with SPIFFS.
- Minimal SPIFFS with a 1.9 MB application slot, OTA, and 190 KB SPIFFS.

Do not select:

- `Huge APP ... No OTA`.
- Any partition scheme containing `No OTA`.

Changing the Partition Scheme in the IDE does not change the device's existing partition table through ordinary OTA. Apply a new layout through USB first.

See the [Arduino-ESP32 partition table documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/partition_table.html?utm_source=chatgpt.com).

If you use a board's USB-to-UART connector, CDC On Boot can be disabled so `Serial` goes to UART0.

With CDC enabled, use `Serial0` when you specifically need UART0 output.

See the [Arduino-ESP32 Tools menu documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/guides/tools_menu.html?utm_source=chatgpt.com).

## 4. Example A: ArduinoOTA with Password and mDNS

Create a sketch named:

```text
S3_ArduinoOTA
```

Replace the Wi-Fi credentials and OTA password.

```cpp
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
```

`ArduinoOTA.begin()` starts mDNS using the configured hostname and advertises the Arduino upload service.

Calling `MDNS.begin()` again is unnecessary.

See the [ArduinoOTA implementation](https://github.com/espressif/arduino-esp32/blob/master/libraries/ArduinoOTA/src/ArduinoOTA.cpp?utm_source=chatgpt.com).

### Upload Workflow for Example A

1. Connect the ESP32-S3 through USB.
2. Select its COM port.
3. Apply the board settings above.
4. Click **Upload**.
5. Open the Serial Monitor at 115200 baud.
6. Reset the board if necessary.
7. Confirm that it prints an IP address and:

   ```text
   ArduinoOTA ready
   ```

8. Put the computer and ESP32 on the same local network.
9. Wait briefly.
10. Open **Tools → Port**.
11. Look for a network entry resembling:

    ```text
    esp32s3-ota at 192.168.1.123
    ```

12. Select that network port.
13. If the IDE asks which board it belongs to, choose:

    ```text
    ESP32S3 Dev Module
    ```

14. Check the Tools settings again.
15. Change:

    ```cpp
    FW_VERSION = "1.0.1";
    ```

16. Click **Upload**.
17. Enter the OTA password when prompted.
18. After the restart, verify that the board reports version `1.0.1`.

USB can remain connected for power and diagnostics.

To read serial logs after uploading, select the USB COM port again.

## 5. Example B: ElegantOTA Web Upload

Install the three external libraries and enable async mode as described earlier.

Create a separate sketch named:

```text
S3_WebOTA
```

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <Update.h>
#include <esp_ota_ops.h>

#if !defined(
  ELEGANTOTA_USE_ASYNC_WEBSERVER
) || \
    ELEGANTOTA_USE_ASYNC_WEBSERVER != 1
#error "Enable async mode in libraries/ElegantOTA/src/ElegantOTA.h"
#endif

const char* WIFI_SSID =
  "YOUR_WIFI_SSID";

const char* WIFI_PASSWORD =
  "YOUR_WIFI_PASSWORD";

const char* WEB_HOSTNAME =
  "esp32s3-webota";

const char* OTA_USERNAME =
  "admin";

const char* OTA_PASSWORD =
  "ChangeThis-WebOTA-Password!";

const char* FW_VERSION =
  "1.0.0";

AsyncWebServer server(
  80
);

uint32_t lastProgressLog =
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
    WEB_HOSTNAME
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
}

void setup() {
  Serial.begin(
    115200
  );

  delay(1000);

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

  if (
    MDNS.begin(
      WEB_HOSTNAME
    )
  ) {
    MDNS.addService(
      "http",
      "tcp",
      80
    );

    Serial.printf(
      "mDNS: http://%s.local/update\n",
      WEB_HOSTNAME
    );
  } else {
    Serial.println(
      "mDNS failed; use the IP address."
    );
  }

  server.on(
    "/",
    HTTP_GET,
    [](AsyncWebServerRequest* request) {
      String page;

      page.reserve(
        350
      );

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
          page
        );

      response->addHeader(
        "Cache-Control",
        "no-store"
      );

      request->send(
        response
      );
    }
  );

  ElegantOTA.begin(
    &server,
    OTA_USERNAME,
    OTA_PASSWORD
  );

  ElegantOTA.setAutoReboot(
    true
  );

  ElegantOTA.onStart([]() {
    lastProgressLog =
      0;

    Serial.println(
      "\nWeb OTA started."
    );

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
        now - lastProgressLog >=
          1000 ||
        (
          total > 0 &&
          current == total
        )
      ) {
        lastProgressLog =
          now;

        if (
          total > 0
        ) {
          const unsigned int percent =
            (unsigned int)(
              (
                (uint64_t)current *
                100
              ) /
              total
            );

          Serial.printf(
            "Web OTA: %u%%, %lu bytes\n",
            percent,
            (unsigned long)current
          );
        } else {
          Serial.printf(
            "Web OTA received: %lu bytes\n",
            (unsigned long)current
          );
        }
      }
    }
  );

  ElegantOTA.onEnd(
    [](bool success) {
      Serial.println(
        success
          ? "Web OTA succeeded. "
            "Restart scheduled."
          : "Web OTA failed."
      );

      if (
        !success &&
        Update.hasError()
      ) {
        Update.printError(
          Serial
        );
      }
    }
  );

  server.begin();

  Serial.print(
    "Open http://"
  );

  Serial.print(
    WiFi.localIP()
  );

  Serial.println(
    "/update"
  );
}

void loop() {
  // Required for ElegantOTA's scheduled
  // automatic reboot.
  ElegantOTA.loop();

  // Add short, non-blocking application work here.
  delay(2);
}
```

The library supplies the `/update` page and upload handlers.

Its callbacks report transfer status, and `ElegantOTA.loop()` handles the scheduled restart.

Asynchronous callbacks should remain short.

See the [ElegantOTA asynchronous example](https://github.com/ayushsharma82/ElegantOTA/blob/master/examples/AsyncDemo/AsyncDemo.ino?utm_source=chatgpt.com).

### Upload Workflow for Example B

1. Select the USB COM port.
2. Upload this sketch through USB.
3. Open the Serial Monitor.
4. Note the printed IP address.
5. From a computer on the same LAN, open:

   ```text
   http://192.168.1.123/update
   ```

   Substitute the actual address.

6. You can also try:

   ```text
   http://esp32s3-webota.local/update
   ```

7. Log in using the configured username and password.
8. Change:

   ```cpp
   FW_VERSION = "1.0.1";
   ```

9. Keep the same board, flash, PSRAM, and partition settings.
10. Select **Sketch → Export Compiled Binary**.
11. Use **Sketch → Show Sketch Folder**.
12. Locate the exported application binary, usually within its build subfolder:

    ```text
    S3_WebOTA.ino.bin
    ```

13. On the `/update` page, select **Firmware**.
14. Choose the `.bin` file.
15. Start the update.
16. Wait for completion and restart.
17. Open the device's root page to confirm version `1.0.1`.

Select the application `.ino.bin`.

Do not upload these as a firmware OTA image:

```text
.bootloader.bin
.partitions.bin
.merged.bin
```

ElegantOTA's file mode expects the compiled application binary.

See the [ElegantOTA file-mode documentation](https://docs.elegantota.pro/ota-modes/file?utm_source=chatgpt.com).

Example B advertises an HTTP service. It does not advertise an ArduinoOTA network port because this sketch does not start `ArduinoOTA`.

## 6. Verify the Update and Diagnose Problems

A successful upload means both:

1. The transfer completed.
2. The new firmware booted.

Verify:

- Firmware version.
- Build information.
- Wi-Fi connection.
- Availability of the OTA service after reboot.

With the illustrated two-slot layout, the running partition should alternate between `app0` and `app1`.

| Symptom | Likely Cause or Check | Action |
|---|---|---|
| Arduino network port absent. | Example B is running. | Use its browser page; a network upload port is expected only for Example A. |
| Arduino network port absent. | No Wi-Fi connection or OTA initialisation. | Check USB logs for the IP address and `ArduinoOTA ready`. |
| Device reachable by IP but no network port or `.local` name. | mDNS discovery blocked. | Check multicast, firewall, VPN, and guest-network settings. Restart the IDE. |
| Neither OTA method can reach the device. | Different subnet or Wi-Fi client isolation. | Put both devices on the same reachable LAN and disable guest/AP isolation. |
| Authentication fails. | Wrong password or cached IDE password. | Use the password in the currently running firmware. |
| No inactive OTA application partition. | Existing layout lacks a second application slot. | Select an OTA partition scheme and upload through USB. |
| Firmware too large or insufficient space. | Image exceeds the inactive slot. | Reduce firmware size or install a larger two-slot layout through USB. |
| `AsyncWebServer*` / `WebServer*` mismatch or undefined reference. | ElegantOTA was compiled in synchronous mode. | Edit its library header, then rebuild. |
| Upload succeeds but the board boot-loops. | Wrong hardware settings or application crash. | Read USB boot logs and check flash mode, PSRAM, and firmware logic. |
| USB logs disappear. | CDC disabled or wrong USB connector. | Match CDC settings and `Serial`/`Serial0` to the connector. |
| Works with USB attached but fails on external power. | `while (!Serial)` is waiting for a host or power is inadequate. | Remove the unbounded wait and check the power supply. |

## ArduinoOTA Firewall Details

Three communication paths matter:

| Purpose | Traffic |
|---|---|
| mDNS discovery | UDP 5353 multicast. |
| OTA invitation and authentication | UDP to ESP32 port 3232. |
| Firmware transfer | ESP32 connects back to a TCP listener on the computer. |

Therefore, permitting only port `3232` does not solve every ArduinoOTA firewall problem.

Allow the Arduino IDE and its uploader or Python process on your trusted private network.

VPNs and multiple network adapters can also interfere with the ESP32's connection back to the computer.

See the [Arduino ESPOTA uploader](https://github.com/espressif/arduino-esp32/blob/master/tools/espota.py?utm_source=chatgpt.com).

For browser OTA, the computer connects to the ESP32's HTTP server on TCP port `80`.

Using the IP address bypasses mDNS.

## Upload Directly to an IP Address

If ArduinoOTA discovery remains unavailable, the bundled uploader can target an IP address directly.

For example, from Windows Command Prompt with Python installed:

```cmd
py "%LOCALAPPDATA%\Arduino15\packages\esp32\hardware\esp32\3.0.7\tools\espota.py" ^
  -i 192.168.1.123 ^
  -p 3232 ^
  -a "YOUR_OTA_PASSWORD" ^
  -f "C:\path\S3_ArduinoOTA.ino.bin"
```

Use the `espota.py` supplied with your Arduino-ESP32 3.0.7 package.

## When an Upload Fails Around 90%

The percentage alone does not identify the fault.

Diagnose it in this order:

1. Read the actual OTA error and USB logs.
2. `OTA_RECEIVE_ERROR` points toward transfer interruption.
3. `OTA_END_ERROR` points toward finalisation or image validation.
4. Compare the binary size with the printed inactive-slot capacity.
5. Remember that the partition installed on the device determines the limit.
6. Check power and Wi-Fi stability.
7. Check for resets, brownouts, broken TCP connections, watchdog resets, long interrupt handlers, concurrent flash-writing operations, and excessive progress logging.
8. Confirm the correct application binary and ESP32-S3 build settings.
9. Check whether the new firmware actually booted.

A lost final response can look like an upload failure. Only the firmware version and build output after reboot establish whether the update took effect.

For Telegram, HTTPS-download, and display projects, long blocking calls can delay `ArduinoOTA.handle()`, especially before an update starts.

Give OTA frequent service time.

Async Web OTA reduces dependence on the main loop for HTTP handling, but flash writes and reboot still interrupt normal operation.

## Security and Reliability Notes

The configured passwords provide access control, but these examples do not:

- Encrypt firmware transport independently of the network connection.
- Implement signed firmware verification.
- Automatically provide crash rollback.

Use OTA on a trusted LAN.

Two OTA slots do not automatically provide crash rollback. Rollback requires explicit bootloader configuration and application validation.

See the [ESP-IDF OTA documentation](https://docs.espressif.com/projects/esp-idf/en/v5.1.4/esp32s3/api-reference/system/ota.html?utm_source=chatgpt.com).

---
# ESP32-S3 OTA: Sync vs Async

For your ESP32-S3 using Arduino IDE 2.x, OTA (Over-The-Air) means updating firmware over Wi-Fi instead of connecting the board to your computer with a USB cable.

The terms synchronous (sync) and asynchronous (async) describe how the program handles network requests while the OTA process is running. They are not two completely different OTA transport methods.

## 1. Synchronous vs asynchronous OTA

### Synchronous OTA (Sync)

<img width="800" height="800" alt="image" src="https://github.com/user-attachments/assets/7f87f3a5-c396-4f4c-85a7-9116c7273876" />

- The program handles a request and waits for the operation to complete before proceeding to the next step.
- Simpler programming model.
- A long operation may block other application tasks.
- Often easier for beginners to understand.

### Asynchronous OTA (Async)

<img width="1200" height="900" alt="image" src="https://github.com/user-attachments/assets/a907221f-df84-4c0f-b15d-9171e5d6972f" />


- Network operations can be handled through callbacks or event-driven processing, without waiting for each client request to finish before servicing other requests.
- Better responsiveness when multiple clients connect.
- Useful for web dashboards and devices that must remain responsive.
- More complex programming and debugging.

### Simple analogy

Imagine your ESP32-S3 is a restaurant.

- **Sync:** One waiter serves a customer and waits until the entire order is finished before serving the next customer.
- **Async:** The waiter takes an order, lets the kitchen prepare it, and serves other customers while waiting.

The key difference is how the program handles waiting and concurrent requests—not necessarily how the firmware is transferred into flash memory.

## 2. The main OTA methods for ESP32-S3

There are several ways to update firmware. The distinction here is how the update is delivered, rather than whether the code is synchronous or asynchronous.

### 1. ArduinoOTA — upload from Arduino IDE

**Best for development**

<img width="708" height="443" alt="image" src="https://github.com/user-attachments/assets/01f47754-d17a-47b0-88c3-2b68b54ab41d" />



Select the ESP32's network port in Arduino IDE and upload as usual. It uses the ArduinoOTA service, with `ArduinoOTA.begin()` and regular calls to `ArduinoOTA.handle()`.

- No browser upload page is required.
- Source note: Github
  + [BasicOTA.ino](https://github.com/espressif/arduino-esp32/blob/master/libraries/ArduinoOTA/examples/BasicOTA/BasicOTA.ino)
  + [ArduinoOTA.h](https://github.com/espressif/arduino-esp32/blob/master/libraries/ArduinoOTA/src/ArduinoOTA.h)

### 2. Web OTA — upload in a browser

**Best for IoT projects**

<img width="665" height="496" alt="image" src="https://github.com/user-attachments/assets/a8698def-6b1b-46df-8378-5a07462c08f3" />


Open the ESP32's IP address in a browser, select a compiled firmware `.bin` file, and upload it. ElegantOTA can use either the built-in synchronous WebServer or an asynchronous web server.

- Useful when a device is installed remotely on your local network.
- Source note: ElegantOTA Docs
  + [Async Mode](https://docs.elegantota.pro/getting-started/async-mode)
  + [ElegantOTA](https://registry.platformio.org/libraries/ayushsharma82/ElegantOTA)

### 3. HTTPS or direct-download OTA

<img width="600" height="414" alt="image" src="https://github.com/user-attachments/assets/7f21fac9-1996-43e9-9a2a-cbd17502ecc6" />


The ESP32 downloads firmware from a server rather than receiving a file uploaded directly from your computer. HTTPS can protect the connection when correctly configured with server certificate verification.

- Useful for deployed devices and fleet updates.

### 4. USB serial flashing — not OTA

<img width="680" height="587" alt="image" src="https://github.com/user-attachments/assets/26aff480-c0a2-420a-a8d6-030537f83400" />


Upload firmware over USB using the serial bootloader. This is a recovery method if Wi-Fi or the OTA firmware stops working.

## 3. Sync vs async specifically in ElegantOTA

This is where the distinction matters most for your Arduino IDE project.

| Feature | ElegantOTA Sync | ElegantOTA Async |
|---|---|---|
| Web server | Built-in `WebServer` | `ESPAsyncWebServer` |
| Main request handling | Polls requests via `handleClient()` | Event-driven callbacks |
| Multiple simultaneous clients | More limited | Better support |
| Main-loop blocking | More likely | Less likely for HTTP handling |
| Dependencies | Simpler | Additional async networking libraries |
| Setup complexity | Lower | Higher |

ElegantOTA's async mode must be enabled and requires the appropriate asynchronous server library. You cannot use the synchronous `WebServer` and `ESPAsyncWebServer` as interchangeable server implementations in the same configuration.

- Source note: ElegantOTA Docs
  + [Async Mode](https://docs.elegantota.pro/getting-started/async-mode)
  + [ElegantOTA Example](https://docs.elegantota.pro/getting-started/examples)


One important nuance: async does not mean the ESP32 can continue every application task uninterrupted during an update. Flash writing, memory use, callbacks, and rebooting can still affect other tasks. Design the application to handle these conditions safely.

## 4. What about AsyncElegantOTA vs ElegantOTA?

These names are easy to confuse.

- `AsyncElegantOTA` was an older library that provided browser-based OTA using asynchronous networking.
- `ElegantOTA` is the newer library name commonly used in current tutorials. It supports both synchronous and asynchronous web-server configurations. Its async mode uses `ESPAsyncWebServer`.
- Source note: ElegantOTA Docs
  + [Async Mode](https://docs.elegantota.pro/getting-started/async-mode)
  + [ElegantOTA - Github](https://github.com/mathieucarbou/ayushsharma82-ElegantOTA)

For a new ESP32-S3 project, I would start with ElegantOTA in synchronous mode unless your project already uses an asynchronous web server or needs its concurrency benefits.

## 5. Other important OTA distinctions

| Term | What it actually means |
|---|---|
| OTA partition scheme | Flash layout that reserves space for firmware updates, commonly using two application slots |
| Rollback OTA | Can revert to a previous firmware image if the new firmware fails validation, when properly configured |
| Secure OTA | Uses appropriate authentication, integrity verification and, where required, signed firmware |
| Wi-Fi vs Ethernet OTA | Different network connections used to deliver the update |
| BLE-based update | Firmware transfer over Bluetooth Low Energy, requiring a suitable update implementation |
| Factory/USB flashing | Firmware installation through a physical connection, rather than OTA |

For your ESP32-S3 DevKitC-1 with 16 MB flash and 8 MB PSRAM, remember that PSRAM capacity does not determine how much space is available for OTA firmware. Your selected flash partition scheme determines the available application slots and their maximum firmware sizes.

## 6. My recommendation for your ESP32-S3

### Learning and developing in Arduino IDE 2.x

Start with ArduinoOTA. It is convenient for repeatedly compiling and uploading sketches over Wi-Fi.

### Building an IoT device with a browser dashboard

Use ElegantOTA Sync first. It is easier to integrate and debug alongside your existing sensor, display, and control code.

### Building a responsive web application

Consider ElegantOTA Async if you already use `ESPAsyncWebServer` or need to serve several concurrent clients.

### Deploying devices to customers

Plan for authenticated updates, firmware integrity, a suitable OTA partition scheme, rollback or recovery, and secure delivery.

**Key takeaway:** ArduinoOTA vs Web OTA describes how you initiate and deliver the update. Sync vs async describes how the web server handles requests. These are separate design choices, and both OTA methods ultimately rely on suitable firmware-update and flash-partition support.

---

# Example A (Arduino OTA) Explain how it works

## 1. Compilation Timestamp

In your ESP32 Arduino code:

```cpp
Serial.printf("Build: %s %s\n", __DATE__, __TIME__);
```

`__DATE__` and `__TIME__` are predefined C/C++ compiler macros. They record the date and time when the source code was compiled.

| Macro | Meaning | Example |
|---|---|---|
| `__DATE__` | Compilation date | `"Oct 10 2026"` |
| `__TIME__` | Compilation time | `"11:35:42"` |
| `\n` | Newline character | Moves to the next line |

Example Serial Monitor output:

```text
Build: Oct 10 2026 11:35:42
```

The example date and time above are illustrative, not your actual compilation timestamp.



## 2. Understanding the two functions: `esp_ota_get_running_partition()` and `esp_ota_get_next_update_partition(nullptr)`

These two functions are commonly used in ESP32-S3 OTA (Over-The-Air) firmware updates. They allow your program to identify which flash partition is currently running the firmware and which partition should receive the next firmware update.

Both functions return a pointer to an `esp_partition_t` structure defined by ESP-IDF, which is also available when using the ESP32 Arduino framework.

### Function 1: `esp_ota_get_running_partition()`

```cpp
const esp_partition_t* running =
    esp_ota_get_running_partition();
```

**Purpose:** Get information about the flash partition from which the currently running application was loaded.

For example, your ESP32-S3 might have two application partitions:

- `ota_0` — currently running firmware.
- `ota_1` — available for the next firmware update.

The function returns a pointer to the partition structure describing `ota_0` in this example.

### Example ESP32-S3 flash layout

- `ota_0` — Running application
  - Address: `0x10000` (example).
  - Contains the firmware currently executing.
- `ota_1` — Update target
  - Contains the other application slot, ready to receive new firmware.

*Illustrative layout only. Actual addresses and sizes depend on your partition table.*

The returned pointer does not contain the firmware itself. It points to a metadata structure describing the partition.

### Function 2: `esp_ota_get_next_update_partition(nullptr)`

```cpp
const esp_partition_t* next =
    esp_ota_get_next_update_partition(nullptr);
```

**Purpose:** Find the application partition that the OTA system recommends for the next firmware update.

The argument `nullptr` means that you are not specifying a particular partition as the reference. The OTA function uses the currently running partition to determine the next update partition.

For a typical two-slot OTA arrangement:

| Current running partition | Next update partition |
|---|---|
| `ota_0` | `ota_1` |
| `ota_1` | `ota_0` |

This is the usual A/B firmware update strategy: run firmware from one slot while writing the new firmware to the other slot.

The function returns `nullptr` if it cannot find a suitable update partition.

## 3. The `esp_partition_t` data structure

Both `running` and `next` have the same C++ type:

```cpp
const esp_partition_t*
```

Break this down:

- `esp_partition_t` — a structure containing information about a flash partition.
- `*` — a pointer to that structure.
- `const` — your code must not modify the structure through this pointer.

Conceptually, the structure looks like this:

```cpp
typedef struct {
    esp_partition_type_t type;
    esp_partition_subtype_t subtype;
    uint32_t address;
    uint32_t size;
    bool encrypted;
    char label;
} esp_partition_t;
```

This is an illustrative representation of the commonly used ESP-IDF structure; use the definition in your installed ESP-IDF headers as the authoritative version.

### What each field means

| Field | Meaning | Example |
|---|---|---|
| `type` | Partition category | `ESP_PARTITION_TYPE_APP` |
| `subtype` | Specific partition purpose | `ESP_PARTITION_SUBTYPE_APP_OTA_0` |
| `address` | Starting address in flash | `0x10000` |
| `size` | Partition capacity in bytes | `0x1E0000` |
| `encrypted` | Whether flash encryption applies | `true` or `false` |
| `label` | Human-readable partition name | `"ota_0"` |

The address and size examples are illustrative, not guaranteed values for your ESP32-S3 N16R8.

## 4. How to access the structure's fields

Because `running` and `next` are pointers, use the `->` operator to access their members.

```cpp
if (running != nullptr) {
    Serial.printf("Running partition: %s\n", running->label);
    Serial.printf("Address: 0x%08X\n",
                  (unsigned int)running->address);
    Serial.printf("Size: %u bytes\n",
                  (unsigned int)running->size);
}

if (next != nullptr) {
    Serial.printf("Next OTA partition: %s\n", next->label);
    Serial.printf("Address: 0x%08X\n",
                  (unsigned int)next->address);
    Serial.printf("Size: %u bytes\n",
                  (unsigned int)next->size);
}
```

Possible output:

```text
Running partition: ota_0
Address: 0x00010000
Size: 1966080 bytes

Next OTA partition: ota_1
Address: 0x001F0000
Size: 1966080 bytes
```

These values are only an example; your actual partition table may differ.

**Important:** Always check for `nullptr` before accessing a returned pointer. Otherwise, accessing `running->label` or `next->label` could cause a crash if the pointer is null.

## 5. How the two functions work together during OTA

1. Identify the running firmware:

   ```cpp
   esp_ota_get_running_partition()
   ```

2. Find the update target:

   ```cpp
   esp_ota_get_next_update_partition(nullptr)
   ```

3. Write the new firmware, typically using `esp_ota_begin()`, `esp_ota_write()`, and `esp_ota_end()`.

4. Select the new firmware and reboot using `esp_ota_set_boot_partition(next)`, followed by a reboot, after successful validation.

The two functions only identify partitions. They do not write firmware, change the boot partition, or reboot the ESP32 by themselves.

## 6. Why use `const esp_partition_t*` instead of `esp_partition_t`?

Consider these two declarations:

```cpp
const esp_partition_t* running;
```

This is a pointer to a read-only structure. You can read its fields, but you cannot modify them through `running`.

```cpp
esp_partition_t running;
```

This declares a local structure variable. It is a different object, not automatically populated with the running partition's information.

For these ESP-IDF APIs, use the pointer returned by the function directly. You generally do not need to copy the structure.

One final distinction: the partition table describes the available flash partitions, while the OTA data partition records which OTA application slot should be booted. The functions above help you inspect the former and identify the next update target; they do not themselves change the latter.


## 7. How ArduinoOTA Works

The key point is that `ArduinoOTA.begin()` prepares the ESP32-S3 to receive an OTA upload, while `ArduinoOTA.handle()` checks for and processes an incoming OTA request. The actual firmware update happens only when an OTA uploader, such as Arduino IDE, connects and sends a new firmware image.

Your `setup()` does not perform the firmware update by itself.

### 1. The overall OTA sequence

1. **ESP32 executes `setup()`**
   - Print partition information and connect to Wi-Fi.

2. **Configure OTA callbacks**
   - Set the hostname, port, password, timeout, progress handler, and error handler.

3. **Call `ArduinoOTA.begin()`**
   - Initialize the OTA service and make it ready to accept uploads.

4. **Call `ArduinoOTA.handle()` repeatedly from `loop()`**
   - The ESP32 checks for incoming OTA activity and processes requests.

5. **Arduino IDE sends the firmware**
   - Authentication, data transfer, progress reporting, and finalization take place.

6. **OTA finishes and the ESP32 restarts**
   - The bootloader selects the updated application partition on reboot.

The sequence is important: the OTA listener is initialized in `setup()`, but the uploader—not `setup()` or `loop()` alone—initiates the upload.

### 2. Explanation of each section in your `setup()`

#### A. Configure OTA identity and security

```cpp
ArduinoOTA.setHostname(OTA_HOSTNAME);
ArduinoOTA.setPort(3232);
ArduinoOTA.setPassword(OTA_PASSWORD);
ArduinoOTA.setMdnsEnabled(true);
ArduinoOTA.setTimeout(15000);
```

| Function | Purpose |
|---|---|
| `setHostname()` | Sets the device's network name, for example, `ESP32S3-OTA`. |
| `setPort(3232)` | Configures the OTA service's TCP port. |
| `setPassword()` | Sets the password required for OTA authentication. |
| `setMdnsEnabled(true)` | Enables mDNS service advertising so the device can be discovered by name on a compatible local network. |
| `setTimeout(15000)` | Sets an OTA timeout of 15,000 ms (15 seconds) for supported OTA operations. |

If the hostname is `ESP32S3-OTA`, Arduino IDE may show a network port such as `ESP32S3-OTA at ...`, and the device may be reachable as `ESP32S3-OTA.local` when mDNS resolution works.

The `.local` name is not guaranteed to resolve on every network. Network isolation, multicast filtering, and computer configuration can interfere.

#### B. `onStart()` — called when an OTA update begins

```cpp
ArduinoOTA.onStart([]() {
  lastProgressLog = 0;
  Serial.println("\nOTA started.");
});
```

This registers a callback. It does not execute the callback immediately.

When the OTA process starts, ArduinoOTA invokes it. This is a good place to:

- Stop motors or other moving hardware.
- Put outputs into a safe state.
- Pause operations that could interfere with firmware updating.
- Reset progress-reporting variables.

The callback should remain short. Avoid lengthy delays or blocking operations.

#### C. `onProgress()` — called during firmware transfer

```cpp
ArduinoOTA.onProgress(
  [](unsigned int progress, unsigned int total) {
    // Calculate percentage and print progress.
  }
);
```

`progress` is the amount of firmware data received so far, and `total` is the expected total amount.

The calculation is:

```text
(progress * 100) / total
```

Your actual code uses a `uint64_t` intermediate, which helps avoid overflow during multiplication.

For example, if `progress` is 500,000 bytes and `total` is 1,000,000 bytes:

\[
\frac{500000 \times 100}{1000000}=50\%
\]

Your `millis()` condition limits routine progress messages to approximately one per second, with an additional message when the transfer reaches 100%.

This callback reports transfer progress. It does not itself write or activate the firmware; ArduinoOTA manages those operations.

#### D. `onEnd()` — called when the update completes

```cpp
ArduinoOTA.onEnd([]() {
  Serial.println(
    "OTA completed. Restarting into new firmware.");
});
```

This callback runs when ArduinoOTA has successfully finalized the upload.

The important distinction is that your callback only prints a message. It does not call `ESP.restart()` or select the new partition itself.

In the standard ESP32 ArduinoOTA implementation, the library handles update finalization and then initiates a restart after the end callback. Therefore, you normally do not need to add another `ESP.restart()` inside this callback.

#### E. `onError()` — called when the update fails

Your code registers an error handler:

```cpp
ArduinoOTA.onError([](ota_error_t error) {
  Serial.printf("OTA error %u: ",
                (unsigned int)error);

  // Identify the error...
});
```

This lets you diagnose common failures:

| Error | Meaning |
|---|---|
| `OTA_AUTH_ERROR` | Password authentication failed. |
| `OTA_BEGIN_ERROR` | The update could not be started. |
| `OTA_CONNECT_ERROR` | The OTA connection could not be established. |
| `OTA_RECEIVE_ERROR` | Firmware data reception failed. |
| `OTA_END_ERROR` | Firmware finalization failed. |

Your `Update.hasError()` and `Update.printError(Serial)` calls provide additional diagnostic information when the Arduino Update subsystem reports an error.

An error does not mean the new firmware has been activated. Typically, the ESP32 continues running its existing firmware, provided the failure has not affected other parts of the application.

#### F. `ArduinoOTA.begin()` — starts the OTA service

```cpp
ArduinoOTA.begin();

Serial.printf("ArduinoOTA ready: %s.local\n",
              OTA_HOSTNAME);
```

This is the initialization step.

The call starts the OTA service using the settings you configured and, when enabled, its mDNS advertising service.

It does not upload firmware, switch partitions, or reboot the ESP32.

The message `ArduinoOTA ready` means the initialization code has run. It does not guarantee that your computer can discover the device or successfully connect to it.

### 3. How does `ArduinoOTA.handle()` trigger an update?

Your `loop()` should include:

```cpp
void loop() {
  ArduinoOTA.handle();

  // Other application tasks...
}
```

Think of `ArduinoOTA.handle()` as a service that the main loop must regularly give an opportunity to run.

It processes OTA protocol activity when a compatible uploader connects and sends commands or firmware data. It does not continuously scan for firmware files or decide on its own to install an update.

The process is roughly:

1. **Select the network port in Arduino IDE.** Compile your sketch and initiate the upload through the network port.
2. **The uploader connects to the ESP32.** The device's OTA service accepts the connection and handles authentication and the upload protocol.
3. **Firmware is written to the update partition.** The existing running application partition remains active while the new image is written to the other OTA slot.
4. **The library finalizes and restarts.** After a successful update, the library selects the new boot partition and restarts the ESP32.

One practical detail: `ArduinoOTA.handle()` must be called frequently. If your `loop()` spends a long time in `delay()`, lengthy display operations, or blocking sensor reads, OTA responsiveness may suffer. Keep your main loop responsive, especially while waiting for an upload.

### 4. Why does the ESP32 reboot and switch partitions without your code doing it?

The ArduinoOTA library and ESP-IDF OTA APIs perform those steps internally.

For a conventional two-slot OTA partition table, the sequence is:

1. **Before the upload:** The ESP32 runs `ota_0`, for example.
2. **During the upload:** The new image is written to `ota_1`. The running application is still `ota_0`.
3. **After successful finalization:** The OTA update mechanism selects `ota_1` as the next boot partition and records that selection in the OTA data partition.
4. **On restart:** The bootloader reads the OTA data and boots the selected application partition.
5. **After reboot:** Your `setup()` runs again, and `esp_ota_get_running_partition()` should now identify `ota_1`.

This is why your sketch does not necessarily need to call these functions explicitly:

```cpp
esp_ota_set_boot_partition(next);
ESP.restart();
```

The standard ArduinoOTA implementation handles the corresponding operations as part of a successful update. If you implement your own OTA process using the lower-level ESP-IDF OTA API, you would normally handle partition selection and restart yourself.

One additional caveat: firmware rollback and first-boot validation depend on the project's OTA configuration. If rollback is enabled, the newly booted application may need to mark itself valid; otherwise, the bootloader can revert to the previous working application.

### 5. How to verify that the partition really changed

Since you already print partition information in `setup()`, use it to verify the result:

```cpp
void printRunningPartition() {
  const esp_partition_t* running =
      esp_ota_get_running_partition();

  if (running != nullptr) {
    Serial.printf(
      "Running partition: %s, address: 0x%08X\n",
      running->label,
      (unsigned int)running->address);
  }
}
```

Call `printRunningPartition()` in `setup()`.

Then perform an OTA update and watch the Serial Monitor after the ESP32 restarts. For a normal alternating two-slot update, you should see the running partition change from `ota_0` to `ota_1`, or vice versa.

In summary: `setup()` configures and starts the OTA service; `loop()` keeps it responsive through `ArduinoOTA.handle()`; Arduino IDE initiates the upload; and the standard ArduinoOTA library handles successful finalization, boot-partition selection, and restart.
