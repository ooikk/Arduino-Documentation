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

#### Upload first OTA program

1. Connect the ESP32-S3 through USB.
2. Select its COM port.
3. Apply the board settings above.
4. Click **Upload**.
5. Open the Serial Monitor at 115200 baud.
6. Reset the board if necessary.
7. Confirm that it prints an IP address and:

```text
IP address: 192.168.0.187
ArduinoOTA ready: esp32s3-ota.local
```

#### Perform OTA with new Firmware

1. Put the computer and ESP32 on the same local network.
2. Wait briefly.
3. Go to **Tools → Port**.
4. Look for a network entry resembling:

```text
    esp32s3-ota at 192.168.0.187
```
5. Select that network port.

> Ignore the Serial Monitor warning when switching to the network port. The network port is for OTA uploading, not USB serial monitoring.

7. If the IDE asks which board it belongs to, choose:

 ```text
    ESP32S3 Dev Module
 ```

7. **Compile and upload:** Choose **Sketch → Upload** (`Ctrl+U`), or **Sketch → Configure and Upload** if needed.     

> Allow Arduino IDE through Windows Security/Firewall when prompted, and enter your configured OTA password when requested.

8. Wait for upload to complete 100% and Done. Below is the example of the output:


```text
Sketch uses 941735 bytes (29%) of program storage space. Maximum is 3145728 bytes.
Global variables use 51228 bytes (15%) of dynamic memory, leaving 276452 bytes for local variables. Maximum is 327680 bytes.
Sending invitation to 192.168.0.187 
Authenticating (PBKDF2-HMAC-SHA256)...
OK

Uploading: [                                                            ] 0%
Uploading: [=                                                           ] 1%
:
Uploading: [============================================================] 100% Done...
```

9. Return to USB serial monitoring:
   - Go to **Tools → Port → COM7**, then open Serial Monitor at your configured baud rate.
   - The ESP32 should restart into the new firmware, and you can inspect its startup messages and firmware version.

```cpp
    FW_VERSION = "1.0.1";
```

#### Important NOTE: 
If you power up your ESP32-S3 from a USB power adapter or another power source at a different location, without connecting it to Arduino IDE. If your firmware already connects to Wi-Fi and calls `ArduinoOTA.begin()`, the ESP32 can remain available for OTA uploads.

You can open PowerShell and ping whether the ESP32 is reachable:

```text
ping 192.168.0.187
```

You should see the response:

```text
PS C:\WINDOWS\system32> ping 192.168.0.187

Pinging 192.168.0.187 with 32 bytes of data:
Reply from 192.168.0.187: bytes=32 time=47ms TTL=64
Reply from 192.168.0.187: bytes=32 time=4ms TTL=64
Reply from 192.168.0.187: bytes=32 time=3ms TTL=64
Reply from 192.168.0.187: bytes=32 time=6ms TTL=64
```

Or check whether the hostname resolves:

```text
PS C:\WINDOWS\system32> ping esp32s3-ota.local

Pinging esp32s3-ota.local [192.168.0.187] with 32 bytes of data:
Reply from 192.168.0.187: bytes=32 time=3ms TTL=64
Reply from 192.168.0.187: bytes=32 time=4ms TTL=64
Reply from 192.168.0.187: bytes=32 time=10ms TTL=64
Reply from 192.168.0.187: bytes=32 time=11ms TTL=64

Ping statistics for 192.168.0.187:
    Packets: Sent = 4, Received = 4, Lost = 0 (0% loss),
Approximate round trip times in milli-seconds:
    Minimum = 3ms, Maximum = 11ms, Average = 7ms
```



## 5. Example B: ElegantOTA Web Upload

Install the three external libraries and enable async mode as described earlier.

### Library
- ElegantOTA by Ayush Sharma: https://github.com/ayushsharma82/ElegantOTA
- Async TCP by ESP32Async: https://github.com/ESP32Async/AsyncTCP
- ESP Async WebServer by ESP32Async: https://github.com/ESP32Async/ESPAsyncWebServer

### Enable asynchronous mode in ElegantOTA

Open this file in a text editor:

```text
C:\Users\ooikk\Documents\Arduino\libraries\ElegantOTA\src\ElegantOTA.h
```

Search for:

```cpp
#define ELEGANTOTA_USE_ASYNC_WEBSERVER
```

Set its value to `1`:

```cpp
#define ELEGANTOTA_USE_ASYNC_WEBSERVER 1
```

If the line already has a value of `0`, change it to `1`. If it is commented out, uncomment it and set it to `1`. Save the file and return to Arduino IDE.

### The check in Example B

```cpp
#include <ElegantOTA.h>

#if !defined(ELEGANTOTA_USE_ASYNC_WEBSERVER) || \
    ELEGANTOTA_USE_ASYNC_WEBSERVER != 1
#error "Enable async mode in libraries/ElegantOTA/src/ElegantOTA.h"
#endif
```

Keep `#include <ElegantOTA.h>` before the check so the macro defined by the library header is visible.

> **Note:** Example B uses asynchronous ElegantOTA, which requires ESPAsyncWebServer. Do not mix it with the synchronous `WebServer` implementation in the same firmware.


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

#if !defined(ELEGANTOTA_USE_ASYNC_WEBSERVER) || \
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

# Explain how it works: Example A (Arduino OTA) 

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

```text
(500000 * 100) / 1000000 = 50%
```

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


## 8. Manual OTA Rollback on ESP32-S3

For your ESP32-S3 using Arduino IDE and ArduinoOTA, you can implement a manual firmware rollback that lets you test new firmware for several days and then return to the previous version if necessary.

The important distinction is that ESP32 OTA rollback has two different mechanisms:

- **Automatic rollback:** The bootloader can revert to the previous firmware if a newly installed image fails its first-boot validation.
- **Manual rollback after days of testing:** Your currently running firmware deliberately selects the previous application partition and reboots into it.

For your scenario—testing for a few days and deciding later that you want the old firmware back—you need the second mechanism, or a remote command that triggers it.

### 1. How manual rollback works

Assume your ESP32-S3 has two OTA application slots.

#### Before the OTA update

| Partition | Firmware | Status |
|---|---|---|
| `ota_0` | Firmware v1.0 | Running |
| `ota_1` | Old / available | Not running |

Upload v1.1 through ArduinoOTA.

#### After the OTA update

| Partition | Firmware | Status |
|---|---|---|
| `ota_0` | Firmware v1.0 | Previous |
| `ota_1` | Firmware v1.1 | Running |

#### After manual rollback and reboot

| Partition | Firmware | Status |
|---|---|---|
| `ota_0` | Firmware v1.0 | Running |
| `ota_1` | Firmware v1.1 | Not running |

This works only if the previous firmware image remains intact in the other OTA slot. A subsequent OTA update can overwrite that image, so two slots do not provide a permanent history of firmware versions.

### 2. Code to manually roll back to the previous firmware

For a typical ESP32-S3 partition table with exactly two OTA slots, you can use the following function:

```cpp
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>

void rollbackFirmware() {
  const esp_partition_t* running =
      esp_ota_get_running_partition();

  if (running == nullptr) {
    Serial.println("Rollback failed: no running partition.");
    return;
  }

  // In a two-slot OTA layout, this identifies
  // the other OTA application partition.
  const esp_partition_t* previous =
      esp_ota_get_next_update_partition(running);

  if (previous == nullptr) {
    Serial.println("Rollback failed: no other OTA slot.");
    return;
  }

  Serial.printf("Current firmware: %s\n", running->label);
  Serial.printf("Rollback target: %s\n", previous->label);

  // Select the old firmware for the next boot.
  esp_err_t err = esp_ota_set_boot_partition(previous);

  if (err != ESP_OK) {
    Serial.printf(
        "Rollback selection failed: %s\n",
        esp_err_to_name(err));
    return;
  }

  Serial.println("Rollback selected. Restarting...");

  delay(200);
  esp_restart();
}
```

> **Important limitation:** This example assumes a two-slot OTA layout and that the other slot contains the previous firmware you want. The function `esp_ota_get_next_update_partition(running)` identifies an OTA update target; it does not prove that the partition contains your desired previous version. For a layout with more than two OTA slots, explicitly identify the previous partition.

The call to `esp_ota_set_boot_partition(previous)` selects the target for the next boot. `esp_restart()` then restarts the chip so the bootloader can load that firmware. ESP-IDF documents this selection-and-restart behavior in its OTA API.

*Source note: ESP-IDF Programming Guide v6.1 documentation.*

### 3. How do you trigger rollback after a few days?

The rollback function does nothing until your application calls it. You need to decide how you want to issue that command.

For example, you could use the Serial Monitor during development:

```cpp
void loop() {
  ArduinoOTA.handle();

  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "rollback") {
      rollbackFirmware();
    }
  }

  // Other application tasks...
}
```

After uploading the new firmware and testing it for several days:

1. Open the Arduino IDE Serial Monitor.
2. Set the baud rate to match your sketch.
3. Type `rollback` and send it.
4. The ESP32 selects the other OTA slot and restarts.
5. In `setup()`, print the running partition and firmware version to verify the result.

This example uses a serial command for simplicity. If the device is deployed remotely, you could instead trigger rollback through an authenticated web endpoint, MQTT command, or another secure management channel.

### 4. What about automatic rollback?

Automatic rollback is useful for a different situation: a new firmware image fails shortly after installation, before it has been confirmed as working.

ESP-IDF supports functions such as:

```cpp
esp_ota_mark_app_invalid_rollback_and_reboot();
```

This marks the running application as invalid and requests a reboot into the previous working application, provided rollback is enabled and a valid fallback exists.

*Source note: [ESP-IDF Programming Guide v6.1 documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/ota.html).*

However, this is not a timer-based rollback mechanism. If your new firmware runs normally for three days and has already been confirmed as valid, automatic rollback will not occur just because you decide the new version is unsatisfactory.

For your use case, a practical strategy is:

1. Keep the previous firmware in the other OTA slot.
2. Include a firmware version string in each build.
3. Test the new firmware for several days.
4. Trigger a manual rollback command if needed.
5. Verify the running version after reboot.

One final precaution: test this procedure on your development board before relying on it remotely. Ensure the old firmware is actually present, can boot with your current settings and partition layout, and is compatible with any changes the new firmware made to persistent settings in NVS.

---

# Automatic Over-The-Air (OTA) Firmware Updating

You publish a new firmware `.bin` file to a web server, and the ESP32-S3 periodically checks whether a newer version is available. If it is, the ESP32 downloads the binary, writes it to the inactive OTA partition, and reboots into the new firmware—without you manually initiating the upload in Arduino IDE.

This differs from both of your existing examples:

- **ArduinoOTA:** Arduino IDE pushes firmware to the ESP32.
- **ElegantOTA:** A user initiates the upload through a web browser.
- **Automatic pull OTA:** The ESP32 checks a server and downloads firmware when an update is available.

## 1. Available approaches

### Option A — HTTPS server and version manifest (recommended)

<img width="800" height="800" alt="image" src="https://github.com/user-attachments/assets/051695a1-817f-4519-8997-f137b1c0638b" />


Host a small version file and firmware binary on a web server. The ESP32 checks the manifest, compares its version with the installed firmware, and downloads the `.bin` only when an update is available.

- Checks at startup and/or on a schedule.
- Works with a conventional web server or static file hosting.
- Supports controlled releases and version tracking.

### Option B — GitHub Releases

<img width="1400" height="844" alt="image" src="https://github.com/user-attachments/assets/2e083cb6-7525-4619-a0a2-8976bf13b311" />


Publish each firmware `.bin` as a release asset, along with version information. The ESP32 checks a release or manifest URL and downloads the corresponding binary.

- Convenient for personal projects and prototypes.
- No need to maintain your own web server.
- Requires handling release URLs, HTTPS, and access restrictions.

### Option C — IoT device-management platform

<img width="1227" height="923" alt="image" src="https://github.com/user-attachments/assets/8688e0eb-73c5-4610-aff2-7a9d38dbe156" />


Use a platform that manages firmware versions, device groups, rollout schedules, and update status.

- Better for many devices.
- Can support staged rollouts and fleet monitoring.
- Requires platform setup and may involve subscription or integration work.

For your ESP32-S3 and Arduino framework, I would start with Option A, then consider a device-management platform if you later deploy many boards.

## 2. How the automatic update system works

1. **Publish new firmware.** Upload `firmware.bin` and update the manifest on your web server.
2. **ESP32 checks the manifest.** It checks at startup, every 6 hours, or on another schedule.
3. **Compare versions.** If the versions match, the ESP32 continues running and checks again later. If the server advertises a newer version, it downloads the firmware over HTTPS.
4. **Validate and install.** Check the image's validity and integrity, then select the new OTA partition.
5. **Reboot and run the new firmware.** Optionally confirm successful operation or roll back after a failed update.

Espressif's OTA system writes the new application to the inactive OTA slot and changes the boot selection after a successful update. Its rollback mechanism can restore the previous application when configured appropriately.

- ESP-IDF Programming Guide v6.1 documentation
  + [Over The Air Updates (OTA)](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/ota.html)
  + [ESP HTTPS OTA](https://github.com/espressif/esp-idf/blob/master/docs/en/api-reference/system/esp_https_ota.rst)


## 3. What goes on the web server?

For a simple first implementation, host two files.

### `version.json`

This small manifest is checked regularly by the ESP32:

```json
{
  "version": "1.0.2",
  "url": "[https://example.com/firmware.bin](https://example.com/firmware.bin)",
  "sha256": "YOUR_FIRMWARE_SHA256"
}
```

### `firmware.bin`

This is the compiled firmware binary generated by Arduino IDE. A typical way to create it is to select **Sketch → Export Compiled Binary**, then locate the exported `.bin` file in your sketch folder.

The example manifest is illustrative; the SHA-256 field must contain the actual hash of the binary. Your firmware should verify the hash before accepting the update, and HTTPS must validate the server certificate. For stronger security, consider signed firmware as well.

+ [Espressif: ESP HTTPS OTA](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_https_ota.html)
+ [Github: ESP HTTPS OTA](https://github.com/espressif/esp-idf/blob/master/docs/en/api-reference/system/esp_https_ota.rst)

When you release version `1.0.3`, upload the new binary and update the manifest to `1.0.3`. The ESP32 compares that version with its compiled-in version and downloads the binary only if the server advertises a newer release.

## 4. When should the ESP32 check?

| Strategy | How it behaves | Recommendation |
|---|---|---|
| At startup | Checks after Wi-Fi connects and the device is ready | Yes |
| Every 6 hours | Checks periodically while running | Good default |
| Once per day | Reduces requests and update checks | Good for stable devices |
| Manual trigger | Checks when a user presses a button or uses a web interface | Optional |

Avoid checking continuously in `loop()`. Use a non-blocking timer based on `millis()`, and don't repeatedly attempt an update if the server is unreachable. For a device that must remain responsive, schedule the network operation when it is safe to pause normal work.

## 5. Which Arduino OTA implementation should you use?

You do not need to choose between ArduinoOTA and ElegantOTA for the download mechanism. They can remain useful for development or manual recovery, while automatic updates use a separate HTTPS client.

For Arduino IDE and ESP32-S3, the main choices are:

- **Arduino HTTPUpdate / HTTPClient:** A relatively straightforward Arduino-framework approach for downloading firmware from a URL. Use HTTPS with certificate validation.
- **ESP-IDF `esp_https_ota`:** Espressif's dedicated HTTPS OTA API, with more advanced OTA features. Integration in Arduino IDE may require using the relevant ESP-IDF APIs available in your installed Arduino-ESP32 core.
- **A managed OTA platform:** Useful if you eventually want device inventories, rollout control, and update status reporting.

The official Espressif HTTPS OTA documentation describes the dedicated `esp_https_ota` approach.

+ [Espressif: ESP HTTPS OTA](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_https_ota.html)
+ [Github: ESP HTTPS OTA](https://github.com/espressif/esp-idf/blob/master/docs/en/api-reference/system/esp_https_ota.rst)


## 6. Important safeguards

Before deploying automatic updates, make sure you have:

- Two OTA application slots in the partition table. The device's partition layout must support OTA; don't assume the current layout does.
- HTTPS certificate validation rather than blindly accepting any server certificate.
- Version and integrity checks so an incomplete or corrupted download is not installed.
- Rollback or recovery planning for firmware that boots but fails during initialization.
- A power and connectivity strategy. A failed download should leave the existing firmware running.
- A release policy. Don't publish an unintended binary or overwrite a release file while devices are downloading it.

Espressif documents the OTA partition requirements and rollback behavior in its [OTA guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/ota.html).

— ESP-IDF Programming Guide v6.1 documentation     
+ [Espressif: ESP HTTPS OTA](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_https_ota.html)
+ [Github: ESP HTTPS OTA](https://github.com/espressif/esp-idf/blob/master/docs/en/api-reference/system/esp_https_ota.rst)

## 7. Recommendation for your ESP32-S3

Start with HTTPS, a version manifest, and automatic pull OTA. Keep your existing ArduinoOTA network port as a development and recovery option.

One decision will determine the most practical step-by-step setup:

- **Where to host the `.bin` file:** GitHub Releases, your own web server or static hosting, or an IoT OTA management platform.
- **When the ESP32 should check:** At startup only, periodically while running, or at startup and every 6 hours.
- **How much update protection you want:** HTTPS, version checks, and SHA-256 verification; those safeguards plus rollback after a failed first boot; or signed firmware and stronger production security.

---
# Automatic Pull OTA for ESP32-S3 — Arduino IDE 2.3.10

This implementation is designed for your ESP32-S3, Arduino-ESP32 core 3.3.12, and GitHub Releases. It checks for firmware at startup and every six hours, verifies the firmware's SHA-256 digest, and installs an update without requiring Arduino IDE to initiate it.

We'll use the Arduino Update API rather than add ElegantOTA or another OTA library. The core's Update API supports SHA-256 verification of the downloaded firmware before the update is accepted.

— GitHub  
- [ESP32 Arduino Update Library](https://github.com/espressif/arduino-esp32/tree/master/libraries/Update#readme)
- [Update.h](https://github.com/espressif/arduino-esp32/blob/master/libraries/Update/src/Update.h) 

## 1. Architecture

1. A GitHub repository hosts a small manifest file containing the current version and SHA-256 hash.
2. The ESP32-S3 checks the version at startup and every six hours.
3. GitHub Releases provides the firmware binary as a release asset.
4. The ESP32 downloads the firmware over HTTPS and verifies its SHA-256 hash.
5. If the image is valid, the ESP32 writes it to the OTA partition and reboots.

GitHub supports stable links to assets in the latest release, using the pattern `/releases/latest/download/asset-name`.

— GitHub Docs: [Linking to releases](https://docs.github.com/en/repositories/releasing-projects-on-github/linking-to-releases)

Because that URL redirects to a file host, TLS certificate verification must work for the redirect destination as well as GitHub itself.

## 2. Check the partition table first

In Arduino IDE, select your ESP32-S3 board and inspect **Tools → Partition Scheme**.

You need a scheme with two OTA application slots—usually `ota_0` and `ota_1`—and an OTA data partition. A scheme with only one application slot is unsuitable for this workflow.

For your N16R8 board, the 16 MB flash capacity gives you room to choose a suitable OTA layout, but don't select a scheme solely based on the board's flash size. The maximum firmware size must fit into each OTA application slot.

You can also print the running and next update partitions using the ESP-IDF APIs included with Arduino-ESP32:

```cpp
#include <esp_ota_ops.h>
#include <esp_partition.h>

void printOtaPartitions() {
  const esp_partition_t *running =
      esp_ota_get_running_partition();

  const esp_partition_t *next =
      esp_ota_get_next_update_partition(nullptr);

  if (running) {
    Serial.printf("Running: %s, size=%u bytes\n",
                  running->label,
                  (unsigned)running->size);
  }

  if (next) {
    Serial.printf("Next OTA: %s, size=%u bytes\n",
                  next->label,
                  (unsigned)next->size);
  } else {
    Serial.println("ERROR: No OTA update partition found");
  }
}
```

Run this diagnostic before attempting any update. If `next` is null, stop and fix the partition scheme first.

## 3. Set up the GitHub repository

Create or use a public GitHub repository for your firmware project. In the instructions below, replace `YOUR_GITHUB_USERNAME` and `YOUR_REPO` with your actual repository details.

### Step 3.1 — Create `version.txt`

Commit a file named `version.txt` to the repository's `main` branch:

```text
1.0.0
REPLACE_WITH_64_CHARACTER_SHA256_HASH
```

- Line 1: The firmware version.
- Line 2: The SHA-256 hash of the `.bin` file for that version.

We'll use a plain-text manifest to avoid needing an extra JSON parsing library.

### Step 3.2 — Compile and calculate the firmware hash

In Arduino IDE:

1. Set the firmware version in your sketch to `1.0.0`.
2. Select your ESP32-S3 board and the intended OTA partition scheme.
3. Choose **Sketch → Export Compiled Binary**.
4. Find the exported application `.bin` file. Do not use a bootloader, partition-table, or merged `.bin` file.

Open Windows PowerShell in the folder containing that binary and run:

```powershell
Get-FileHash .\YourSketch.ino.bin -Algorithm SHA256
```

Replace `YourSketch.ino.bin` with the actual filename. Copy the 64-character hash into line 2 of `version.txt`, then commit that file.

```powershell
PS C:\Users\ooikk\Documents\Arduino\02_OTA_Elegant\build\esp32.esp32.esp32s3> Get-FileHash .\firmware.ino.bin -Algorithm SHA256

Algorithm       Hash                                                                   Path
---------       ----                                                                   ----
SHA256          AF7D816315A176D6D651631A9DB317A3762E7C4571813435D59A4949FA2E65D6       C:\Users\ooikk\Documents\Ardu...
```

### Step 3.3 — Publish a GitHub Release

Create a GitHub Release with tag `v1.0.0`, and upload the application binary as a release asset named exactly:

```text
firmware.bin
```

The OTA code will use these URLs:

**Manifest:**

```text
https://raw.githubusercontent.com/YOUR_GITHUB_USERNAME/YOUR_REPO/main/02_MISC/02_OTA/version.txt
```

**Firmware:**

#### Option 1: Release Independence Path

```text
https://github.com/YOUR_GITHUB_USERNAME/YOUR_REPO/releases/latest/download/firmware.bin
```
>Note: Need to use `setInsecure()` with your GitHub Releases URL, to bypass the certificate verification check entirely. This allows the ESP32 to trust the github.com certificate and successfully follow the redirect to GitHub's CDN without throwing error -9984.

**Important:** If the release is a draft or prerelease, don't assume the latest URL will point to it. Publish a normal release for this initial implementation.

#### Option 2: Direct Repository Path

Upload directly to the sub-repository folder, preferbly at same location as `version.txt`.   

```text
https://raw.githubusercontent.com/YOUR_GITHUB_USERNAME/YOUR_REPO/main/02_MISC/02_OTA/firmware.bin
```
>Note: This is the secure path with CA Cert

Keep the manifest version and release asset in sync. When publishing a new version, upload the new binary to a new release and update `version.txt` with the new version and matching hash.



## 4. TLS certificate configuration

The code below uses `WiFiClientSecure` with a trusted root CA certificate. It does not use `setInsecure()`, which would disable server-certificate validation.

GitHub's TLS chain can use Let's Encrypt. The certificate is available from [Let's Encrypt](https://letsencrypt.org/certificates/)

**ISRG Root X1**
- Subject: C=US, O=Internet Security Research Group, CN=ISRG Root X1
- Key type: RSA 4096
- Trusted until: 2030-06-04 (generated 2015-06-04)
- CA details: crt.sh, issued certs
- Certificate details (self-signed): crt.sh, der, [pem](https://letsencrypt.org/certs/isrgrootx1.pem), txt
- Certificate details (cross-signed by DST Root CA X3): crt.sh, der, pem, txt (retired)
- CRL hostname: x1.c.lencr.org
- Test websites: valid, revoked, expired

ISRG Root X1: [pem](https://letsencrypt.org/certs/isrgrootx1.pem)

The code includes the Let's Encrypt ISRG Root X1 PEM certificate. Since GitHub may change its certificate chain or redirect destinations over time, test both the manifest request and firmware download against the actual network you use. If certificate verification fails, investigate the server's current certificate chain rather than disabling verification.

### NOTE: Root CA Certificates

For your ESP32-S3 automatic OTA project, there are two useful approaches: use the correct individual root CA certificates, as you are doing now, or use a maintained bundle of trusted root certificates.

#### 1. Where to find the CA requirements

GitHub does not provide one permanent root CA certificate that every client must use. The certificate chain can vary by hostname, hosting infrastructure, and changes over time. Your ESP32 must trust a root CA that validates the chain presented by each HTTPS server it contacts.

These are the official references worth bookmarking:

- **GitHub — TLS certificate troubleshooting**  
  Explains why an outdated or unsuitable CA certificate causes HTTPS verification failures.  
  [GitHub certificate troubleshooting](https://docs.github.com/en/authentication/troubleshooting-ssh/error-ssl-certificate-problem-verify-that-the-ca-cert-is-ok)  

- **Let's Encrypt — Certificate authority certificates**  
  Official source for ISRG Root X1, ISRG Root X2, and related certificate-chain information.  
  [Let's Encrypt certificate list](https://letsencrypt.org/certificates/)  
  *Let's Encrypt*
  - [ISRG CPS v2.0](https://letsencrypt.org/documents/isrg-cps-v2.0/)
  - [ISRG CP/CPS v5.8](https://letsencrypt.org/documents/isrg-cp-cps-v5.8/)

- **Espressif — ESP x509 Certificate Bundle**  
  Explains how ESP-IDF uses a collection of trusted root certificates instead of requiring one manually selected root for every public HTTPS server.  
  [Espressif certificate-bundle documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_crt_bundle.html)  
  *ESP-IDF Programming Guide*
  - [ESP x509 Certificate Bundle](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/protocols/esp_crt_bundle.html)


#### 2. What other CA sources can you use?

| Source | What it provides | Suitability for your project |
|---|---|---|
| Let's Encrypt — ISRG Root X1 | Individual root certificate | Already confirmed to work for your manifest endpoint. |
| Mozilla CA Certificate Store | A broad collection of public root CAs | Useful if GitHub endpoints or other services use different certificate chains. |
| curl CA bundle | A PEM bundle of trusted roots derived from Mozilla's store | Convenient for obtaining a maintained CA collection, but consider ESP32 memory and bundle compatibility. |
| Espressif certificate bundle | Root certificates in an ESP-IDF-compatible bundle format | A good option for applications that connect to multiple HTTPS services. |

Useful official sources:

- [Mozilla — CA Certificate Program](https://www.mozilla.org/en-US/about/governance/policies/security-group/certs/)
- [curl — CA certificates](https://curl.se/docs/caextract.html)
- [Espressif — Certificate Bundle](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_crt_bundle.html)

The Mozilla and curl sources provide broader collections, while Espressif documents how to convert and integrate a certificate bundle into ESP-IDF firmware.

#### 3. My recommendation for your ESP32-S3

For your current Arduino IDE 2.3.10 and ESP32 core 3.3.12 setup, I suggest:

1. **Keep ISRG Root X1 for now.** It has successfully validated your connection to `raw.githubusercontent.com`.
2. **Test the actual firmware download separately.** Your release URL redirects to a different asset-hosting domain, so success with the manifest does not guarantee the firmware download will work with the same CA.
3. **If you plan to add more HTTPS services, investigate an Espressif-compatible certificate bundle** rather than manually collecting individual roots.
4. **Keep certificate verification enabled.** Do not use `setInsecure()` for production OTA.

One final point: root certificates are not selected merely by the website's brand. They must validate the certificate chain the ESP32 receives for the specific hostname. That chain can change, so verify it again if an endpoint starts failing.




## 5. Complete Arduino sketch

Create a new sketch named `GitHub_Auto_OTA.ino`. This version uses the libraries bundled with Arduino-ESP32; no additional Arduino libraries are required.

Before compiling, change the Wi-Fi credentials and GitHub URLs, and make sure the version in `FIRMWARE_VERSION` matches the firmware currently installed on your board.

```cpp
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

// ============================================================
// 1. USER CONFIGURATION
// ============================================================

const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Version of THIS compiled firmware.
// Change this when building a new release.
#define FIRMWARE_VERSION "1.0.0"

const char* MANIFEST_URL ="https://raw.githubusercontent.com/YOUR_GITHUB_USERNAME/YOUR_REPO/version.txt";

// const char* FIRMWARE_URL = "https://github.com/YOUR_GITHUB_USERNAME/YOUR_REPO/releases/latest/download/firmware.bin";
const char* FIRMWARE_URL = "https://raw.githubusercontent.com/YOUR_GITHUB_USERNAME/YOUR_REPO/firmware.bin";

// Check every 6 hours while running.
const unsigned long OTA_CHECK_INTERVAL_MS =
  6UL * 60UL * 60UL * 1000UL;

// Avoid downloading unexpectedly large files.
const size_t MAX_FIRMWARE_SIZE = 8UL * 1024UL * 1024UL;

// ============================================================
// 2. TLS ROOT CERTIFICATE
// Let's Encrypt certificate chain terminating at ISRG Root X1
// Obtain/verify the current PEM from Let's Encrypt's official site.
// ============================================================

static const char ROOT_CA[] PROGMEM = R"PEM(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)PEM";

// ============================================================
// 3. STATE
// ============================================================

unsigned long lastOtaCheckMs = 0;
bool otaCheckHasRun = false;
bool otaInProgress = false;

// ============================================================
// 4. VERSION COMPARISON
// Supports versions such as 1.0.0, 1.2.3, 2.0.0
// ============================================================

bool parseVersion(const String& text,
                  unsigned long& major,
                  unsigned long& minor,
                  unsigned long& patch) {
  unsigned int a, b, c;
  char extra;

  if (sscanf(text.c_str(), "%u.%u.%u%c", &a, &b, &c, &extra) != 3) {
    return false;
  }

  major = a;
  minor = b;
  patch = c;
  return true;
}

int compareVersions(const String& a, const String& b) {
  unsigned long a1, a2, a3;
  unsigned long b1, b2, b3;

  if (!parseVersion(a, a1, a2, a3) ||
      !parseVersion(b, b1, b2, b3)) {
    return 0; // Invalid versions are rejected separately.
  }

  if (a1 != b1) return a1 > b1 ? 1 : -1;
  if (a2 != b2) return a2 > b2 ? 1 : -1;
  if (a3 != b3) return a3 > b3 ? 1 : -1;

  return 0;
}

// ============================================================
// 5. READ AND VALIDATE THE MANIFEST
//
// version.txt format:
// 1.0.1
// 64-character SHA-256 hash
// ============================================================

bool fetchManifest(String& remoteVersion, String& expectedSha256) {
  WiFiClientSecure tls;
  tls.setCACert(ROOT_CA);
  tls.setTimeout(15000);

  HTTPClient http;
  http.setConnectTimeout(15000);
  http.setTimeout(20000);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  Serial.println("Checking GitHub manifest...");

  if (!http.begin(tls, MANIFEST_URL)) {
    Serial.println("Manifest: HTTP begin failed");
    return false;
  }

  int code = http.GET();

  if (code != HTTP_CODE_OK) {
    Serial.printf("Manifest HTTP error: %d\n", code);
    http.end();
    return false;
  }

  String body = http.getString();
  http.end();

  body.trim();

  int newline = body.indexOf('\n');
  if (newline < 0) {
    Serial.println("Manifest format error: missing hash line");
    return false;
  }

  remoteVersion = body.substring(0, newline);
  remoteVersion.trim();

  expectedSha256 = body.substring(newline + 1);
  expectedSha256.trim();
  expectedSha256.toLowerCase();

  unsigned long a, b, c;
  if (!parseVersion(remoteVersion, a, b, c)) {
    Serial.println("Manifest version is invalid");
    return false;
  }

  if (expectedSha256.length() != 64) {
    Serial.println("Manifest SHA-256 must be 64 hex characters");
    return false;
  }

  for (size_t i = 0; i < expectedSha256.length(); i++) {
    char ch = expectedSha256[i];
    if (!((ch >= '0' && ch <= '9') ||
          (ch >= 'a' && ch <= 'f'))) {
      Serial.println("Manifest contains invalid SHA-256 characters");
      return false;
    }
  }

  Serial.printf("Installed version: %s\n", FIRMWARE_VERSION);
  Serial.printf("Published version: %s\n", remoteVersion.c_str());

  return true;
}

// ============================================================
// 6. DOWNLOAD AND INSTALL FIRMWARE
// ============================================================

bool downloadAndInstall(const String& expectedSha256) {
  WiFiClientSecure tls;
  tls.setCACert(ROOT_CA);
  tls.setTimeout(20000);

  HTTPClient http;
  http.setConnectTimeout(20000);
  http.setTimeout(30000);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  Serial.println("Connecting to GitHub firmware asset...");

  if (!http.begin(tls, FIRMWARE_URL)) {
    Serial.println("Firmware: HTTP begin failed");
    return false;
  }

  int code = http.GET();

  if (code != HTTP_CODE_OK) {
    Serial.printf("Firmware HTTP error: %d\n", code);
    http.end();
    return false;
  }

  int contentLength = http.getSize();

  if (contentLength <= 0 ||
      (size_t)contentLength > MAX_FIRMWARE_SIZE) {
    Serial.printf("Invalid firmware size: %d\n", contentLength);
    http.end();
    return false;
  }

  const esp_partition_t* target =
      esp_ota_get_next_update_partition(nullptr);

  if (target == nullptr) {
    Serial.println("No OTA target partition; update cancelled");
    http.end();
    return false;
  }

  Serial.printf("Target partition: %s\n", target->label);
  Serial.printf("Firmware size: %d bytes\n", contentLength);

  if ((size_t)contentLength > target->size) {
    Serial.println("Firmware does not fit OTA partition");
    http.end();
    return false;
  }

  if (!Update.begin((size_t)contentLength, U_FLASH)) {
    Serial.printf("Update.begin failed: %s\n",
                  Update.errorString());
    http.end();
    return false;
  }

  // Must be set after begin() and before writing firmware bytes.
  if (!Update.setSHA256(expectedSha256.c_str())) {
    Serial.println("Could not configure SHA-256 verification");
    Update.abort();
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();

  Serial.println("Downloading firmware...");

  size_t written = Update.writeStream(*stream);

  Serial.printf("Bytes written: %u / %u\n",
                (unsigned)written,
                (unsigned)contentLength);

  http.end();

  if (written != (size_t)contentLength) {
    Serial.println("Incomplete download; aborting update");
    Update.abort();
    return false;
  }

  // Update.end() validates the image and the configured SHA-256.
  if (!Update.end()) {
    Serial.printf("Update.end failed: %s\n",
                  Update.errorString());
    return false;
  }

  if (!Update.isFinished()) {
    Serial.println("Update did not finish correctly");
    return false;
  }

  Serial.println("Firmware verified and installed.");
  Serial.println("Rebooting into the new firmware...");
  delay(1000);
  ESP.restart();

  return true; // Normally not reached.
}

// ============================================================
// 7. CHECK FOR A NEWER RELEASE
// ============================================================

void checkForFirmwareUpdate() {
  if (otaInProgress) return;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("OTA check skipped: Wi-Fi disconnected");
    return;
  }

  otaInProgress = true;

  String remoteVersion;
  String expectedSha256;

  if (!fetchManifest(remoteVersion, expectedSha256)) {
    Serial.println("OTA check failed; existing firmware retained");
    otaInProgress = false;
    return;
  }

  int comparison =
      compareVersions(remoteVersion, String(FIRMWARE_VERSION));

  if (comparison < 0) {
    Serial.println("Server version is older; ignoring it");
    otaInProgress = false;
    return;
  }

  if (comparison == 0) {
    Serial.println("Firmware is already up to date");
    otaInProgress = false;
    return;
  }

  Serial.println("New firmware version available");

  downloadAndInstall(expectedSha256);

  // Normally a successful update reboots before reaching here.
  otaInProgress = false;
}

// ============================================================
// 8. WIFI
// ============================================================

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < 30000UL) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected. IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi unavailable; OTA will retry later");
  }
}

// ============================================================
// 9. SETUP AND LOOP
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32-S3 Automatic Pull OTA");
  Serial.printf("Running firmware version: %s\n",
                FIRMWARE_VERSION);

  const esp_partition_t* running =
      esp_ota_get_running_partition();

  const esp_partition_t* next =
      esp_ota_get_next_update_partition(nullptr);

  if (running) {
    Serial.printf("Running partition: %s, size=%u\n",
                  running->label,
                  (unsigned)running->size);
  }

  if (next) {
    Serial.printf("Next OTA partition: %s, size=%u\n",
                  next->label,
                  (unsigned)next->size);
  } else {
    Serial.println("WARNING: no OTA target partition found");
  }

  connectWiFi();

  // Check once at startup if Wi-Fi connected.
  if (WiFi.status() == WL_CONNECTED && next != nullptr) {
    checkForFirmwareUpdate();
  }

  lastOtaCheckMs = millis();
  otaCheckHasRun = true;
}

void loop() {
  // Reconnect Wi-Fi if it drops.
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastReconnectAttempt = 0;

    if (millis() - lastReconnectAttempt >= 15000UL) {
      lastReconnectAttempt = millis();
      Serial.println("Attempting Wi-Fi reconnect...");
      WiFi.reconnect();
    }
  }

  // Check every six hours after the startup check.
  if (otaCheckHasRun &&
      millis() - lastOtaCheckMs >= OTA_CHECK_INTERVAL_MS) {
    lastOtaCheckMs = millis();

    if (WiFi.status() == WL_CONNECTED) {
      checkForFirmwareUpdate();
    } else {
      Serial.println("Scheduled OTA check skipped: no Wi-Fi");
    }
  }

  // Your normal application tasks can run here.
  delay(10);
}
```

## 6. Step-by-step: Create a GitHub Release for ESP32-S3 automatic OTA

Two repositoty location options for `firmware.bin`.

### Option 1: Release Independence

We'll use your repository, [ooikk/Arduino-Documentation](https://github.com/ooikk/Arduino-Documentation), and publish the ESP32-S3 firmware as a GitHub Release.

The goal is to make this URL work for your ESP32:

```text
https://github.com/ooikk/Arduino-Documentation/releases/latest/download/firmware.bin
```
Your release will contain the compiled firmware file `firmware.bin`. Your `version.txt` file will remain in `02_MISC/02_OTA/` on the `main` branch and contain the firmware version and SHA-256 hash.

### Option 2: Direct Repository Path

Upload the `firmware.bin` directly to the `02_MISC/02_OTA/`

Your `version.txt` and `firmware.bin` files will remain in `02_MISC/02_OTA/`.

```text
https://raw.githubusercontent.com/ooikk/Arduino-Documentation/main/02_MISC/02_OTA/firmware.bin
https://raw.githubusercontent.com/ooikk/Arduino-Documentation/main/02_MISC/02_OTA/version.txt
```

### Step 1 — Compile the ESP32-S3 firmware

1. Open your automatic OTA sketch in Arduino IDE 2.3.10.
2. Confirm that the board is your ESP32-S3 and that the selected partition scheme supports OTA with two application slots.
3. Set the firmware version, for example:

   ```cpp
   #define FIRMWARE_VERSION "1.0.0"
   ```

4. Choose **Sketch → Export Compiled Binary**.
5. Locate the exported application `.bin` file in your sketch folder. Use the application binary, not the bootloader, partition-table, or merged binary.
6. Rename a copy of that application binary to:

   ```text
   firmware.bin
   ```

7. Keep the original exported file as a backup.

### Step 2 — Calculate the SHA-256 hash

On Windows, open PowerShell in the folder containing `firmware.bin` and run:

```powershell
Get-FileHash .\firmware.bin -Algorithm SHA256
```

Copy the value shown under `Hash`. It should be 64 hexadecimal characters.

For example, your `02_MISC/02_OTA/version.txt` file should look like this:

```text
1.0.0
paste_your_actual_64_character_sha256_hash_here
```

Do not use the example placeholder as the real hash. Save and commit the updated `version.txt` file to your repository's `main` branch before testing the release.

>Note: skip **Step 3 to 6** if you are using *Direct Repository Path*

### Step 3 — Open the Releases page

1. Open your repository: [ooikk/Arduino-Documentation](https://github.com/ooikk/Arduino-Documentation).
2. On the repository page, find **Releases**. Depending on your screen layout, it may appear in the right-hand sidebar.
3. Click **Releases**, then click **Draft a new release**.

[Managing releases in a repository](https://docs.github.com/en/repositories/releasing-projects-on-github/managing-releases-in-a-repository)

<img width="1000" height="631" alt="image" src="https://github.com/user-attachments/assets/2c524e6a-3da1-4b7d-9724-2d12c146180f" />


If you cannot find **Releases** in the sidebar, open this direct link: 

[Open your repository's Releases page](https://github.com/ooikk/Arduino-Documentation/releases)

### Step 4 — Fill in the new release details

On the **Draft a new release** page, enter these values:

| GitHub field | Value |
|---|---|
| Choose a tag | `v1.0.0` |
| Target | `main` |
| Release title | `ESP32-S3 Firmware v1.0.0` |
| Description | `Initial firmware release for ESP32-S3 automatic pull OTA.` |

For the tag:

1. Click **Choose a tag**.
2. Type `v1.0.0`.
3. Select **Create new tag: v1.0.0 on publish** (wording may vary).
4. Ensure the target branch is `main`.

Use a new tag for each subsequent firmware release, such as `v1.0.1`, `v1.0.2`, and `v1.1.0`. GitHub releases are associated with Git tags.

- [Releasing projects on github](https://github.com/github/docs/blob/main/content/repositories/releasing-projects-on-github/about-releases.md)
- [Managing releases in a repository](https://docs.github.com/en/repositories/releasing-projects-on-github/managing-releases-in-a-repository)


### Step 5 — Upload `firmware.bin`

<img width="1534" height="1194" alt="image" src="https://github.com/user-attachments/assets/f64c8c25-57ab-460f-9f16-5c6dbcee10a4" />

1. Find the **Attach binaries by dropping them here or selecting them** area near the bottom of the release form.
2. Drag your `firmware.bin` file into that area, or click the area and select the file.
3. Wait until the upload finishes.
4. Verify the asset list shows exactly `firmware.bin`.

**Important:** Do not upload `version.txt` as the firmware asset. The manifest is read directly from the `02_MISC` folder; the release asset is the compiled application binary.

GitHub allows manually uploaded binary assets to be downloaded through a stable latest-release URL.

- [Linking to releases](https://docs.github.com/en/repositories/releasing-projects-on-github/linking-to-releases)
- [REST API endpoints for release assets](https://docs.github.com/en/rest/releases/assets?apiVersion=2026-03-10)

### Step 6 — Publish the release

Before publishing, check that:

- The tag is `v1.0.0`.
- The target is `main`.
- The release is not marked as a prerelease.
- The asset is named `firmware.bin`.
- The binary corresponds to firmware version `1.0.0`.
- `version.txt` contains `1.0.0` and the matching SHA-256 hash.

Then click **Publish release**. Do not leave it as a draft. If the page offers **Set as the latest release**, ensure the release is selected as latest where appropriate. GitHub's latest-release behavior depends on release status and settings.


### Step 7 — Test the two URLs

After publishing, open these links in your browser.

#### A. Firmware binary

### For Option 1: Release Independence

[Download `firmware.bin` from the latest release](https://github.com/ooikk/Arduino-Documentation/releases/latest/download/firmware.bin)

### For Option 2: Direct Repository Path

[Download `firmware.bin` from direct repository path](https://raw.githubusercontent.com/ooikk/Arduino-Documentation/main/02_MISC/02_OTA/firmware.bin)

**Expected result:** Your browser downloads a file named `firmware.bin`.

#### B. Version manifest

[Open `version.txt`](https://raw.githubusercontent.com/ooikk/Arduino-Documentation/main/02_MISC/02_OTA/version.txt)

**Expected result:** The browser displays two lines: the firmware version and its SHA-256 hash.

If either URL fails, fix it before testing OTA on the ESP32-S3. The firmware URL will not work until a published release contains an asset with the exact name `firmware.bin`.

### Step 8 — Update the ESP32-S3

Once both URLs work:

1. Upload the automatic OTA sketch to the ESP32-S3 over USB.
2. Open Serial Monitor at 115200 baud.
3. Reset the board.
4. Check that it connects to Wi-Fi and reads `version.txt`.
5. If the installed firmware and published release versions match, it should report that the firmware is up to date.

When testing an upgrade, publish a new version with its matching binary and hash. The ESP32 should download the newer firmware on its next check.

For the next release, increment the firmware version, compile again, calculate the new SHA-256 hash, update `version.txt`, and publish a new release with the new `firmware.bin`.




## 7. How to test it safely

Do not start by publishing a new release to a board that you cannot recover. First upload this sketch manually over USB using Arduino IDE.

### Test 1 — Confirm the normal startup

1. Open Serial Monitor at 115200 baud.
2. Reset the ESP32-S3.
3. Confirm Wi-Fi connects and the running partition is printed.
4. Confirm the manifest returns HTTP 200.
5. Confirm the output says the firmware is up to date.

If the manifest request fails, check the GitHub URL, repository visibility, Wi-Fi, and certificate-validation errors in the serial log.

### Test 2 — Publish a test update

1. Change the firmware version in the sketch from `1.0.0` to `1.0.1`.
2. Add a visible change to the application, such as printing `Firmware 1.0.1 started`.
3. Export the compiled binary.
4. Calculate its SHA-256 hash.
5. Create a normal GitHub Release tagged `v1.0.1`, with the asset named `firmware.bin`.
6. Update `version.txt` to contain the new version and matching hash.
7. Keep the currently installed ESP32 firmware at version `1.0.0` and reset it.

The ESP32 should see that `1.0.1` is newer, download the release asset, validate the hash, write the new image, and reboot. After reboot, verify the new version in Serial Monitor.

### Test 3 — Verify that a bad hash is rejected

Temporarily change one character of the hash in `version.txt` while leaving the binary unchanged. The update should fail its SHA-256 check, and the currently running firmware should remain available. Restore the correct hash afterward.

### Test 4 — Verify the six-hour schedule

The interval is controlled by:

```cpp
const unsigned long OTA_CHECK_INTERVAL_MS =
  6UL * 60UL * 60UL * 1000UL;
```

For initial testing only, change it to:

```cpp
const unsigned long OTA_CHECK_INTERVAL_MS =
  2UL * 60UL * 1000UL;  // 2 minutes
```

Publish another test version and verify that the board discovers it on the next scheduled check. Restore the six-hour interval for normal use.

## 8. Recovery and important limitations

- **If the download fails:** The sketch aborts the incomplete update, and the existing application should remain intact.
- **If the SHA-256 does not match:** The update must not be accepted.
- **If the new firmware installs but fails at startup:** This sketch does not yet implement automatic first-boot rollback. That requires configuring and using ESP-IDF's OTA rollback mechanism, including marking the new image valid only after it passes startup checks.
- **If TLS fails:** Do not work around it with `setInsecure()`. Verify the current certificate chain and trusted root configuration.
- **If your application performs time-critical tasks:** Move the OTA operation to a safe maintenance window or a dedicated task. The example performs the network download synchronously, so normal `loop()` processing pauses during the update.
- **If an attacker can control the repository:** SHA-256 alone cannot prove that the firmware came from you if the attacker changes both the manifest and binary. HTTPS protects the connection, while signed firmware provides stronger release authenticity.

For production use, Espressif documents the relevant partition and rollback behavior in the [ESP-IDF OTA guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/ota.html).

— GitHub  
- [ESP32 Arduino Update Library](https://github.com/espressif/arduino-esp32/blob/master/libraries/Update/README.md)
- [HTTPS_OTA_Update.ino](https://github.com/espressif/arduino-esp32/blob/master/libraries/Update/examples/HTTPS_OTA_Update/HTTPS_OTA_Update.ino)

One final detail: `FIRMWARE_VERSION` is compiled into the firmware. Every time you publish a new release, update that constant before compiling, then publish the matching `.bin` and manifest hash. Otherwise, a device may repeatedly consider a release newer than the version it believes it is running.

---
## ESP32-S3 OTA: How `downloadAndInstall()` and `Update` work

In your ESP32-S3 automatic pull OTA project, `downloadAndInstall()` is the function responsible for downloading the new firmware binary from GitHub and writing it into the ESP32-S3's inactive OTA application partition.

The important distinction is that downloading firmware and installing firmware are two separate operations. `HTTPClient` retrieves the bytes over HTTPS; the Arduino `Update` library writes those bytes into flash and validates the update process.

### 1. The overall OTA sequence

1. **Check for an update** — Download `version.txt`; compare installed and published versions.
2. **`downloadAndInstall()`** — Open HTTPS connection to the firmware URL.
3. **Prepare the update** — Check image size and SHA-256; call `Update.begin()`.
4. **Download and write** — Read HTTP stream in chunks; pass bytes to `Update.write()`.
5. **Finalize** — Call `Update.end()`; check success and image integrity.
6. **Reboot** — ESP32 boots the newly selected OTA application partition.

The exact checks and call order depend on your implementation, but this is the usual structure for Arduino-ESP32 OTA using `HTTPClient`, `Update.h`, and the ESP-IDF OTA partition mechanism.

### 2. What happens inside `downloadAndInstall()`

Your implementation uses these main components:

| Component | Responsibility |
|---|---|
| `WiFiClientSecure` | Establishes HTTPS and validates the server certificate |
| `HTTPClient` | Sends the HTTP GET request and obtains the response |
| `Update` | Writes firmware bytes to the inactive OTA flash partition |
| `esp_ota_ops.h` | Provides ESP-IDF OTA partition functions |
| `esp_partition.h` | Provides partition information and operations |

Let's examine the steps in detail.

#### Step 1 — Create a secure connection and request the binary

Conceptually, the function starts with something like:

```cpp
WiFiClientSecure client;
client.setCACert(ROOT_CA);

HTTPClient http;
http.begin(client, FIRMWARE_URL);

int httpCode = http.GET();
```

- `WiFiClientSecure client;` creates a TLS-capable network client.
- `setCACert(ROOT_CA)` configures certificate verification.
- `http.begin()` prepares the request for the firmware URL.
- `http.GET()` sends an HTTP GET request to retrieve the binary.

The GET request can only proceed successfully if the TLS connection and HTTP exchange succeed.

You should normally require an HTTP `200 OK` response before accepting the firmware body. A redirect or an HTTP error page must not be mistaken for a firmware binary.

#### Step 2 — Check the firmware size

The HTTP response can provide the firmware length through the `Content-Length` header.

The function should check that:

- The length is known and greater than zero, if your download method requires it.
- The image fits inside the inactive OTA partition.
- The download is not obviously incomplete.

Your ESP32-S3 has two OTA application partitions, each approximately 3 MiB in your current partition layout. The firmware image must fit into the destination partition, regardless of the board's 16 MB total flash capacity.

#### Step 3 — Prepare flash using `Update.begin()`

This is the first major `Update` operation.

```cpp
if (!Update.begin(firmwareSize)) {
    Serial.printf("Update.begin failed: %s\n",
                  Update.errorString());
    http.end();
    return false;
}
```

Here, `firmwareSize` is the expected number of firmware bytes.

`Update.begin()` prepares the update subsystem to receive a new image. In the normal OTA configuration, the library targets the inactive application partition rather than overwriting the application currently running.

It also checks whether the update can be started, including whether the target partition has enough capacity.

**Why this matters:** your running firmware must remain intact while the new firmware is being downloaded and written. If the update fails before it is finalized, the currently running application can generally continue running.

#### Step 4 — Stream the firmware and call `Update.write()`

The firmware is not normally downloaded into one enormous RAM buffer. Instead, the function reads the response stream in manageable chunks.

The simplified pattern is:

```cpp
uint8_t buffer[1024];
size_t written = 0;

while (httpConnectedAndDataAvailable) {
    size_t bytesRead = readChunk(buffer, sizeof(buffer));

    if (bytesRead > 0) {
        size_t bytesWritten = Update.write(buffer, bytesRead);

        if (bytesWritten != bytesRead) {
            // Write failed or was incomplete.
            Update.abort();
            return false;
        }

        written += bytesWritten;
    }
}
```

The names `httpConnectedAndDataAvailable` and `readChunk()` above are illustrative placeholders, not actual Arduino API functions.

The key operation is:

```cpp
Update.write(buffer, bytesRead);
```

It passes bytes to the `Update` library, which handles writing them to the destination flash partition.

A robust implementation checks that the number of bytes written equals the number of bytes read. It should also distinguish a completed download from a connection that drops unexpectedly.

#### Step 5 — Validate the downloaded firmware

Your OTA workflow also uses a SHA-256 value from `version.txt`.

Conceptually, the manifest contains:

```text
1.2.0
<64-character SHA-256 digest>
```

The firmware download must be compared with the digest published in the manifest. This detects a binary that differs from the expected file, whether due to corruption, an incomplete download, or an unintended file change.

For a secure design, calculate SHA-256 over the complete downloaded binary and compare it with the trusted expected digest before accepting the update.

There are two separate security checks here:

- **TLS certificate verification:** validates the HTTPS server's identity and protects the transfer.
- **SHA-256 comparison:** verifies that the downloaded bytes match the expected digest.

A plain SHA-256 digest does not authenticate the publisher if an attacker can change both the binary and the manifest. A signed manifest or signed firmware image offers stronger protection against that scenario.

#### Step 6 — Finalize with `Update.end()`

This is the second major `Update` operation.

```cpp
if (!Update.end()) {
    Serial.printf("Update.end failed: %s\n",
                  Update.errorString());
    return false;
}
```

The `Update` library finalizes the write operation and checks whether the expected update data has been received. When finalization succeeds, the library completes the OTA process and arranges for the updated application partition to be used on the next boot.

An important distinction:

- `Update.write()` writes the incoming bytes.
- `Update.end()` finalizes the update.

A successful `Update.end()` does not mean the new application has already started executing.

The device is still running the old firmware until a reboot transfers control to the new image.

Some implementations use `Update.end(true)`, where the argument permits finalization even when fewer bytes than expected have arrived. You should not use that as a workaround for an incomplete download; it can weaken the expected-size safeguard.

#### Step 7 — Close the HTTP connection and reboot

After a successful update, the function should release the HTTP resources and reboot:

```cpp
http.end();

if (updateSucceeded) {
    delay(500);
    ESP.restart();
}
```

`http.end()` releases the HTTP connection resources. `ESP.restart()` resets the ESP32 so the bootloader can start the newly selected application.

On your current two-partition layout, the expected sequence is:

1. Firmware version 1.1.0 runs from `app0`.
2. The new image is written to `app1`.
3. The update is finalized successfully.
4. The device reboots.
5. The bootloader starts the application selected by the OTA update process.

The new firmware should report version 1.2.0, assuming that is the version compiled into the downloaded binary.

On subsequent updates, the target normally alternates between the OTA application partitions, depending on the currently running partition.

### 3. The key Update API functions

| Function | Purpose |
|---|---|
| `Update.begin(size)` | Start preparing the update for the specified image size |
| `Update.write(data, len)` | Write a block of firmware bytes |
| `Update.end()` | Finalize the update and verify that the expected write operation completed |
| `Update.isFinished()` | Check whether the Update library considers the update finished |
| `Update.hasError()` | Check whether the Update library recorded an error |
| `Update.errorString()` | Obtain a human-readable description of the latest Update error |
| `Update.abort()` | Abort an update in progress |
| `ESP.restart()` | Reboot the ESP32 |

Your project also uses ESP-IDF OTA and partition APIs for inspecting the current and next partitions. Those APIs help with diagnostics, but they serve a different role from the `Update` library's write and finalization operations.

### 4. What if something fails?

| Failure case | Recommended response |
|---|---|
| TLS or HTTP request fails | No valid firmware should be installed. Close the connection and retain the currently running application. |
| `Update.begin()` fails | The update cannot be started, often because of a size or partition issue. Report the error and do not reboot as if the update succeeded. |
| Download or hash verification fails | Abort the update and do not finalize an invalid image. The active application should remain the one in use. |
| `Update.end()` succeeds | The update is finalized. Reboot to start the new application, then verify its version and health. |

One final engineering detail: successful installation is not the same as successful operation of the new firmware. If you want automatic rollback when a new image boots but crashes or fails initialization, you need to configure and implement ESP-IDF's OTA rollback mechanism. Merely calling `ESP.restart()` does not guarantee rollback.

If you paste your current `downloadAndInstall()` function, I can annotate your actual code line by line, including where the SHA-256 check occurs, which partition `Update` writes to, and precisely when the boot partition changes.

---

# Using OneDrive, Google Drive, and other cloud storage for ESP32-S3 OTA

In principle, you can host both `version.txt` and `firmware.bin` on almost any cloud storage service that supports publicly accessible downloads. Your ESP32-S3 does not fundamentally care whether the files are stored on GitHub, Google Drive, OneDrive, Dropbox, or your own web server.

The important requirement is that the ESP32 must receive the actual file contents over HTTPS, not an HTML preview page, sign-in page, or download-confirmation page.

For your existing Arduino OTA project, the update flow can remain largely unchanged:

```text
ESP32-S3
Checks the manifest and compares firmware versions

Cloud storage
version.txt and firmware.bin are downloaded over HTTPS

Existing OTA updater
Verify digest → write inactive partition → finalize → reboot
```

You normally change the two URL constants and perhaps the redirect-handling code. Your version comparison, partition handling, `Update` calls, and reboot logic need not change if the cloud provider serves compatible responses.

## 1. Compare the cloud-storage options

These options differ mainly in how they expose files to a device that has no browser session or interactive sign-in.

| Cloud option | Can host OTA files? | Main consideration |
|---|---|---|
| GitHub raw content | Yes | Simple direct file URLs; you already have this working |
| Google Drive | Yes, with a suitable public-download link | Share links may return a confirmation page or redirect |
| OneDrive | Yes, with a suitable anonymous download link | Sharing URLs and redirect behaviour can vary |
| Dropbox | Yes, with a suitable shared link | `dl=1` requests a download, but redirects may still occur |
| Object storage / static hosting | Yes | Often best suited to a production firmware distribution endpoint |

For your project, I'd compare GitHub raw content, Google Drive, and OneDrive first because they fit your existing workflow. Dropbox is another workable option. For a larger deployment, object storage or a static web host can give you more predictable control over URLs, caching, and access policies.

### GitHub raw content — your current working solution

Your URLs can remain in this form:

```cpp
static const char* MANIFEST_URL =
    "https://raw.githubusercontent.com/OWNER/REPO/main/02_MISC/02_OTA/version.txt";

static const char* FIRMWARE_URL =
    "https://raw.githubusercontent.com/OWNER/REPO/main/02_MISC/02_OTA/firmware.bin";
```

Replace `OWNER` and `REPO` with the repository owner and repository name.

**Advantages:** direct file content, simple URLs, and no need to implement cloud authentication for a public repository.

**Considerations:** repository contents are public, and a branch-based URL can serve a different file after a later commit. For reproducible releases, a versioned path or immutable commit reference is preferable.

### Google Drive

Google Drive is usable if the files can be downloaded anonymously. Google's sharing controls allow you to make files available to anyone with the link, subject to account or administrator restrictions.

Typical workflow:

1. Upload `version.txt` and `firmware.bin`.
2. Set each file's access to **Anyone with the link — Viewer**, if that option is available.
3. Obtain each file's sharing URL and file ID.
4. Test whether the URL returns the actual content to an unauthenticated client.

A commonly used download URL pattern is:

```text
https://drive.google.com/uc?export=download&id=FILE_ID
```

This is a URL pattern to test, not a guarantee that every file will download directly. Google Drive can return redirects, confirmation pages, or access-denied responses, so you must test the actual response.

For a small manifest, Google Drive may be adequate. For a firmware binary, I would be more cautious about confirmation pages, redirects, and download quotas.

### OneDrive

OneDrive also supports anonymous sharing when the account and sharing policies allow it. Microsoft documents that an **Anyone** link can allow access without requiring recipients to sign in, though organizational policies may disable that option.

Typical workflow:

1. Upload both files.
2. Create a view-only sharing link for each.
3. Ensure the link permits downloading without an account or interactive authentication.
4. Test the link from a private browser window and then from the ESP32.

A `download=1` parameter is used with some OneDrive and SharePoint sharing URLs, but it should not be assumed to work for every link type.

The key issue is not just obtaining a share URL; it is ensuring that the final HTTPS response is the firmware file itself.

### Dropbox

Dropbox provides another option. Its official guidance explains that adding `dl=1` requests a file download, while `raw=1` can redirect to a URL that serves the content directly.

Example pattern:

```text
https://www.dropbox.com/scl/fi/SHARE_ID/firmware.bin?rlkey=KEY&dl=1
```

Use the actual link Dropbox generates, retaining its required parameters. Your ESP32 may still need to handle redirects.

## 2. What needs to change in your Arduino code?

In the simplest case, only the URL definitions change. Your manifest parser and firmware writer can stay as they are.

For example, if you choose Google Drive:

```cpp
static const char* MANIFEST_URL =
    "https://drive.google.com/uc?export=download&id=MANIFEST_FILE_ID";

static const char* FIRMWARE_URL =
    "https://drive.google.com/uc?export=download&id=FIRMWARE_FILE_ID";
```

If you choose OneDrive, substitute the actual anonymous download URLs for those two constants instead.

However, because your existing project uses strict TLS verification and has already encountered a certificate issue with GitHub, you should check both the initial hostname and every redirected hostname before concluding that a provider is compatible.

### The four areas to check

- **URL configuration:** Point `MANIFEST_URL` and `FIRMWARE_URL` at the new files.
- **Redirect handling:** Determine whether the provider responds directly with the file or redirects to another hostname. If it redirects, the client must follow it correctly.
- **TLS certificates:** Ensure the configured CA certificates validate each hostname in the download chain. Do not use `setInsecure()` to work around a failure.
- **Response validation:** Verify the HTTP status, binary size, and SHA-256 digest. Do not pass an HTML preview or login page to `Update.write()`.

## 3. Why redirects matter for your HTTPClient code

Your Google Sheets code already demonstrates manual redirect handling:

```cpp
http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);

int httpCode = http.GET();

if (httpCode == 302) {
    String redirectUrl = http.getLocation();
    // Close the first request and issue a new HTTPS request.
}
```

This is useful when you want to inspect the redirect destination and create a new `WiFiClientSecure` for it.

But note two differences:

- Your telemetry code handles Google Apps Script redirects for its particular response pattern. It does not automatically handle every possible cloud-storage redirect.
- A redirect may use `301`, `302`, `303`, `307`, or `308`, and there may be more than one redirect. Your handler should account for the statuses that the selected provider actually uses.

For a GET request to retrieve a firmware binary, following a redirect normally means issuing another GET request to the destination URL. For a POST request, redirect semantics can be different, so your telemetry code should not be copied blindly into the OTA downloader.

**My recommendation:** keep the OTA downloader's redirect logic separate from the telemetry logic. First test each provider's actual response, then add only the handling that is needed.

## 4. How to test a cloud provider before changing the ESP32

Test each file URL independently.

| Test | Expected result |
|---|---|
| Open manifest URL in a private browser window | Displays the two manifest lines, without sign-in |
| Open firmware URL in a private browser window | Downloads `firmware.bin`, not an HTML page |
| Check initial HTTP response | Usually `200 OK` or a redirect that can be followed |
| Check TLS for each hostname | Certificate validation succeeds |
| Check downloaded binary size | Matches the expected file size |
| Check SHA-256 | Matches the manifest digest for that exact binary |

For a quick Windows test, download the files with `curl.exe`:

```powershell
curl.exe -fL "YOUR_MANIFEST_URL" -o version.txt
curl.exe -fL "YOUR_FIRMWARE_URL" -o firmware.bin
```

The `-L` option follows redirects. Then calculate the firmware's SHA-256:

```powershell
Get-FileHash .\firmware.bin -Algorithm SHA256
```

Compare that hash with the second line of `version.txt`. This verifies the downloaded file, although Windows' successful TLS connection does not guarantee that the ESP32's configured CA certificates will validate the same host.

## 5. My recommendation for your ESP32-S3 project

I'd use this order:

1. **Keep GitHub raw content as the baseline.** Your current OTA download and installation already work with this URL.
2. **Test Google Drive or OneDrive as an experiment.** Upload both files, create anonymous download links, and verify the actual responses before changing the firmware.
3. **Keep the update logic provider-independent.** The downloader should accept a URL, handle supported redirects, validate the response and digest, and write to the inactive OTA partition.
4. **Consider object storage for production.** A service designed to serve binary objects is generally a better fit than a consumer file-sharing interface when you need predictable downloads and repeated firmware releases.

One final security point: a publicly accessible firmware URL is effectively a public distribution endpoint. Keep the firmware digest check, and consider signing the firmware or manifest if you intend to deploy this updater beyond your own test devices.

If you'd like to experiment next, I suggest trying Google Drive first. You can retain your current `downloadAndInstall()` implementation and change the URL constants initially, then inspect the ESP32's HTTP status and TLS diagnostics to determine whether additional redirect handling is necessary.

---
# Reference & Other Resources

https://simpleota.com/blog/
