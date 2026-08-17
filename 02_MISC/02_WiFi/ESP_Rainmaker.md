

# ESP32-S3 ESP RainMaker Guide (QWEN)

[![Link](https://img.shields.io/badge/Rainmaker_Gemini-blue?logo=github)](
https://github.com/ooikk/Arduino-Documentation/blob/main/02_MISC/02_WiFi/ESP_Rainmaker.md#esp-rainmaker-overview-gemini)

This is a practical, step-by-step guide for building an ESP RainMaker device with an ESP32-S3 and Arduino IDE. The device reports telemetry and controls an LED from the ESP RainMaker application.

> **Important clarification:** The `TOPIC_*` strings below look like Adafruit IO MQTT feed topics. ESP RainMaker does **not** normally allow arbitrary MQTT topics such as `ooikk/feeds/temperature`. RainMaker uses a **node/device/parameter model** and manages MQTT topics internally.
>
> The correct RainMaker approach is to map each original feed or topic to a RainMaker **device parameter**.

## 1. Mapping Topics to RainMaker Parameters

The original topics are:

```cpp
const char* TOPIC_STATUS    = "ooikk/feeds/status";
const char* TOPIC_TEMP      = "ooikk/feeds/temperature";
const char* TOPIC_RSSI      = "ooikk/feeds/rssi";
const char* TOPIC_UPTIME    = "ooikk/feeds/uptime";
const char* TOPIC_LED_SET   = "ooikk/feeds/led-control";
const char* TOPIC_LED_STATE = "ooikk/feeds/led-state";
const char* TOPIC_BUTTON    = "ooikk/feeds/button";
```

The equivalent RainMaker model used in this guide is:

| Original Topic | RainMaker Replacement | Direction | Meaning |
|---|---|---|---|
| `TOPIC_STATUS` | `Telemetry` device, `status` parameter | Device → Cloud | `"online"`, `"boot"`, and similar status values |
| `TOPIC_TEMP` | `Temperature` device, `temperature` parameter | Device → Cloud | Temperature in °C |
| `TOPIC_RSSI` | `Telemetry` device, `rssi` parameter | Device → Cloud | Wi-Fi RSSI in dBm |
| `TOPIC_UPTIME` | `Telemetry` device, `uptime` parameter | Device → Cloud | Seconds since boot |
| `TOPIC_LED_SET` | `LED` device, standard `power` parameter | Cloud → Device | Command received from the application |
| `TOPIC_LED_STATE` | `LED` device, `led-state` parameter | Device → Cloud | Reported LED state |
| `TOPIC_BUTTON` | `Telemetry` device, `button` parameter | Device → Cloud | Button event |

In a standard MQTT sketch, you might write:

```cpp
mqtt.publish(TOPIC_TEMP, "25.4");
```

In RainMaker, you instead report a parameter:

```cpp
esp_rmaker_param_update_and_report(
  temp_param,
  esp_rmaker_float(25.4)
);
```

Instead of subscribing to `TOPIC_LED_SET`, RainMaker invokes your LED write callback when the application changes the LED parameter.

## 2. Install Arduino IDE Support

### 2.1 Install the ESP32 Arduino Core

1. Open Arduino IDE.
2. Go to:

   ```text
   File → Preferences
   ```

3. In **Additional Boards Manager URLs**, add:

   ```text
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```

4. Go to:

   ```text
   Tools → Board → Boards Manager
   ```

5. Search for:

   ```text
   esp32
   ```

6. Install **esp32 by Espressif Systems**.

Use a recent version because ESP RainMaker support is updated frequently.

### 2.2 Install the ESP RainMaker Library

1. Go to:

   ```text
   Sketch → Include Library → Manage Libraries
   ```

2. Search for:

   ```text
   ESP RainMaker
   ```

3. Install:

   ```text
   ESP RainMaker by Espressif Systems
   ```

This library provides the Arduino-facing ESP RainMaker APIs.

### 2.3 Select the ESP32-S3 Board

Go to:

```text
Tools → Board
```

Select:

```text
ESP32S3 Dev Module
```

Typical settings are:

```text
Board:              ESP32S3 Dev Module
USB CDC On Boot:    Enabled
CPU Frequency:      240 MHz
Flash Mode:         QIO 80 MHz
Flash Size:         4 MB or 8 MB, depending on the board
Partition Scheme:   Huge APP (3 MB No OTA) or a larger scheme
Upload Speed:       921600
Port:               Your ESP32-S3 COM port
```

Important notes:

- ESP RainMaker firmware is relatively large.
- If you encounter flash or partition errors, choose a larger partition scheme.
- If Arduino OTA is not required, `Huge APP` is usually the easiest option.
- If the board has 8 MB of flash, select an 8 MB partition scheme when available.

## 3. Hardware Used

This example assumes the following hardware.

### LED

Use an external LED on GPIO 2:

```text
GPIO2 ---- resistor ---- LED ---- GND
```

You can change the LED pin with:

```cpp
const int LED_PIN = 2;
```

Notes:

- Some ESP32-S3 development boards have an addressable RGB LED on GPIO 48.
- If the onboard LED is addressable, `digitalWrite()` will not control it correctly.
- For addressable LEDs, use a library such as `Adafruit_NeoPixel` or `FastLED`.

### Push Button

This example uses the BOOT button on GPIO 0:

```cpp
const int BUTTON_PIN = 0;
```

GPIO 0 can be used as a runtime button input with the internal pull-up resistor:

```cpp
pinMode(BUTTON_PIN, INPUT_PULLUP);
```

Button logic:

```text
Not pressed = HIGH
Pressed     = LOW
```

## 4. ESP RainMaker Mobile-App Setup

1. Install the ESP RainMaker application from the Google Play Store or Apple App Store.
2. Create an ESP RainMaker account or sign in.
3. Later, select:

   ```text
   Add Device
   ```

The application provisions the ESP32-S3 over BLE or SoftAP and sends it the Wi-Fi credentials.

This example uses BLE provisioning by default.

## 5. Complete Arduino Sketch

Create a new Arduino sketch, for example:

```text
RainMakerTelemetry.ino
```

Paste the following code:

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiProv.h>
#include <RMaker.h>

// ------------------------------------------------------------------
// Original topic names kept for reference.
//
// ESP RainMaker does not publish directly to these topics.
// ------------------------------------------------------------------
const char* TOPIC_STATUS    = "ooikk/feeds/status";
const char* TOPIC_TEMP      = "ooikk/feeds/temperature";
const char* TOPIC_RSSI      = "ooikk/feeds/rssi";
const char* TOPIC_UPTIME    = "ooikk/feeds/uptime";
const char* TOPIC_LED_SET   = "ooikk/feeds/led-control";
const char* TOPIC_LED_STATE = "ooikk/feeds/led-state";
const char* TOPIC_BUTTON    = "ooikk/feeds/button";

// ------------------------------------------------------------------
// RainMaker parameter names
// ------------------------------------------------------------------
const char* PARAM_STATUS = "status";
const char* PARAM_TEMP   = "temperature";
const char* PARAM_RSSI   = "rssi";
const char* PARAM_UPTIME = "uptime";

// RainMaker standard switch devices normally use "power".
const char* PARAM_LED_CONTROL = "power";

// Additional reported LED-state parameter
const char* PARAM_LED_STATE = "led-state";

// Button-event parameter
const char* PARAM_BUTTON = "button";

// ------------------------------------------------------------------
// Provisioning settings
// ------------------------------------------------------------------
const char* PROV_SERVICE_NAME = "PROV_OOIKK";
const char* PROV_POP = "abcd1234";

// ------------------------------------------------------------------
// Hardware pins
// ------------------------------------------------------------------
const int LED_PIN = 2;
const int BUTTON_PIN = 0;

// ------------------------------------------------------------------
// Timing
// ------------------------------------------------------------------
const uint32_t TELEMETRY_INTERVAL_MS = 10000;
const uint32_t BUTTON_DEBOUNCE_MS = 50;

// ------------------------------------------------------------------
// RainMaker parameter handles
// ------------------------------------------------------------------
static esp_rmaker_param_t* status_param = nullptr;
static esp_rmaker_param_t* temp_param = nullptr;
static esp_rmaker_param_t* rssi_param = nullptr;
static esp_rmaker_param_t* uptime_param = nullptr;
static esp_rmaker_param_t* button_param = nullptr;
static esp_rmaker_param_t* led_power_param = nullptr;
static esp_rmaker_param_t* led_state_param = nullptr;

// ------------------------------------------------------------------
// Runtime state
// ------------------------------------------------------------------
static bool led_state = false;
static volatile bool wifi_connected = false;

// ------------------------------------------------------------------
// RainMaker write callback for LED control
//
// This is the RainMaker equivalent of subscribing to TOPIC_LED_SET.
// ------------------------------------------------------------------
static esp_err_t led_write_cb(
  device_handle_t device,
  param_handle_t param,
  const param_val_t* val,
  void* priv_data,
  write_ctx_t* ctx
) {
  if (!param || !val) {
    return ESP_ERR_INVALID_ARG;
  }

  const char* name = esp_rmaker_param_get_name(param);

  if (strcmp(name, PARAM_LED_CONTROL) == 0) {
    led_state = val->val.b;

    digitalWrite(
      LED_PIN,
      led_state ? HIGH : LOW
    );

    // Report the extra led-state parameter
    if (led_state_param) {
      esp_rmaker_param_update_and_report(
        led_state_param,
        esp_rmaker_bool(led_state)
      );
    }

    // Acknowledge and update the standard power parameter
    esp_rmaker_param_update_and_report(
      param,
      esp_rmaker_bool(led_state)
    );

    Serial.printf(
      "LED set to %s\n",
      led_state ? "ON" : "OFF"
    );
  }

  return ESP_OK;
}

// ------------------------------------------------------------------
// Telemetry reporting
//
// This is the RainMaker equivalent of publishing to the feed topics.
// ------------------------------------------------------------------
void reportTelemetry() {
  if (
    !status_param ||
    !temp_param ||
    !rssi_param ||
    !uptime_param
  ) {
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    // If Wi-Fi is down, the device usually cannot publish "offline".
    // RainMaker detects the node as offline through the MQTT disconnect.
    return;
  }

  float temp_c = temperatureRead();
  int rssi = WiFi.RSSI();
  int uptime_seconds = static_cast<int>(millis() / 1000);

  esp_rmaker_param_update_and_report(
    status_param,
    esp_rmaker_str("online")
  );

  esp_rmaker_param_update_and_report(
    temp_param,
    esp_rmaker_float(temp_c)
  );

  esp_rmaker_param_update_and_report(
    rssi_param,
    esp_rmaker_int(rssi)
  );

  esp_rmaker_param_update_and_report(
    uptime_param,
    esp_rmaker_int(uptime_seconds)
  );

  Serial.printf(
    "Telemetry: temp=%.2f C, RSSI=%d dBm, uptime=%d s\n",
    temp_c,
    rssi,
    uptime_seconds
  );
}

// ------------------------------------------------------------------
// Button handling
//
// This is the RainMaker equivalent of publishing to TOPIC_BUTTON.
// ------------------------------------------------------------------
void handleButton() {
  static bool pressed = false;
  static uint32_t last_check = 0;

  if (millis() - last_check < BUTTON_DEBOUNCE_MS) {
    return;
  }

  last_check = millis();

  bool active = digitalRead(BUTTON_PIN) == LOW;

  if (active && !pressed) {
    pressed = true;

    if (button_param) {
      esp_rmaker_param_update_and_report(
        button_param,
        esp_rmaker_str("pressed")
      );
    }

    Serial.println("Button pressed");

    // Optional:
    // Uncomment this block if the physical button should toggle the LED.
    /*
    led_state = !led_state;

    digitalWrite(
      LED_PIN,
      led_state ? HIGH : LOW
    );

    if (led_power_param) {
      esp_rmaker_param_update_and_report(
        led_power_param,
        esp_rmaker_bool(led_state)
      );
    }

    if (led_state_param) {
      esp_rmaker_param_update_and_report(
        led_state_param,
        esp_rmaker_bool(led_state)
      );
    }
    */
  }
  else if (!active && pressed) {
    pressed = false;

    if (button_param) {
      esp_rmaker_param_update_and_report(
        button_param,
        esp_rmaker_str("released")
      );
    }

    Serial.println("Button released");
  }
}

// ------------------------------------------------------------------
// Setup
// ------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("----------------------------------------");
  Serial.println("ESP32-S3 ESP RainMaker telemetry example");
  Serial.println("----------------------------------------");

  // Keep compiler quiet when the topic constants are only documentation
  (void)TOPIC_STATUS;
  (void)TOPIC_TEMP;
  (void)TOPIC_RSSI;
  (void)TOPIC_UPTIME;
  (void)TOPIC_LED_SET;
  (void)TOPIC_LED_STATE;
  (void)TOPIC_BUTTON;

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(
    LED_PIN,
    led_state ? HIGH : LOW
  );

  // ----------------------------------------------------------------
  // 1. Create the RainMaker node
  // ----------------------------------------------------------------
  esp_rmaker_node_t* node = esp_rmaker_node_create(
    "OOIKK ESP32-S3",
    "esp.node",
    "1.0"
  );

  if (!node) {
    Serial.println("Failed to create RainMaker node");

    while (true) {
      delay(1000);
    }
  }

  // ----------------------------------------------------------------
  // 2. Create the LED device
  //
  // This replaces:
  // TOPIC_LED_SET   -> LED/power writable parameter
  // TOPIC_LED_STATE -> LED/led-state readable parameter
  // ----------------------------------------------------------------
  esp_rmaker_device_t* led_dev =
    esp_rmaker_device_create(
      "LED",
      "esp.device.switch",
      nullptr
    );

  led_power_param =
    esp_rmaker_param_create(
      PARAM_LED_CONTROL,
      "esp.param.power",
      esp_rmaker_bool(false),
      PROP_FLAG_READ |
      PROP_FLAG_WRITE |
      PROP_FLAG_PERSIST
    );

  led_state_param =
    esp_rmaker_param_create(
      PARAM_LED_STATE,
      "custom.param.led_state",
      esp_rmaker_bool(false),
      PROP_FLAG_READ
    );

  esp_rmaker_device_add_param(
    led_dev,
    led_power_param
  );

  esp_rmaker_device_add_param(
    led_dev,
    led_state_param
  );

  esp_rmaker_device_assign_write_cb(
    led_dev,
    led_write_cb
  );

  esp_rmaker_node_add_device(
    node,
    led_dev
  );

  // ----------------------------------------------------------------
  // 3. Create the temperature device
  //
  // This replaces TOPIC_TEMP.
  // ----------------------------------------------------------------
  esp_rmaker_device_t* temp_dev =
    esp_rmaker_device_create(
      "Temperature",
      "esp.device.temperature-sensor",
      nullptr
    );

  temp_param =
    esp_rmaker_param_create(
      PARAM_TEMP,
      "esp.param.temperature",
      esp_rmaker_float(0.0f),
      PROP_FLAG_READ
    );

  esp_rmaker_device_add_param(
    temp_dev,
    temp_param
  );

  esp_rmaker_node_add_device(
    node,
    temp_dev
  );

  // ----------------------------------------------------------------
  // 4. Create the telemetry device
  //
  // This replaces:
  // TOPIC_STATUS
  // TOPIC_RSSI
  // TOPIC_UPTIME
  // TOPIC_BUTTON
  // ----------------------------------------------------------------
  esp_rmaker_device_t* telemetry_dev =
    esp_rmaker_device_create(
      "Telemetry",
      "custom.device.telemetry",
      nullptr
    );

  status_param =
    esp_rmaker_param_create(
      PARAM_STATUS,
      "custom.param.status",
      esp_rmaker_str("boot"),
      PROP_FLAG_READ
    );

  rssi_param =
    esp_rmaker_param_create(
      PARAM_RSSI,
      "custom.param.rssi",
      esp_rmaker_int(0),
      PROP_FLAG_READ
    );

  uptime_param =
    esp_rmaker_param_create(
      PARAM_UPTIME,
      "custom.param.uptime",
      esp_rmaker_int(0),
      PROP_FLAG_READ
    );

  button_param =
    esp_rmaker_param_create(
      PARAM_BUTTON,
      "custom.param.button_event",
      esp_rmaker_str("idle"),
      PROP_FLAG_READ
    );

  esp_rmaker_device_add_param(
    telemetry_dev,
    status_param
  );

  esp_rmaker_device_add_param(
    telemetry_dev,
    rssi_param
  );

  esp_rmaker_device_add_param(
    telemetry_dev,
    uptime_param
  );

  esp_rmaker_device_add_param(
    telemetry_dev,
    button_param
  );

  esp_rmaker_node_add_device(
    node,
    telemetry_dev
  );

  // ----------------------------------------------------------------
  // 5. Start RainMaker
  // ----------------------------------------------------------------
  esp_err_t err = esp_rmaker_start();

  Serial.printf(
    "esp_rmaker_start() returned %d\n",
    err
  );

  // ----------------------------------------------------------------
  // 6. Wi-Fi event handling
  // ----------------------------------------------------------------
  WiFi.onEvent(
    [](WiFiEvent_t event, WiFiEventInfo_t info) {
      switch (event) {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
          Serial.println("Wi-Fi STA connected");
          break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
          Serial.print("IP address: ");
          Serial.println(WiFi.localIP());
          wifi_connected = true;
          break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
          Serial.println("Wi-Fi STA disconnected");
          wifi_connected = false;
          break;

        default:
          break;
      }
    }
  );

  // ----------------------------------------------------------------
  // 7. Start provisioning
  //
  // The default method here is BLE provisioning.
  // ----------------------------------------------------------------
  Serial.println("Starting BLE provisioning");
  Serial.printf(
    "Service name: %s\n",
    PROV_SERVICE_NAME
  );

  Serial.printf(
    "PoP:          %s\n",
    PROV_POP
  );

  WiFiProv.beginProvision(
    WIFI_PROV_SCHEME_BLE,
    WIFI_PROV_SCHEME_HANDLER_NONE,
    WIFI_PROV_SECURITY_1,
    PROV_POP,
    PROV_SERVICE_NAME,
    nullptr
  );

  // Initialize the Wi-Fi stack
  wifiLowLevelInit(false);
}

// ------------------------------------------------------------------
// Main loop
// ------------------------------------------------------------------
void loop() {
  static uint32_t last_telemetry = 0;

  if (
    millis() - last_telemetry >=
    TELEMETRY_INTERVAL_MS
  ) {
    last_telemetry = millis();
    reportTelemetry();
  }

  handleButton();

  delay(10);
}
```

## 6. Code Explanation

### 6.1 Included Libraries

```cpp
#include <WiFi.h>
#include <WiFiProv.h>
#include <RMaker.h>
```

- `WiFi.h`: Provides the Wi-Fi stack.
- `WiFiProv.h`: Provides ESP32 unified provisioning.
- `RMaker.h`: Provides the ESP RainMaker API.

### 6.2 Original Topics

```cpp
const char* TOPIC_STATUS = "ooikk/feeds/status";
```

These topics are kept for reference only.

ESP RainMaker does not use them directly. Instead, RainMaker uses devices and parameters.

### 6.3 RainMaker Parameter Names

```cpp
const char* PARAM_STATUS = "status";
const char* PARAM_TEMP   = "temperature";
const char* PARAM_RSSI   = "rssi";
const char* PARAM_UPTIME = "uptime";
```

These names appear inside the RainMaker devices.

For LED control, use the standard RainMaker parameter name:

```cpp
const char* PARAM_LED_CONTROL = "power";
```

This helps the ESP RainMaker application display the LED as a switch or toggle.

The additional state parameter is:

```cpp
const char* PARAM_LED_STATE = "led-state";
```

This corresponds conceptually to the original `TOPIC_LED_STATE`.

### 6.4 Provisioning Settings

```cpp
const char* PROV_SERVICE_NAME = "PROV_OOIKK";
const char* PROV_POP = "abcd1234";
```

When using the ESP RainMaker application:

- The phone discovers `PROV_OOIKK`.
- The application may request the Proof of Possession code.
- Enter:

  ```text
  abcd1234
  ```

The application then sends the Wi-Fi credentials to the ESP32-S3.

For production devices:

- Do not use a fixed public Proof of Possession code.
- Use a unique PoP for each device.
- Use ESP RainMaker claiming for production deployments.

### 6.5 RainMaker Node

```cpp
esp_rmaker_node_t* node =
  esp_rmaker_node_create(
    "OOIKK ESP32-S3",
    "esp.node",
    "1.0"
  );
```

A node represents the physical ESP32-S3 device.

The node can contain multiple devices:

```text
Node
 ├─ LED
 ├─ Temperature
 └─ Telemetry
```

### 6.6 LED Device

```cpp
esp_rmaker_device_t* led_dev =
  esp_rmaker_device_create(
    "LED",
    "esp.device.switch",
    nullptr
  );
```

This creates a standard switch device.

The device contains:

```cpp
led_power_param
```

This parameter can be written by the RainMaker application.

```cpp
led_state_param
```

This parameter reports the current LED state.

The following line assigns the write callback:

```cpp
esp_rmaker_device_assign_write_cb(
  led_dev,
  led_write_cb
);
```

When the application changes the LED parameter, RainMaker calls `led_write_cb()`.

This replaces subscribing to:

```cpp
TOPIC_LED_SET
```

### 6.7 LED Write Callback

When the application sends a new LED state:

```cpp
led_state = val->val.b;

digitalWrite(
  LED_PIN,
  led_state ? HIGH : LOW
);
```

The device then reports the state back:

```cpp
esp_rmaker_param_update_and_report(
  led_state_param,
  esp_rmaker_bool(led_state)
);
```

This is conceptually equivalent to publishing to:

```cpp
TOPIC_LED_STATE
```

### 6.8 Temperature Device

```cpp
esp_rmaker_device_t* temp_dev =
  esp_rmaker_device_create(
    "Temperature",
    "esp.device.temperature-sensor",
    nullptr
  );
```

This uses a standard RainMaker temperature-sensor device type.

The temperature parameter is created with:

```cpp
temp_param =
  esp_rmaker_param_create(
    PARAM_TEMP,
    "esp.param.temperature",
    esp_rmaker_float(0.0f),
    PROP_FLAG_READ
  );
```

This replaces:

```cpp
TOPIC_TEMP
```

The example uses:

```cpp
float temp_c = temperatureRead();
```

This reads the ESP32-S3 internal temperature sensor.

> **Important:** The internal temperature sensor is approximate. It can be affected by Wi-Fi activity and chip self-heating. For accurate measurements, use an external sensor such as a DS18B20, BME280, SHT31, or SHTC3.

### 6.9 Telemetry Device

```cpp
esp_rmaker_device_t* telemetry_dev =
  esp_rmaker_device_create(
    "Telemetry",
    "custom.device.telemetry",
    nullptr
  );
```

This custom device contains:

```text
status
rssi
uptime
button
```

These parameters replace:

```cpp
TOPIC_STATUS
TOPIC_RSSI
TOPIC_UPTIME
TOPIC_BUTTON
```

### 6.10 Reporting Telemetry

The main RainMaker reporting function is:

```cpp
esp_rmaker_param_update_and_report(
  parameter,
  value
);
```

Examples:

```cpp
esp_rmaker_param_update_and_report(
  temp_param,
  esp_rmaker_float(temp_c)
);

esp_rmaker_param_update_and_report(
  rssi_param,
  esp_rmaker_int(rssi)
);

esp_rmaker_param_update_and_report(
  uptime_param,
  esp_rmaker_int(uptime_seconds)
);
```

This replaces MQTT publishing such as:

```cpp
mqtt.publish(TOPIC_TEMP, payload);
mqtt.publish(TOPIC_RSSI, payload);
mqtt.publish(TOPIC_UPTIME, payload);
```

### 6.11 Starting RainMaker

```cpp
esp_rmaker_start();
```

This starts the RainMaker core, MQTT handling, cloud-association logic, and related background services.

### 6.12 Starting Provisioning

The example uses BLE provisioning:

```cpp
WiFiProv.beginProvision(
  WIFI_PROV_SCHEME_BLE,
  WIFI_PROV_SCHEME_HANDLER_NONE,
  WIFI_PROV_SECURITY_1,
  PROV_POP,
  PROV_SERVICE_NAME,
  nullptr
);
```

The Wi-Fi stack is then initialized with:

```cpp
wifiLowLevelInit(false);
```

## 7. Uploading and Provisioning

### 7.1 Upload the Sketch

1. Connect the ESP32-S3 to the computer.
2. Select the correct COM port.
3. Upload the sketch.

If the board does not enter upload mode automatically:

1. Hold the BOOT button.
2. Press and release RESET.
3. Release the BOOT button.
4. Upload the sketch again.

### 7.2 Open the Serial Monitor

Set the baud rate to:

```text
115200
```

You should see output similar to:

```text
ESP32-S3 ESP RainMaker telemetry example
esp_rmaker_start() returned 0
Starting BLE provisioning
Service name: PROV_OOIKK
PoP:          abcd1234
```

### 7.3 Add the Device in the ESP RainMaker App

1. Open the ESP RainMaker application.
2. Tap **Add Device**.
3. Allow the application to scan for BLE devices.
4. Select:

   ```text
   PROV_OOIKK
   ```

5. Enter the Proof of Possession code if requested:

   ```text
   abcd1234
   ```

6. Select the local Wi-Fi network.
7. Enter the Wi-Fi password.
8. Wait for provisioning to complete.

> **Important:** The ESP32-S3 supports 2.4 GHz Wi-Fi. A 5 GHz-only network will not work.

## 8. Expected Output

After provisioning, the Serial Monitor should eventually show:

```text
Wi-Fi STA connected
IP address: 192.168.x.x
```

Telemetry should then appear every 10 seconds:

```text
Telemetry: temp=31.20 C, RSSI=-58 dBm, uptime=10 s
Telemetry: temp=31.40 C, RSSI=-60 dBm, uptime=20 s
```

In the ESP RainMaker application, you should see:

- An LED device.
- A Temperature device.
- A Telemetry device, depending on the application's support for custom parameters.

When the LED is toggled in the application:

```text
LED set to ON
LED set to OFF
```

When the BOOT button is pressed:

```text
Button pressed
Button released
```

## 9. Custom Parameter Notes

RainMaker works best with standard device types and standard parameters.

The LED and temperature devices use standard device types:

```text
esp.device.switch
esp.device.temperature-sensor
```

These are more likely to display correctly in the ESP RainMaker application.

The telemetry device uses:

```text
custom.device.telemetry
```

Custom devices and parameters are valid, but the official mobile application may not render them with specialized widgets.

To visualize custom values such as `status`, `rssi`, `uptime`, and `button`, you may need:

- A custom mobile or web dashboard.
- RainMaker cloud APIs.
- AWS IoT rules.
- A backend bridge.
- A separate MQTT dashboard.

## 10. Publishing to the Original Topics

If your backend or dashboard requires these exact MQTT topics:

```cpp
ooikk/feeds/status
ooikk/feeds/temperature
ooikk/feeds/rssi
ooikk/feeds/uptime
ooikk/feeds/led-control
ooikk/feeds/led-state
ooikk/feeds/button
```

then you are probably using an Adafruit IO-style MQTT broker rather than native ESP RainMaker.

In that case, use a regular MQTT client such as:

```cpp
#include <PubSubClient.h>
```

Publish values with:

```cpp
mqtt.publish(
  TOPIC_STATUS,
  "online"
);

mqtt.publish(
  TOPIC_TEMP,
  String(temp_c).c_str()
);

mqtt.publish(
  TOPIC_RSSI,
  String(WiFi.RSSI()).c_str()
);

mqtt.publish(
  TOPIC_UPTIME,
  String(millis() / 1000).c_str()
);

mqtt.publish(
  TOPIC_LED_STATE,
  led_state ? "1" : "0"
);

mqtt.publish(
  TOPIC_BUTTON,
  "pressed"
);
```

Subscribe to the LED-control topic:

```cpp
mqtt.subscribe(TOPIC_LED_SET);
```

This is a standard MQTT solution, not a native ESP RainMaker solution.

It is possible to run both systems, but doing so requires additional RAM, flash, code, and connection management.

## 11. Optional Improvements

### 11.1 Use SoftAP Provisioning

If BLE provisioning is unstable, use SoftAP provisioning instead:

```cpp
WiFiProv.beginProvision(
  WIFI_PROV_SCHEME_SOFTAP,
  WIFI_PROV_SCHEME_HANDLER_NONE,
  WIFI_PROV_SECURITY_1,
  PROV_POP,
  PROV_SERVICE_NAME,
  "your_ap_password"
);
```

The ESP32-S3 will create an access point named:

```text
PROV_OOIKK
```

The phone application can connect to this access point and provision the Wi-Fi credentials.

### 11.2 Use an External Temperature Sensor

Replace:

```cpp
float temp_c = temperatureRead();
```

with an external sensor reading, for example:

```cpp
float temp_c = bme.readTemperature();
```

or:

```cpp
float temp_c = ds18b20.getTempC();
```

### 11.3 Enable OTA

To enable OTA updates:

1. Select an OTA-capable partition scheme.
2. Enable RainMaker OTA before calling `esp_rmaker_start()`.

Conceptually:

```cpp
esp_rmaker_ota_enable(OTA_USING_TOPICS);
```

Check the ESP RainMaker examples installed with your library because the exact OTA API and partition requirements can vary by library version.

### 11.4 Enable Schedules and Scenes

RainMaker supports scheduling and scenes. Enable the required RainMaker services before starting RainMaker.

The exact API may depend on the installed library version, so consult the ESP RainMaker examples included with the library.

## 12. Troubleshooting

### `RMaker.h: No such file or directory`

Install **ESP RainMaker by Espressif Systems** through the Arduino Library Manager and restart Arduino IDE.

### Partition-Size Compilation Error

Select a larger partition scheme:

```text
Tools → Partition Scheme → Huge APP
```

Alternatively, choose a larger 8 MB scheme if one is available for the board.

### ESP32-S3 Does Not Appear as a COM Port

Try the following:

- Use a USB data cable rather than a charge-only cable.
- Install the required USB-UART driver.
- Hold BOOT while pressing RESET to enter download mode.
- Try a different USB port.

### Serial Monitor Shows Nothing

Check that:

```text
Baud rate: 115200
USB CDC On Boot: Enabled
```

Press RESET after opening the Serial Monitor.

### The Application Cannot Find `PROV_OOIKK`

Try the following:

- Enable Bluetooth on the phone.
- Grant nearby-device or location permissions if requested.
- Move the phone closer to the ESP32-S3.
- Restart the ESP32-S3.
- Retry provisioning.
- Use SoftAP provisioning as an alternative.

### Wi-Fi Provisioning Fails

Check the following:

- Use a 2.4 GHz network.
- Verify the SSID and password.
- Avoid captive-portal, hotel, campus, or enterprise networks.
- Move the ESP32-S3 closer to the router.
- Restart both the phone and ESP32-S3.

### The LED Does Not Toggle

Check:

```cpp
const int LED_PIN = 2;
```

Make sure this matches the actual LED GPIO.

If the board uses an addressable RGB LED, `digitalWrite()` will not work. Use `Adafruit_NeoPixel` or `FastLED`.

### The Temperature Is Inaccurate

The internal ESP32-S3 temperature sensor is not intended for precision measurements.

For real telemetry, use an external sensor such as:

```text
DS18B20
BME280
BMP280
SHT31
SHTC3
```

### Custom Parameters Do Not Display Correctly

Custom devices and parameters are valid, but the official application may not provide specialized widgets for them.

Possible solutions include:

- Use standard RainMaker devices wherever possible.
- Build a custom dashboard using RainMaker cloud APIs.
- Forward data to another backend using AWS IoT rules.
- Use plain MQTT and Adafruit IO if exact feed topics are required.

## 13. Summary

For ESP RainMaker, do not think in terms of raw MQTT topics such as:

```cpp
ooikk/feeds/temperature
```

Instead, use a node containing devices and parameters:

```text
Node
 ├─ Device: LED
 │   ├─ Param: power         ← replaces TOPIC_LED_SET
 │   └─ Param: led-state     ← replaces TOPIC_LED_STATE
 │
 ├─ Device: Temperature
 │   └─ Param: temperature   ← replaces TOPIC_TEMP
 │
 └─ Device: Telemetry
     ├─ Param: status        ← replaces TOPIC_STATUS
     ├─ Param: rssi          ← replaces TOPIC_RSSI
     ├─ Param: uptime        ← replaces TOPIC_UPTIME
     └─ Param: button        ← replaces TOPIC_BUTTON
```

The most important RainMaker functions are:

```cpp
esp_rmaker_node_create();
esp_rmaker_device_create();
esp_rmaker_param_create();
esp_rmaker_device_add_param();
esp_rmaker_node_add_device();
esp_rmaker_device_assign_write_cb();
esp_rmaker_start();
esp_rmaker_param_update_and_report();
```

The LED command is received in:

```cpp
led_write_cb();
```

Telemetry is reported with:

```cpp
esp_rmaker_param_update_and_report();
```

# ESP RainMaker Standard Device Types

In ESP RainMaker, standard device types are predefined in the SDK files:

```text
esp_rmaker_standard_types.h
esp_rmaker_standard_devices.h
```

They are also exposed in the Arduino library through wrapper classes.

ESP RainMaker provides five standard device types.

## Standard Device Types Overview

| # | Device | Type String | C Helper Function | Arduino Class |
|---:|---|---|---|---|
| 1 | Switch | `esp.device.switch` | `esp_rmaker_switch_device_create()` | `Switch` |
| 2 | Outlet | `esp.device.outlet` | `esp_rmaker_outlet_device_create()` | `Outlet` |
| 3 | Light Bulb | `esp.device.lightbulb` | `esp_rmaker_lightbulb_device_create()` | `LightBulb` |
| 4 | Fan | `esp.device.fan` | `esp_rmaker_fan_device_create()` | `Fan` |
| 5 | Temperature Sensor | `esp.device.temperature-sensor` | `esp_rmaker_temperature_sensor_device_create()` | `TemperatureSensor` |

## Detailed Descriptions

### 1. Switch — `esp.device.switch`

A switch is a generic binary ON/OFF control device.

It is commonly used for:

- Relays.
- Contactors.
- LEDs.
- Appliances.
- Any device that simply turns ON or OFF.

#### Standard Parameter

```text
power
```

Parameter type:

```text
esp.param.power
```

Data type and access:

```text
Boolean
Read/write
```

#### Application UI

The RainMaker application normally renders a switch as a toggle control.

#### Arduino Example

```cpp
Switch my_switch("LED Control");
```

## 2. Outlet — `esp.device.outlet`

An outlet represents a smart power plug or wall socket.

It is functionally similar to a switch but semantically represents a socket supplying power to a connected appliance.

#### Standard Parameter

```text
power
```

Parameter type:

```text
esp.param.power
```

Data type and access:

```text
Boolean
Read/write
```

#### Application UI

The application usually displays an outlet or smart-plug icon.

#### Arduino Example

```cpp
Outlet my_outlet("Living Room Outlet");
```

## 3. Light Bulb — `esp.device.lightbulb`

A light-bulb device represents a lighting product.

In addition to ON/OFF control, it can optionally support dimming and color control, depending on the parameters added.

#### Standard Parameters

| Parameter | Type | Function |
|---|---|---|
| `power` | `esp.param.power` | Turns the light ON or OFF |
| `brightness` | `esp.param.brightness` | Controls brightness |
| `hue` | `esp.param.hue` | Controls the color hue |
| `saturation` | `esp.param.saturation` | Controls color saturation |
| `intensity` | `esp.param.intensity` | Controls light intensity |
| `cct` | `esp.param.cct` | Controls correlated color temperature |

#### Application UI

The RainMaker application normally displays a bulb icon.

Adding color parameters may enable:

- A color wheel.
- Brightness sliders.
- Color-temperature controls.

#### Arduino Example

```cpp
LightBulb my_light("Living Room Light");
```

## 4. Fan — `esp.device.fan`

A fan device represents a motorized fan with ON/OFF control and optional speed regulation.

#### Standard Parameters

| Parameter | Type | Function |
|---|---|---|
| `power` | `esp.param.power` | Turns the fan ON or OFF |
| `speed` | `esp.param.speed` | Controls fan speed |

The `speed` parameter is generally an integer and can represent multiple fan-speed levels.

#### Application UI

The RainMaker application normally displays:

- A fan icon.
- A toggle control.
- A speed slider if the `speed` parameter is available.

#### Arduino Example

```cpp
Fan my_fan("Ceiling Fan");
```

## 5. Temperature Sensor — `esp.device.temperature-sensor`

A temperature sensor is a read-only device that reports temperature measurements.

This is the device type used for the temperature device in the ESP32-S3 RainMaker sketch.

#### Standard Parameter

```text
temperature
```

Parameter type:

```text
esp.param.temperature
```

Data type and access:

```text
Floating-point value
Read-only
```

The value is normally expressed in degrees Celsius.

#### Application UI

The RainMaker application may display:

- A thermometer icon.
- The current temperature.
- A historical time-series chart.

#### Arduino Example

```cpp
TemperatureSensor my_temp_sensor("Room Temperature");
```

## Standard Device Creation Methods

In the Arduino ESP RainMaker library, devices can be created with wrapper classes:

```cpp
Switch my_switch("LED Control");

Outlet my_outlet("Smart Outlet");

LightBulb my_light("Living Room Light");

Fan my_fan("Ceiling Fan");

TemperatureSensor my_temp("Room Temperature");
```

Alternatively, devices can be created generically using a device type string:

```cpp
Device my_switch(
  "LED Control",
  "esp.device.switch"
);
```

The C API can also be used:

```cpp
esp_rmaker_device_t* device =
  esp_rmaker_device_create(
    "LED Control",
    "esp.device.switch",
    nullptr
  );
```

## Important Notes

### Standard Types Provide Dedicated UI

These five standard device types have dedicated creation helpers and polished application UI widgets:

```text
Switch
Outlet
LightBulb
Fan
TemperatureSensor
```

Other devices can still be created, but they are generally custom device types.

For example:

```text
custom.device.telemetry
```

can be used for:

- Status.
- RSSI.
- Uptime.
- Button events.

Custom devices are fully supported by the RainMaker cloud, but the official applications may render them generically as parameter lists rather than providing dedicated icons and specialized widgets.

### Device Behavior Comes from Parameters

The device type string mainly tells the RainMaker application how to represent the device.

The actual behavior is determined by its parameters.

For example, you could add a temperature parameter to a switch device:

```cpp
Device custom_device(
  "Combined Device",
  "esp.device.switch"
);

Param temperature_param(
  "Temperature",
  "esp.param.temperature",
  value_f(25.0f),
  PROP_FLAG_READ
);

custom_device.addParam(temperature_param);
```

Although this is technically possible, using semantically appropriate device types usually produces a clearer application interface.

### Standard UI Types

Standard UI types can be combined with standard parameters to create application controls such as:

```text
esp.ui.toggle
esp.ui.slider
esp.ui.dropdown
esp.ui.text
esp.ui.trigger
```

Examples:

- `esp.ui.toggle`: ON/OFF switch.
- `esp.ui.slider`: Adjustable numeric value.
- `esp.ui.dropdown`: Selection menu.
- `esp.ui.text`: Text or status display.
- `esp.ui.trigger`: Momentary action or event control.

## Arduino and C API Options

You can use either the Arduino wrapper classes:

```cpp
Switch
Outlet
LightBulb
Fan
TemperatureSensor
```

or create devices generically with:

```cpp
Device(
  name,
  "esp.device.xxx"
);
```

The lower-level C API uses:

```cpp
esp_rmaker_device_create();
```

The wrapper classes are generally easier to read, while the generic `Device` class and C API provide more flexibility for custom device structures.

## Summary

The five standard ESP RainMaker device types are:

```text
1. Switch
2. Outlet
3. Light Bulb
4. Fan
5. Temperature Sensor
```

Their type strings are:

```text
esp.device.switch
esp.device.outlet
esp.device.lightbulb
esp.device.fan
esp.device.temperature-sensor
```

Anything else, such as a telemetry device containing status, RSSI, uptime, and button parameters, is a custom device type.

Custom devices still work with the RainMaker cloud, but the official applications generally display them with generic rendering instead of dedicated icons and specialized widgets.

# C API to Generic Wrapper Mapping

The following table maps the raw ESP RainMaker C API functions to their equivalent generic Arduino wrapper methods.

## API Mapping

| Concept | Raw C API | Generic Wrapper |
|---|---|---|
| Create node | `esp_rmaker_node_create()` | `Node.setNodeInfo()` |
| Create device | `esp_rmaker_device_create()` | `Device led("LED", "esp.device.switch")` |
| Create parameter | `esp_rmaker_param_create()` | `Param p(name, type, val, flags)` |
| Attach parameter | `esp_rmaker_device_add_param()` | `device.addParam(p)` |
| Attach device to node | `esp_rmaker_node_add_device()` | `Node.addDevice(d)` |
| Register write callback | `esp_rmaker_device_assign_write_cb()` | `device.addCb(cb)` |
| Report a value | `esp_rmaker_param_update_and_report(handle, val)` | `device.updateAndReportParam(&p, val)` |
| Start the RainMaker stack | `esp_rmaker_start()` | `RMaker.start()` |

## Notes and Important Details

### Same Names, Different Types

The same identifiers can be used in both branches of a dual-mode sketch:

```cpp
status_param
temp_param
rssi_param
uptime_param
```

In wrapper mode, these are `Param` objects:

```cpp
Param status_param;
Param temp_param;
```

In C API mode, they are pointers or handles:

```cpp
esp_rmaker_param_t* status_param;
esp_rmaker_param_t* temp_param;
```

The `#ifdef` blocks make the two implementations mutually exclusive, allowing the remaining code to use consistent names.

Example:

```cpp
#ifdef USE_WRAPPER_API
  Param* temp_param;
#else
  esp_rmaker_param_t* temp_param;
#endif
```

### Callback Prototypes Differ

The callback signatures differ between the two API styles.

#### Wrapper-Style Callback

The wrapper API passes `Device*` and `Param*` objects:

```cpp
void led_write_cb(
  Device* device,
  Param* param,
  const param_val_t val,
  void* priv_data,
  write_ctx_t* ctx
) {
  bool state = val.val.b;
  digitalWrite(LED_PIN, state ? HIGH : LOW);

  param->updateAndReport(val);
}
```

#### C-Style Callback

The C API passes device and parameter handles:

```cpp
esp_err_t led_write_cb(
  device_handle_t device,
  param_handle_t param,
  const param_val_t* val,
  void* priv_data,
  write_ctx_t* ctx
) {
  bool state = val->val.b;
  digitalWrite(LED_PIN, state ? HIGH : LOW);

  esp_rmaker_param_update_and_report(
    param,
    esp_rmaker_bool(state)
  );

  return ESP_OK;
}
```

This is why a dual-mode sketch requires two callback implementations.

### Keep `Param` and `Device` Objects Alive

`Device` and `Param` objects should normally be declared as global or static objects:

```cpp
static Device led_device(
  "LED",
  "esp.device.switch"
);

static Param status_param(
  "Status",
  "custom.param.status",
  value_s("boot"),
  PROP_FLAG_READ
);
```

Their internal handles must remain valid for the lifetime of the program.

Avoid creating long-lived RainMaker objects as local variables inside `setup()`:

```cpp
void setup() {
  Device temporary_device(
    "LED",
    "esp.device.switch"
  );

  // The object is destroyed when setup() returns.
}
```

A local object may be destroyed when `setup()` finishes, leaving invalid references or handles.

### `updateAndReportParam()` Overloads

The generic wrapper provides overloaded versions of `updateAndReportParam()` for common data types, including:

```cpp
bool
int
float
const char*
```

Examples:

```cpp
led_device.updateAndReportParam(
  "Power",
  true
);

telemetry_device.updateAndReportParam(
  "RSSI",
  -65
);

temperature_device.updateAndReportParam(
  "Temperature",
  25.4f
);

telemetry_device.updateAndReportParam(
  "Status",
  "online"
);
```

This is why the same call style can be used for:

- LED state.
- RSSI.
- Temperature.
- Uptime.
- Status strings.

### Specialized Classes Are Wrappers

The specialized classes are convenience subclasses of `Device`.

Examples include:

```cpp
Switch
Outlet
LightBulb
Fan
TemperatureSensor
```

They pre-create standard device parameters.

For example:

```cpp
Switch my_switch(
  "LED Control"
);
```

is a convenient wrapper around a standard switch device with a standard `Power` parameter.

The generic form gives more control:

```cpp
Device my_device(
  "LED Control",
  "esp.device.switch"
);

Param power_param(
  "Power",
  "esp.param.power",
  value_b(false),
  PROP_FLAG_READ | PROP_FLAG_WRITE
);

my_device.addParam(power_param);
```

The generic `Device` and `Param` form maps more directly to the underlying C API.

### C-Style Callback Fallback

If the installed library version does not accept the wrapper-style callback typedef with `addCb()`, register a C-style callback directly through the device handle:

```cpp
esp_rmaker_device_assign_write_cb(
  led_device.getDeviceHandle(),
  c_style_cb
);
```

Example:

```cpp
esp_err_t c_style_cb(
  device_handle_t device,
  param_handle_t param,
  const param_val_t* val,
  void* priv_data,
  write_ctx_t* ctx
) {
  if (!val) {
    return ESP_ERR_INVALID_ARG;
  }

  bool state = val->val.b;

  digitalWrite(
    LED_PIN,
    state ? HIGH : LOW
  );

  esp_rmaker_param_update_and_report(
    param,
    esp_rmaker_bool(state)
  );

  return ESP_OK;
}
```

### The Cloud Model Is Identical

The wrapper API and the C API create the same RainMaker cloud model.

You can upload the sketch twice:

1. With the mode-selection macro commented out.
2. With the mode-selection macro uncommented.

For example:

```cpp
//#define USE_C_API
```

or:

```cpp
#define USE_C_API
```

The Serial Monitor can display which mode is active:

```cpp
#ifdef USE_C_API
  Serial.println("Running in C API mode");
#else
  Serial.println("Running in generic wrapper mode");
#endif
```

Regardless of which API branch is compiled, the RainMaker application and web dashboard should show the same logical:

- Node.
- Devices.
- Parameters.
- Controls.
- Telemetry values.

## Example Dual-Mode Structure

```cpp
//#define USE_C_API

#ifdef USE_C_API

  esp_rmaker_param_t* temp_param;
  esp_rmaker_device_t* temp_device;

#else

  Param temp_param(
    "Temperature",
    "esp.param.temperature",
    value_f(0.0f),
    PROP_FLAG_READ
  );

  Device temp_device(
    "Temperature",
    "esp.device.temperature-sensor"
  );

#endif
```

The preprocessor includes only one implementation during compilation.

```text
USE_C_API disabled
    └── Generic Arduino wrapper API

USE_C_API enabled
    └── Raw ESP RainMaker C API
```

## Summary

The generic Arduino wrapper is a higher-level interface over the same RainMaker concepts exposed by the C API.

The corresponding operations are:

```text
Create node       → Node / esp_rmaker_node_create()
Create device     → Device / esp_rmaker_device_create()
Create parameter  → Param / esp_rmaker_param_create()
Attach parameter  → addParam() / esp_rmaker_device_add_param()
Attach device     → addDevice() / esp_rmaker_node_add_device()
Add callback      → addCb() / esp_rmaker_device_assign_write_cb()
Report value      → updateAndReportParam() /
                    esp_rmaker_param_update_and_report()
Start stack       → RMaker.start() / esp_rmaker_start()
```

Use the wrapper classes for simpler Arduino code. Use the generic `Device` and `Param` classes when you need more control while still preferring an object-oriented API. Use the raw C API when you need exact control over RainMaker handles, callback signatures, and low-level integration.\

---

# ESP RainMaker Web Dashboard

Setting up the ESP RainMaker Web Dashboard allows you to monitor your ESP32-S3 from a desktop browser, view historical charts, and build custom control panels.

The Web Dashboard and Mobile App use the same cloud backend, so you do not need to provision the device again. Since the `OOIKK ESP32-S3` node was already provisioned using the phone application, it should automatically appear on the web dashboard.

This guide explains how to configure and customize the dashboard using the C++ code described earlier.

## Step 1: Log In to the RainMaker Web Dashboard

1. Open a web browser and visit:

   [ESP RainMaker Web Dashboard](https://dashboard.rainmaker.espressif.com/login)

2. Click **Sign In**.
3. Use the same email address and password used to create the account in the ESP RainMaker mobile application.
4. After signing in, you will see the main dashboard overview.

## Step 2: Locate the Node

1. In the left-hand navigation menu, click **Nodes**.
2. Alternatively, look for the node in the main dashboard grid.
3. Locate:

   ```text
   OOIKK ESP32-S3
   ```

4. The status indicator should show **Online** if the ESP32-S3 is powered on and connected to Wi-Fi.
5. Click the `OOIKK ESP32-S3` node to expand it and view the devices created in the Arduino code.

## Step 3: Explore the Auto-Generated User Interface

Because the C++ code uses standard Espressif device types, the Web Dashboard automatically generates native UI widgets for those devices.

### 3.1 LED Device

Device type:

```text
esp.device.switch
```

#### What You Should See

You should see:

- A toggle-switch widget.
- Possibly a secondary state indicator.

#### Code Mapping

The toggle switch controls the `power` parameter:

```text
PARAM_LED_CONTROL
```

The secondary indicator shows the `led-state` parameter:

```text
PARAM_LED_STATE
```

#### Test Action

Click the toggle switch on the Web Dashboard.

Within approximately 1–2 seconds:

- The physical ESP32-S3 LED should turn on or off.
- The Serial Monitor should display:

  ```text
  LED set to ON
  ```

  or:

  ```text
  LED set to OFF
  ```

### 3.2 Temperature Device

Device type:

```text
esp.device.temperature-sensor
```

#### What You Should See

The dashboard may display:

- A thermometer icon.
- The current temperature.
- A time-series chart showing recent readings.

#### Code Mapping

 Maps to the `temperature` parameter:

```text
PARAM_TEMP
```

#### Test Action

The temperature should update every 10 seconds when the Arduino `reportTelemetry()` function runs.

## Step 4: View Custom Telemetry Data

The C++ code creates a custom device:

```text
Telemetry
```

with the device type:

```text
custom.device.telemetry
```

Because this is a *custom* device type, the default dashboard may display the values as a raw list of key-value pairs rather than specialized widgets.

Scroll to the **Telemetry** device section on the node page.

You may see values such as:

```text
status: "online"
rssi: -65
uptime: 340
button_event: "idle"
```

The values map to the following parameters:

| Displayed Parameter | RainMaker Parameter | Meaning |
|---|---|---|
| `status` | `PARAM_STATUS` | Device status, such as `"online"` |
| `rssi` | `PARAM_RSSI` | Wi-Fi signal strength in dBm |
| `uptime` | `PARAM_UPTIME` | Seconds since boot |
| `button_event` | `PARAM_BUTTON` | Button state or event, `"idle"` or `"pressed"` |

The raw list is useful for checking the data, but it may not be visually appealing. To improve the layout, create a custom dashboard.

## Step 5: Build a Custom Dashboard

The Web Dashboard allows custom layouts using drag-and-drop widgets.

### 5.1 Create the Dashboard

1. In the left-hand navigation menu, click **Dashboards**.
2. Click **Create Dashboard** or **New Dashboard**.
3. Enter a name, such as:

   ```text
   OOIKK Telemetry Monitor
   ```

4. Save the dashboard.
5. Open the new dashboard.
6. Enter **Edit Mode**.
7. Click **Add Widget**.

### 5.2 Widget A: Wi-Fi Signal Strength

Configure the widget as follows:

```text
Widget Type: Gauge or Number
Title:       Wi-Fi RSSI
Node:        OOIKK ESP32-S3
Device:      Telemetry
Parameter:   rssi
Minimum:     -100
Maximum:     0
Unit:        dBm
```

Click **Save** or **Add**.

### 5.3 Widget B: Device Uptime

Configure the widget as follows:

```text
Widget Type: Number or Text
Title:       Uptime
Node:        OOIKK ESP32-S3
Device:      Telemetry
Parameter:   uptime
Unit:        seconds
```

Click **Save** or **Add**.

### 5.4 Widget C: System Status

Configure the widget as follows:

```text
Widget Type: Text or State
Title:       System Status
Node:        OOIKK ESP32-S3
Device:      Telemetry
Parameter:   status
```

Click **Save** or **Add**.

Repeat the process to add another text widget for:

```text
button_event
```

After adding the widgets:

- Drag and drop them to rearrange the dashboard grid.
- Click **Save**.
- Exit **Edit Mode**.

## Step 6: View Historical Data and Charts

One of the most useful features of the RainMaker Web Dashboard is historical-data analysis.

1. Return to the **Nodes** view.
2. Select:

   ```text
   OOIKK ESP32-S3
   ```

3. Open the **Temperature** device.
4. Look for the **Time Series** or **Chart** view.
5. Select a time range, such as:

   ```text
   Last 1 Hour
   Last 24 Hours
   Last 7 Days
   ```

The cloud should render a line graph using the temperature values sent by the ESP32-S3.

### Optional RSSI Chart

You can also add a line-chart widget to the custom dashboard and map it to:

```text
Telemetry → rssi
```

This allows you to observe Wi-Fi signal changes over time.

## Step 7: Test the Complete System

After configuring the dashboard, test each part of the system.

### Test the Button

1. Press the physical BOOT button on the ESP32-S3.
2. Observe the **Button Event** widget.
3. It should change from:

   ```text
   idle
   ```

   to:

   ```text
   pressed
   ```

   and then:

   ```text
   released
   ```

### Test the LED

1. Click the LED toggle switch on the Web Dashboard.
2. Observe the physical LED.
3. Confirm that its state changes.
4. Check the Serial Monitor for:

   ```text
   LED set to ON
   ```

   or:

   ```text
   LED set to OFF
   ```

### Test Offline Behavior

1. Unplug the ESP32-S3.
2. Wait approximately 30–60 seconds.
3. Refresh the Web Dashboard.
4. The node status should change from:

   ```text
   Online
   ```

   to:

   ```text
   Offline
   ```

5. Plug the ESP32-S3 back in.
6. The device should reconnect automatically and update the dashboard.

## Troubleshooting

### The Node Is Missing

Check the following:

- You are logged in to the web portal.
- You are using the same account as the ESP RainMaker mobile application.
- The ESP32-S3 was provisioned successfully.
- The node is powered on and connected to Wi-Fi.

If a different account was used on the web portal, the node will not appear there.

### Custom Parameters Display `Null` or `N/A`

This usually means that the ESP32-S3 has not sent a telemetry update yet.

The sketch uses:

```cpp
const uint32_t TELEMETRY_INTERVAL_MS = 10000;
```

Wait approximately 10 seconds after boot for the first `esp_rmaker_param_update_and_report()` message to reach the cloud.

Also verify that:

- The ESP32-S3 is connected to Wi-Fi.
- `reportTelemetry()` is being called.
- The parameter handles are not `nullptr`.
- The Serial Monitor shows telemetry output.

### The Web Toggle Changes but the LED Does Not

Check the Serial Monitor.

If it displays:

```text
LED set to ON
```

but the physical LED remains off, the configured LED pin may be incorrect.

Check:

```cpp
const int LED_PIN = 2;
```

The correct pin depends on the specific ESP32-S3 development board.

Also verify whether the board uses:

- A normal digital LED.
- An active-low LED.
- An addressable RGB or NeoPixel LED.

### Charts Are Empty

Historical charts require stored data points.

Check that:

- The ESP32-S3 has remained powered on.
- The device has stayed connected to Wi-Fi.
- Telemetry has been running for several minutes.
- The selected time range contains data.
- The correct device and parameter were selected.

The amount of historical data retained may also depend on the RainMaker cloud service or account configuration.

## Dashboard Data Flow

The overall data flow is:

```text
ESP32-S3
    │
    ├── LED state
    ├── Temperature
    ├── RSSI
    ├── Uptime
    └── Button events
    │
    ▼
ESP RainMaker Cloud
    │
    ├── Mobile application
    ├── Web Dashboard
    ├── Historical charts
    └── Custom dashboard widgets
```

The Web Dashboard and mobile application use the same RainMaker cloud data. Therefore, provisioning the node once through the mobile application is sufficient for accessing it from the web dashboard.

---

# ESP RainMaker Provisioning and Wi-Fi Credentials

You can use a known Wi-Fi SSID and password to connect the ESP32-S3 to Wi-Fi.

However, for ESP RainMaker, Wi-Fi credentials alone are not enough for the normal end-to-end setup.

ESP RainMaker requires two separate things:

1. Wi-Fi network access.
2. RainMaker cloud and user-account association.

## 1. Wi-Fi Network Access

The ESP32-S3 needs your SSID and password to connect to the local router and access the internet.

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

This gives the ESP32-S3 network access, but it does not automatically register the device with ESP RainMaker.

## 2. RainMaker Cloud and User Association

The ESP32-S3 must also be securely associated with your RainMaker user account and node.

This allows:

- The device to appear in the RainMaker application.
- The application to control the device.
- Telemetry to be displayed.
- The device to use RainMaker cloud services.
- Secure communication between the device and RainMaker.

The BLE or SoftAP provisioning process, together with the RainMaker application, handles both Wi-Fi configuration and RainMaker association.

## What BLE Provisioning Does

When using the ESP RainMaker application with BLE provisioning, the application sends information such as:

```text
1. Wi-Fi SSID
2. Wi-Fi password
3. Device provisioning information
4. RainMaker claiming or association data
```

The ESP32-S3 stores this information, usually in NVS flash.

After successful provisioning, the ESP32-S3 can:

```text
1. Reconnect to the configured Wi-Fi network.
2. Authenticate with the RainMaker cloud.
3. Reconnect to the RainMaker MQTT endpoint.
4. Report telemetry.
5. Receive commands from the mobile or web application.
```

Therefore, the BLE and application step is not only for Wi-Fi login. It also securely registers the device with your RainMaker account.

## Why Hardcoded Wi-Fi Credentials Are Not Enough

You could write:

```cpp
#include <WiFi.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

void setup() {
  WiFi.begin(ssid, password);
}
```

This allows the ESP32-S3 to connect to the router.

However, for ESP RainMaker, this does not automatically provide:

```text
1. RainMaker cloud claiming.
2. Device certificates.
3. RainMaker user-node association.
4. Secure cloud authentication.
```

That association step is normally completed by the RainMaker application during provisioning.

## Wi-Fi Login versus RainMaker Association

These are two different processes.

### Wi-Fi Login

```text
SSID + password
```

This gives the ESP32-S3 access to the local network and internet.

### RainMaker Cloud Association

```text
RainMaker account + node association + secure device credentials
```

This allows the ESP32-S3 to appear in your RainMaker account and communicate securely with the RainMaker cloud.

Knowing the Wi-Fi password does not prove to RainMaker that the user owns or is authorized to manage the ESP32-S3.

## Can BLE and SoftAP Provisioning Be Skipped?

### Short Answer

For normal ESP RainMaker onboarding, not easily, unless the device has already been provisioned or has been correctly pre-provisioned and claimed.

### You Can Skip Provisioning If

You can skip BLE or SoftAP provisioning on later boots if:

- The device has already been provisioned.
- Wi-Fi credentials are stored in NVS.
- The RainMaker user-node association is stored.
- RainMaker certificates and claim data are stored.
- The flash contents have not been erased.

For a new or factory-erased ESP32-S3, you normally need a provisioning mechanism.

## What Happens After First Provisioning?

After successful first-time provisioning, you normally do not need to use BLE or the application again unless:

- You erase the flash or NVS.
- You change the Wi-Fi router or password.
- You perform a factory reset.
- You reflash the board with erase-all-flash enabled.
- You move the device to another RainMaker account.

During a normal reboot, the ESP32-S3 should automatically:

```text
1. Read the stored Wi-Fi credentials.
2. Reconnect to the Wi-Fi network.
3. Read the stored RainMaker credentials.
4. Reconnect to the RainMaker cloud.
```

## Why Not Hardcode Wi-Fi Credentials?

Hardcoding credentials can work for simple experiments, but it has several disadvantages.

### 1. Security Risk

If you put credentials directly in the source code:

```cpp
const char* ssid = "MyHomeWiFi";
const char* password = "MySecretPassword";
```

Anyone who reads the source code may see the Wi-Fi password.

The credentials may also be extracted from the compiled firmware binary.

### 2. No Secure Per-Device Onboarding

If you build 100 devices, you must decide whether to place the same Wi-Fi password into every device.

That approach is difficult to scale and creates a security risk.

### 3. Wi-Fi Changes Require Reflashing

If the Wi-Fi password changes, you would need to:

1. Modify the source code.
2. Recompile the firmware.
3. Reflash the ESP32-S3.

With RainMaker provisioning, you can usually reconfigure the device through the application.

### 4. RainMaker Association Is Missing

Even if the ESP32-S3 connects to Wi-Fi, it still needs to be associated with your RainMaker account.

Hardcoding the Wi-Fi credentials does not automatically complete this association.

## Use SoftAP Provisioning Instead of BLE

If the problem is specifically BLE, you can use SoftAP provisioning.

Replace the BLE provisioning call:

```cpp
WiFiProv.beginProvision(
  WIFI_PROV_SCHEME_BLE,
  WIFI_PROV_SCHEME_HANDLER_NONE,
  WIFI_PROV_SECURITY_1,
  PROV_POP,
  PROV_SERVICE_NAME,
  nullptr
);
```

with SoftAP provisioning:

```cpp
WiFiProv.beginProvision(
  WIFI_PROV_SCHEME_SOFTAP,
  WIFI_PROV_SCHEME_HANDLER_NONE,
  WIFI_PROV_SECURITY_1,
  PROV_POP,
  PROV_SERVICE_NAME,
  "your_ap_password"
);
```

The ESP32-S3 creates its own temporary Wi-Fi access point, for example:

```text
PROV_OOIKK
```

The RainMaker application connects to this access point and sends the actual home Wi-Fi credentials to the ESP32-S3.

This method still uses the RainMaker application, but it avoids BLE.

## Can Known Wi-Fi Credentials Be Used Without the App?

For a normal ESP RainMaker beginner setup, this is not recommended.

You would need to handle several advanced tasks yourself:

```text
1. Wi-Fi connection.
2. RainMaker claiming.
3. Device certificates.
4. MQTT endpoint configuration.
5. RainMaker user-node association.
6. Secure credential storage in NVS.
7. Reconnection logic.
8. Factory-reset handling.
9. Re-provisioning logic.
```

The RainMaker application and unified provisioning system handle these tasks for you.

## Consider Plain MQTT Instead

If your real requirement is:

```text
1. Hardcoded Wi-Fi credentials.
2. User-defined MQTT topics.
3. Direct telemetry publishing.
4. Direct command subscriptions.
5. No RainMaker mobile application.
6. No RainMaker cloud association.
7. A simple Arduino IDE workflow.
```

then plain MQTT may be a better fit than ESP RainMaker.

For example, with Adafruit IO, HiveMQ, EMQX, Mosquitto, or another MQTT broker, you can use:

```cpp
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const char* mqttServer = "your-mqtt-broker.example.com";
const int mqttPort = 1883;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
```

You can then publish to custom topics:

```cpp
mqttClient.publish(
  "ooikk/feeds/temperature",
  "25.4"
);

mqttClient.publish(
  "ooikk/feeds/status",
  "online"
);
```

and subscribe to a command topic:

```cpp
mqttClient.subscribe(
  "ooikk/feeds/led-control"
);
```

This model uses the exact user-defined topics you specify.

ESP RainMaker uses a different structure:

```text
RainMaker Node
 ├─ Device
 │   └─ Parameter
 └─ Device
     └─ Parameter
```

It does not normally expose arbitrary user-defined MQTT feed topics such as:

```text
ooikk/feeds/temperature
```

## Practical Recommendation

### If You Want to Use ESP RainMaker

Use provisioning once:

```text
1. Install the ESP RainMaker application.
2. Start BLE or SoftAP provisioning.
3. Send Wi-Fi credentials to the ESP32-S3.
4. Associate the device with your RainMaker account.
5. Confirm that the node appears in the application.
```

After that, the ESP32-S3 should reconnect automatically without using BLE or the application on every boot.

If BLE is unstable, use SoftAP provisioning:

```cpp
WiFiProv.beginProvision(
  WIFI_PROV_SCHEME_SOFTAP,
  WIFI_PROV_SCHEME_HANDLER_NONE,
  WIFI_PROV_SECURITY_1,
  PROV_POP,
  PROV_SERVICE_NAME,
  "your_ap_password"
);
```

### If You Want to Avoid the Application Completely

ESP RainMaker may not be the best fit unless you are prepared to implement:

- Device claiming.
- Manufacturing provisioning.
- Secure credential storage.
- A custom backend.
- A custom mobile or web application.
- Certificate and endpoint management.
- Factory-reset and re-provisioning support.

For this use case, plain MQTT may be simpler.

## Final Summary

You can use known Wi-Fi credentials to connect the ESP32-S3 to Wi-Fi, but ESP RainMaker requires more than network access.

The BLE or SoftAP provisioning process and the RainMaker application are used to:

- Send Wi-Fi credentials.
- Configure the device.
- Securely associate the device with your RainMaker account.
- Store credentials and cloud information.

After first-time provisioning, the ESP32-S3 can reconnect automatically without BLE or the application.

If you want to avoid BLE, use SoftAP provisioning. If you want to avoid the RainMaker application completely, use a custom production-provisioning system or choose plain MQTT instead of ESP RainMaker.


## Does Mobile have to login to the same IoT network?

No, your mobile phone does not need to be connected to the IoT Wi-Fi network.

Yes, you can and should manually enter the IoT SSID and password directly into the ESP RainMaker application during setup.

## How the Provisioning Flow Works

The ESP RainMaker application is designed for this situation. It uses your phone as a bridge to transfer the credentials to the ESP32-S3.

The phone and ESP32-S3 do not need to be connected to the same Wi-Fi network at the same time.

### 1. Phone-to-ESP32 Connection

Your phone temporarily connects to the ESP32-S3 using either:

- Bluetooth Low Energy (BLE).
- The ESP32-S3's temporary Wi-Fi access point through SoftAP provisioning.

### 2. Credential Transfer

The application asks for the target Wi-Fi details.

Manually enter:

```text
IoT SSID:      MyHouse_IOT
IoT Password:  YOUR_IOT_PASSWORD
```

The application encrypts the credentials and sends them to the ESP32-S3 through the temporary BLE or SoftAP connection.

### 3. ESP32-to-IoT-Router Connection

The ESP32-S3:

1. Receives the credentials.
2. Disconnects from the temporary phone connection.
3. Connects to the IoT Wi-Fi router.
4. Obtains internet access.
5. Connects to the ESP RainMaker cloud.

### 4. Cloud Synchronization

Your phone uses its own internet connection, such as:

- Your main home Wi-Fi.
- Mobile data.
- Another available internet connection.

The ESP32-S3 uses the IoT Wi-Fi network to communicate with the RainMaker cloud.

Both devices meet through the cloud:

```text
Phone ───────────────┐
                     ├── ESP RainMaker Cloud
ESP32-S3 ────────────┘
```

The ESP32-S3 can therefore appear in the RainMaker application even when the phone and device are connected to different Wi-Fi networks.

## Important: Avoid the Auto-Fill Trap

When you reach the Wi-Fi setup screen in the ESP RainMaker application, the application may automatically fill in the SSID of the Wi-Fi network currently used by your phone.

For example:

```text
Phone's current network: Main_Home_WiFi
Target device network:    MyHouse_IOT
```

Before continuing:

1. Delete the automatically filled SSID.
2. Enter the IoT SSID manually.
3. Enter the IoT Wi-Fi password manually.
4. Confirm that the credentials belong to the intended IoT network.

If the IoT network is hidden, look for an option such as:

```text
Enter network manually
```

Then type the hidden SSID instead of selecting a network from the scanned list.

## Four Requirements for the IoT Network

The IoT Wi-Fi network must meet the following requirements.

### 1. It Must Support 2.4 GHz Wi-Fi

The ESP32-S3 supports 2.4 GHz Wi-Fi but cannot connect to a 5 GHz-only network.

If the router broadcasts one combined SSID for both 2.4 GHz and 5 GHz, the connection may work. However, a dedicated 2.4 GHz SSID is generally more reliable.

Recommended configuration:

```text
Main Wi-Fi:  Main_Home_WiFi
IoT Wi-Fi:   MyHouse_IOT_2G
```

### 2. It Must Provide Internet Access

The IoT network cannot be a completely isolated local network.

The ESP32-S3 needs internet access to communicate with the ESP RainMaker cloud servers.

A local-only network may allow the ESP32-S3 to connect to the router but prevent it from completing RainMaker cloud synchronization.

### 3. It Must Not Use a Captive Portal

The ESP32-S3 cannot normally use networks that require a web-browser login, such as:

- Hotel Wi-Fi.
- Dormitory Wi-Fi.
- Coffee-shop Wi-Fi.
- Airport Wi-Fi.
- Guest networks requiring terms acceptance.
- Networks requiring a room number or access code through a web page.

The ESP32-S3 expects a standard Wi-Fi connection using a normal password.

Supported example:

```text
Security: WPA2-Personal
SSID:     MyHouse_IOT
Password: YOUR_PASSWORD
```

### 4. It Must Not Require WPA-Enterprise

Standard WPA2-Personal or WPA3-Personal authentication normally uses one password and is suitable for this setup.

WPA-Enterprise commonly requires:

- A username.
- A password.
- Certificates.
- An authentication server.

It is often used by universities and large corporations and is not supported by standard ESP RainMaker provisioning out of the box.

## Recommended Setup

Use the following arrangement:

```text
Phone:
  Connected to main home Wi-Fi or mobile data

ESP RainMaker app:
  Connected to the ESP32-S3 through BLE or SoftAP during provisioning

ESP32-S3:
  Configured with the IoT SSID and password

IoT router:
  Connected to the internet
```

During setup:

1. Keep your phone connected to your normal home Wi-Fi or mobile data.
2. Open the ESP RainMaker application.
3. Use BLE or SoftAP to find the ESP32-S3.
4. When prompted for Wi-Fi details, delete any automatically filled SSID.
5. Manually enter the IoT SSID.
6. Enter the IoT Wi-Fi password.
7. Complete provisioning.
8. Wait for the ESP32-S3 to connect to the IoT router and RainMaker cloud.

## Summary

Your phone does not need to be connected to the IoT Wi-Fi network.

Keep the phone connected to your main Wi-Fi or use mobile data. Use BLE or SoftAP to connect temporarily to the ESP32-S3, then manually enter the IoT SSID and password in the RainMaker application.

The ESP32-S3 will use those credentials to connect to the IoT router and communicate with the RainMaker cloud.

---

# ESP RainMaker Overview (Gemini)

ESP RainMaker is Espressif’s end-to-end IoT platform designed to deploy, manage, and control ESP32-based products. Unlike standard MQTT setups that require manual topic definitions, cloud broker provisioning, and custom dashboard development, RainMaker uses dynamic metadata models (nodes, devices, parameters) where the ESP32 automatically describes its capabilities to the cloud.

## Key Features

### Zero-Code Provisioning
Built-in Wi-Fi commissioning over Bluetooth LE (BLE) using standard iOS/Android ESP RainMaker apps.

### Dynamic UI Generation
The mobile app automatically builds controls (switches, sliders, drop-downs) based on parameters reported by the firmware.

### Local & Cloud Control
Supports local network control (via mDNS/HTTP) when internet connectivity is lost, alongside AWS IoT cloud relay for remote access.

### OTA Updates & Voice Assistants
Integrated support for remote firmware updates, Google Assistant, and Amazon Alexa.

## Key Advantages

### Native Integration
Built into the ESP-IDF and ESP32 Arduino Core; no external libraries like PubSubClient or manual TLS certificate management are required.

### No MQTT Topic Mapping
Controls are mapped to simple C/C++ callback handlers rather than parsing raw JSON strings.

### Self-Claiming (ESP32-S3)
Uses hardware security features to auto-register certificates directly with Espressif’s cloud without manual token steps.

## Limitations

### Espressif Ecosystem Lock-in
Native support is restricted exclusively to ESP32 family microcontrollers.

### Fair Usage Rate Limits
Subject to messaging limits (~25,000 messages/day per account) to prevent server abuse.

### Public Cloud Shared Infrastructure
Cloud responsiveness depends on public server loads (private enterprise deployments require self-hosted AWS instances).

## Free Account Limits

### Default Quota
Up to 5 physical nodes (ESP32-S3 devices) under Assisted Claiming on a public account.

### Self-Claiming Advantage
For ESP32-S3 devices using Self-Claiming, hardware-assisted credentials remove the strict 5-node limit for evaluation/hobbyist use.

### Quota Upgrades
Developers can request quota increases free of charge by contacting `esp-rainmaker-admin@espressif.com`.

## ESP RainMaker Architecture

In ESP RainMaker, you move away from manually defined MQTT topic strings, such as `ooikk/feeds/...`.

Instead, you create a single **Node**—your ESP32-S3—which contains individual **Devices** and **Parameters**.

When the ESP32-S3 connects to the mobile app or web dashboard, RainMaker automatically handles:

- State synchronization.
- Cloud updates.
- Device communication.
- User-interface generation for parameters.

The following section presents a standalone ESP RainMaker sketch, followed by an explanation of how its parameters map to the original MQTT feeds.

### Standalone ESP RainMaker Sketch

```cpp
#include "RMaker.h"
#include "WiFiProv.h"

// -----------------------------------------------------------------------------
// HARDWARE & TIMING CONFIG
// -----------------------------------------------------------------------------
const int LED_PIN = 2; // ESP32-S3 GPIO for onboard/external LED
const uint32_t TELEMETRY_PERIOD_MS = 10000;
uint32_t lastTelemetryMs = 0;

// BLE Provisioning Credentials (Used for first-time pairing via phone app)
const char *service_name = "PROV_ESP32S3";
const char *pop = "12345678"; // Proof of Possession / PIN

// -----------------------------------------------------------------------------
// RAINMAKER NODES & DEVICES
// -----------------------------------------------------------------------------
static Node my_node;
// FIX 1: Cast (void*)&LED_PIN to void* to prevent const void* conversion error
static Switch my_switch("LED Control", (void*)&LED_PIN);
static TemperatureSensor my_temp_sensor("Temperature Sensor");

// Generic device to hold system telemetry parameters
static Device sys_telemetry("System Stats", "custom.device.system");

// Parameter Pointers for dynamic reporting
static Param *p_status = NULL;
static Param *p_rssi   = NULL;
static Param *p_uptime = NULL;

// -----------------------------------------------------------------------------
// PROVISIONING EVENT CALLBACK
// -----------------------------------------------------------------------------
void sysProvEvent(arduino_event_t *sys_event) {
    switch (sys_event->event_id) {
        case ARDUINO_EVENT_PROV_START:
            Serial.printf("\n[PROV] Started! BLE Name: %s | POP PIN: %s\n", service_name, pop);
            break;
        case ARDUINO_EVENT_PROV_CRED_RECV:
            Serial.println("\n[PROV] Received Wi-Fi credentials from App.");
            break;
        case ARDUINO_EVENT_PROV_CRED_SUCCESS:
            Serial.println("\n[PROV] Provisioning Successful! Connected to Wi-Fi.");
            break;
        default:
            break;
    }
}

// -----------------------------------------------------------------------------
// WRITE CALLBACK (App/Cloud -> ESP32-S3)
// -----------------------------------------------------------------------------
void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx) {
    const char *device_name = device->getDeviceName();
    const char *param_name = param->getParamName();

    if (strcmp(device_name, "LED Control") == 0) {
        if (strcmp(param_name, "Power") == 0) {
            bool newState = val.val.b;
            digitalWrite(LED_PIN, newState ? HIGH : LOW);
            param->updateAndReport(val);
            Serial.printf("[RainMaker] LED toggled -> %s\n", newState ? "ON" : "OFF");
        }
    }
}

// -----------------------------------------------------------------------------
// TELEMETRY PUBLISHER
// -----------------------------------------------------------------------------
void publishTelemetry() {
    // 1. Temperature Feed
    float temperature = random(100, 500) / 10.0f;
    my_temp_sensor.updateAndReportParam("Temperature", temperature);

    // 2. RSSI Feed (Wi-Fi Signal Strength in dBm)
    // FIX 2: Use esp_rmaker_int() and esp_rmaker_str() helper functions in v3.x
    int rssi = WiFi.RSSI();
    p_rssi->updateAndReport(esp_rmaker_int(rssi));

    // 3. Uptime Feed (Seconds since boot)
    int uptime_sec = (int)(millis() / 1000);
    p_uptime->updateAndReport(esp_rmaker_int(uptime_sec));

    // 4. Status Feed
    p_status->updateAndReport(esp_rmaker_str("online"));

    Serial.printf("[Telemetry] Temp: %.1f C | RSSI: %d dBm | Uptime: %d s\n", 
                  temperature, rssi, uptime_sec);
}

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // 1. Initialize Node
    my_node = RMaker.initNode("ESP32S3_Node");

    // 2. Configure LED Switch Device
    my_switch.addCb(write_callback);
    my_node.addDevice(my_switch);

    // 3. Configure Temperature Sensor Device
    my_node.addDevice(my_temp_sensor);

    // 4. Create System Telemetry Parameters using v3.x esp_rmaker_* value helpers
    p_status = new Param("Status", "custom.param.status", esp_rmaker_str("online"), PROP_FLAG_READ);
    p_rssi   = new Param("RSSI", "custom.param.rssi", esp_rmaker_int(0), PROP_FLAG_READ);
    p_uptime = new Param("Uptime", "custom.param.uptime", esp_rmaker_int(0), PROP_FLAG_READ);

    sys_telemetry.addParam(*p_status);
    sys_telemetry.addParam(*p_rssi);
    sys_telemetry.addParam(*p_uptime);
    my_node.addDevice(sys_telemetry);

    // 5. Enable Standard Services
    RMaker.enableOTA(OTA_USING_TOPICS);
    RMaker.enableTZService();
    RMaker.enableSchedule();

    // 6. Start RainMaker Stack
    RMaker.start();

    // 7. Start BLE Provisioning using updated NETWORK_PROV_* namespaces for core 3.x
    WiFi.onEvent(sysProvEvent);
    WiFiProv.beginProvision(NETWORK_PROV_SCHEME_BLE, 
                            NETWORK_PROV_SCHEME_HANDLER_FREE_BTDM, 
                            NETWORK_PROV_SECURITY_1, 
                            pop, 
                            service_name);
}

// -----------------------------------------------------------------------------
// MAIN LOOP
// -----------------------------------------------------------------------------
void loop() {
    if (millis() - lastTelemetryMs >= TELEMETRY_PERIOD_MS) {
        lastTelemetryMs = millis();
        if (WiFi.status() == WL_CONNECTED) {
            publishTelemetry();
        }
    }
    delay(10);
}
```

### Step-by-Step Architecture

#### Step 1: Feed-Mapping Strategy

Instead of manually defining topic strings such as `TOPIC_TEMP` and `TOPIC_LED_SET`, RainMaker represents inputs and outputs through standard devices and custom parameters.

| Original MQTT Feed | RainMaker Structure | Behavior and Access |
|---|---|---|
| `TOPIC_LED_SET` and `TOPIC_LED_STATE` | `Switch my_switch("LED Control")` | Read/write. The mobile app displays a toggle button. Changing it invokes `write_callback()`, updates the hardware, and synchronizes the cloud state. |
| `TOPIC_TEMP` | `TemperatureSensor my_temp_sensor` | Read-only. Accepts floating-point values through `updateAndReportParam()` and can display them on app graphs. |
| `TOPIC_STATUS` | `Param* p_status` | Read-only string parameter returning `"online"`. |
| `TOPIC_RSSI` | `Param* p_rssi` | Read-only integer parameter reporting signal strength in dBm. |
| `TOPIC_UPTIME` | `Param* p_uptime` | Read-only integer parameter reporting seconds since boot. |

##### ESP RainMaker Device Definitions

ESP RainMaker provides two types of device definitions:

1. **Standard pre-built device classes:** Include built-in UI controls and standard parameters.
2. **Generic or custom devices:** Used to build custom device types and UI controls.

###### 1. Pre-Defined Standard Device Classes

These C++ wrapper classes automatically attach required standard parameters, such as `Power`, `Temperature`, and `Speed`. They also instruct the RainMaker mobile application to render appropriate native UI controls.

| Class Name | Constructor Example | Primary Parameter | Default Additional Parameters and Description |
|---|---|---|---|
| `Switch` | `Switch my_switch("Onboard LED");` | `Power` (`Bool`) | Simple binary ON/OFF device for relays, power sockets, LEDs, and switches. The UI renders a large toggle button. |
| `LightBulb` | `LightBulb my_light("Living Room Light");` | `Power` (`Bool`) | Smart-lighting device. It can be extended with `Brightness`, `Hue`, `Saturation`, or CCT, which represents color temperature. |
| `TemperatureSensor` | `TemperatureSensor my_temp("Room Temp");` | `Temperature` (`Float`) | Environmental sensor device. The mobile app displays a live temperature gauge or card. |
| `Fan` | `Fan my_fan("Ceiling Fan");` | `Power` (`Bool`) | Motor-control device. It can be extended with standard parameters such as `Speed` and `Direction`. |

###### 2. Standard Device Types

Under the hood, and in the underlying `esp_rmaker_standard_types.h` system, Espressif categorizes devices using string types in the form `esp.device.<type>`.

When building applications, these device types help determine the dynamic UI template selected by the RainMaker cloud.

| Device Type String | Primary Function | Typical Parameter UI Controls |
|---|---|---|
| `esp.device.switch` | General-purpose electrical switch | Toggle switch (`UI_TOGGLE`) |
| `esp.device.outlet` | Smart wall socket or power plug | Toggle switch (`UI_TOGGLE`) |
| `esp.device.lightbulb` | Dimmable or RGB light source | Toggle, sliders (`UI_SLIDER`), and hue picker |
| `esp.device.fan` | Ventilation fan or exhaust fan | Toggle switch and speed slider |
| `esp.device.temp-sensor` | Thermometer or temperature sensor node | Read-only value card or chart |
| `esp.device.thermostat` | Climate controller | Setpoint slider and Heat/Cool/Off mode selector |
| `esp.device.lock` | Smart door lock | Lock/Unlock toggle |
| `esp.device.blind` | Window blinds or motorized curtains | Open/Close control and position slider |
| `esp.device.garage-door` | Garage-door opener | Open/Close toggle state |
| `esp.device.air-conditioner` | HVAC air-conditioning unit | Power, target temperature, fan speed, and mode dropdown |

###### 3. Generic Custom Device Class

When none of the pre-defined classes match your sensor or peripheral array, create a generic device container using the base `Device` class and attach custom parameters manually.

```cpp
// Constructor:
// Device(const char *dev_name, const char *dev_type, void *priv_data);

Device my_custom_device(
  "System Stats",
  "custom.device.system"
);
```

###### Standard UI Types for Custom Parameters

You can append `Param` objects to a generic device and control how the mobile application displays them by using UI type flags.

| UI Type | Description |
|---|---|
| `ESP_RMAKER_UI_TOGGLE` | Displays a simple ON/OFF switch. |
| `ESP_RMAKER_UI_SLIDER` | Displays an interactive horizontal slider, suitable for percentages, brightness, or setpoints. |
| `ESP_RMAKER_UI_DROPDOWN` | Displays a list-selection menu for discrete operating modes. |
| `ESP_RMAKER_UI_TEXT` | Displays text strings or status information, such as `online` or `error`. |

###### Custom Telemetry Device Example

The following example creates a custom sensor-hub device with temperature and mode parameters:

```cpp
Device my_sensor_hub(
  "Sensor Hub",
  "custom.device.hub"
);

Param temp_param(
  "Temperature",
  "esp.param.temperature",
  value_f(25.5),
  PROP_FLAG_READ
);

Param mode_param(
  "Mode",
  "custom.param.mode",
  value_s("Auto"),
  PROP_FLAG_READ | PROP_FLAG_WRITE
);

mode_param.addUIType(
  ESP_RMAKER_UI_DROPDOWN
);

my_sensor_hub.addParam(temp_param);
my_sensor_hub.addParam(mode_param);

my_node.addDevice(my_sensor_hub);
```

###### Parameter Access Modes

- `PROP_FLAG_READ`: The parameter can be read by the application but cannot be changed there.
- `PROP_FLAG_WRITE`: The parameter can be modified by the application.
- `PROP_FLAG_READ | PROP_FLAG_WRITE`: The parameter can be read and changed by the application.

#### Step 2: Instantiating Devices and Parameters

RainMaker provides standard device classes, such as `Switch` and `TemperatureSensor`, as well as a generic `Device` container for custom parameters.

```cpp
// Preconfigured device types
static Switch my_switch(
  "LED Control",
  &LED_PIN
);

static TemperatureSensor my_temp_sensor(
  "Temperature Sensor"
);

// Custom container for system statistics
static Device sys_telemetry(
  "System Stats",
  "custom.device.system"
);

p_status = new Param(
  "Status",
  "custom.param.status",
  value_s("online"),
  PROP_FLAG_READ
);

p_rssi = new Param(
  "RSSI",
  "custom.param.rssi",
  value_i(0),
  PROP_FLAG_READ
);

p_uptime = new Param(
  "Uptime",
  "custom.param.uptime",
  value_i(0),
  PROP_FLAG_READ
);
```

`PROP_FLAG_READ` specifies that the parameters are read-only telemetry values sent from the ESP32-S3 to the cloud. The application cannot overwrite them.

Adding the parameters to `sys_telemetry` groups them under a single system-statistics device in the RainMaker application.

#### Step 3: Handling Direct Application Commands

In a standard MQTT application, incoming JSON strings must be parsed inside a topic callback.

In RainMaker, incoming cloud commands trigger a callback containing a typed `param_val_t` value:

```cpp
void write_callback(
  Device* device,
  Param* param,
  const param_val_t val,
  void* priv_data,
  write_ctx_t* ctx
) {
  const char* param_name = param->getParamName();

  if (strcmp(param_name, "Power") == 0) {
    bool newState = val.val.b;

    digitalWrite(
      LED_PIN,
      newState ? HIGH : LOW
    );

    // Confirm the changed state to RainMaker
    param->updateAndReport(val);
  }
}
```

`val.val.b` directly extracts the Boolean value, either `true` or `false`.

Calling `param->updateAndReport(val)` acts as the equivalent of publishing the updated value to `TOPIC_LED_STATE`.

#### Step 4: Provisioning and Initialization

```cpp
RMaker.initNode("ESP32S3_Node");

// Add devices and parameters here

RMaker.start();

WiFiProv.beginProvision(
  WIFI_PROV_SCHEME_BLE,
  // Additional provisioning arguments
  pop,
  service_name
);
```

The initialization process works as follows:

- `initNode()` initializes the Espressif cloud entity for the ESP32-S3.
- `my_node.addDevice()` attaches devices to the node.
- RainMaker builds the device structure used to generate application controls.
- If no Wi-Fi credentials are stored in non-volatile storage (NVS), `beginProvision()` starts BLE advertising.
- The device advertises under the name `PROV_ESP32S3`.
- The Proof of Possession PIN is `12345678`.
- The RainMaker application securely sends the Wi-Fi credentials over BLE.

#### Step 5: Simplified Telemetry Publishing

```cpp
my_temp_sensor.updateAndReportParam(
  "Temperature",
  temperature
);

p_rssi->updateAndReport(
  param_val_t(rssi)
);

p_uptime->updateAndReport(
  param_val_t(uptime_sec)
);

p_status->updateAndReport(
  param_val_t("online")
);
```

Calling `updateAndReport()` packages the updated value into RainMaker's internal format and sends it through the RainMaker communication system. This avoids manual string formatting, topic construction, and subscription management.

### RainMaker Mobile-App Provisioning Flow

1. Power on the ESP32-S3.
2. Open the ESP RainMaker application on iOS or Android.
3. Tap **Add Device**.
4. Select **BLE Provisioning**.
5. Allow the application to discover `PROV_ESP32S3`.
6. Enter the Proof of Possession PIN:

   ```text
   12345678
   ```

7. Select the local Wi-Fi network.
8. Enter the Wi-Fi password.
9. Wait for provisioning to complete.
10. Open the device in the RainMaker application.

After provisioning, RainMaker can generate controls for:

- LED Control.
- Temperature Sensor.
- System Stats.
- Status.
- RSSI.
- Uptime.


## Local Button and Cloud Control Synchronization

To handle local hardware input, such as a tactile push button, alongside cloud controls:

1. Monitor the button state inside the program.
2. Toggle the LED state when the button is pressed.
3. Explicitly update RainMaker with:

   ```cpp
   my_switch.updateAndReportParam("Power", newState);
   ```

This keeps the physical LED output and the mobile application UI synchronized.

### Non-Blocking Button Handling with Debouncing

The following complete sketch for ESP32 Arduino Core v3.x uses non-blocking button handling and software debouncing alongside RainMaker cloud callbacks.

```cpp
#include "RMaker.h"
#include "WiFiProv.h"

// -----------------------------------------------------------------------------
// Hardware and Timing Configuration
// -----------------------------------------------------------------------------
const int LED_PIN    = 2;
const int BUTTON_PIN = 0;

// ESP32-S3 GPIO for the LED
// ESP32-S3 GPIO for the push button, such as the BOOT button

const uint32_t TELEMETRY_PERIOD_MS = 10000;
uint32_t lastTelemetryMs = 0;

// Button state tracking and debouncing
bool currentLedState = false;
bool lastButtonReading = HIGH;

uint32_t lastDebounceTime = 0;
const uint32_t DEBOUNCE_DELAY_MS = 50;

// BLE provisioning credentials
const char* service_name = "PROV_ESP32S3";
const char* pop = "12345678";

// -----------------------------------------------------------------------------
// RainMaker Nodes and Devices
// -----------------------------------------------------------------------------
static Node my_node;

static Switch my_switch(
  "LED Control",
  (void*)&LED_PIN
);

static TemperatureSensor my_temp_sensor(
  "Temperature Sensor"
);

static Device sys_telemetry(
  "System Stats",
  "custom.device.system"
);

static Param* p_status = nullptr;
static Param* p_rssi   = nullptr;
static Param* p_uptime = nullptr;

// -----------------------------------------------------------------------------
// Provisioning Event Callback
// -----------------------------------------------------------------------------
void sysProvEvent(arduino_event_t* sys_event) {
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
      Serial.printf(
        "\n[PROV] Started! BLE Name: %s | POP PIN: %s\n",
        service_name,
        pop
      );
      break;

    case ARDUINO_EVENT_PROV_CRED_RECV:
      Serial.println(
        "\n[PROV] Received Wi-Fi credentials from the app."
      );
      break;

    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      Serial.println(
        "\n[PROV] Provisioning successful! Connected to Wi-Fi."
      );
      break;

    default:
      break;
  }
}

// -----------------------------------------------------------------------------
// Cloud Callback
// Application or Cloud -> ESP32-S3
// -----------------------------------------------------------------------------
void write_callback(
  Device* device,
  Param* param,
  const param_val_t val,
  void* priv_data,
  write_ctx_t* ctx
) {
  const char* device_name = device->getDeviceName();
  const char* param_name = param->getParamName();

  if (
    strcmp(device_name, "LED Control") == 0 &&
    strcmp(param_name, "Power") == 0
  ) {
    currentLedState = val.val.b;

    digitalWrite(
      LED_PIN,
      currentLedState ? HIGH : LOW
    );

    // Confirm the state change to the cloud
    param->updateAndReport(val);

    Serial.printf(
      "[Cloud Command] LED set to -> %s\n",
      currentLedState ? "ON" : "OFF"
    );
  }
}

// -----------------------------------------------------------------------------
// Local Hardware Button Handler
// -----------------------------------------------------------------------------
void handleHardwareButton() {
  bool reading = digitalRead(BUTTON_PIN);

  // Reset the debounce timer if the reading changes
  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if (
    millis() - lastDebounceTime >
    DEBOUNCE_DELAY_MS
  ) {
    // Retains the confirmed button state between function calls
    static bool steadyButtonState = HIGH;

    // Detect a confirmed state change
    if (reading != steadyButtonState) {
      steadyButtonState = reading;

      // Trigger the action when the active-LOW button is pressed
      if (steadyButtonState == LOW) {
        currentLedState = !currentLedState;

        digitalWrite(
          LED_PIN,
          currentLedState ? HIGH : LOW
        );

        // Report the local state change to RainMaker
        my_switch.updateAndReportParam(
          "Power",
          currentLedState
        );

        Serial.printf(
          "[Local Button] Toggled LED -> %s "
          "(Synced to App)\n",
          currentLedState ? "ON" : "OFF"
        );
      }
    }
  }

  lastButtonReading = reading;
}

// -----------------------------------------------------------------------------
// Telemetry Publisher
// -----------------------------------------------------------------------------
void publishTelemetry() {
  float temperature = random(100, 500) / 10.0f;

  my_temp_sensor.updateAndReportParam(
    "Temperature",
    temperature
  );

  int rssi = WiFi.RSSI();

  p_rssi->updateAndReport(
    esp_rmaker_int(rssi)
  );

  int uptime_sec = static_cast<int>(
    millis() / 1000
  );

  p_uptime->updateAndReport(
    esp_rmaker_int(uptime_sec)
  );

  p_status->updateAndReport(
    esp_rmaker_str("online")
  );

  Serial.printf(
    "[Telemetry] Temp: %.1f C | RSSI: %d dBm | Uptime: %d s\n",
    temperature,
    rssi,
    uptime_sec
  );
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Configure the button with the internal pull-up resistor
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // 1. Initialize the RainMaker node
  my_node = RMaker.initNode(
    "ESP32S3_Node"
  );

  // 2. Configure devices
  my_switch.addCb(write_callback);

  my_node.addDevice(my_switch);
  my_node.addDevice(my_temp_sensor);

  // 3. Configure telemetry parameters
  p_status = new Param(
    "Status",
    "custom.param.status",
    esp_rmaker_str("online"),
    PROP_FLAG_READ
  );

  p_rssi = new Param(
    "RSSI",
    "custom.param.rssi",
    esp_rmaker_int(0),
    PROP_FLAG_READ
  );

  p_uptime = new Param(
    "Uptime",
    "custom.param.uptime",
    esp_rmaker_int(0),
    PROP_FLAG_READ
  );

  sys_telemetry.addParam(*p_status);
  sys_telemetry.addParam(*p_rssi);
  sys_telemetry.addParam(*p_uptime);

  my_node.addDevice(sys_telemetry);

  // 4. Enable standard RainMaker services
  RMaker.enableOTA(OTA_USING_TOPICS);
  RMaker.enableTZService();
  RMaker.enableSchedule();

  // 5. Start RainMaker and provisioning
  RMaker.start();

  WiFi.onEvent(sysProvEvent);

  WiFiProv.beginProvision(
    NETWORK_PROV_SCHEME_BLE,
    NETWORK_PROV_SCHEME_HANDLER_FREE_BTDM,
    NETWORK_PROV_SECURITY_1,
    pop,
    service_name
  );
}

// -----------------------------------------------------------------------------
// Main Loop
// -----------------------------------------------------------------------------
void loop() {
  // 1. Continuously check the local hardware input
  handleHardwareButton();

  // 2. Periodically publish system telemetry
  if (
    millis() - lastTelemetryMs >=
    TELEMETRY_PERIOD_MS
  ) {
    lastTelemetryMs = millis();

    if (WiFi.status() == WL_CONNECTED) {
      publishTelemetry();
    }
  }

  delay(10);
}
```

### Key Implementation Details

#### Active-LOW Pull-Up Configuration

The following line enables the ESP32-S3's internal pull-up resistor:

```cpp
pinMode(BUTTON_PIN, INPUT_PULLUP);
```

The button behaves as follows:

- Unpressed: The pin reads `HIGH`.
- Pressed: The button connects the pin to ground, so it reads `LOW`.

#### Synchronizing the Local State with the Cloud

When the physical button is pressed, this call reports the new state to RainMaker:

```cpp
my_switch.updateAndReportParam(
  "Power",
  currentLedState
);
```

The new state is sent to the RainMaker cloud, causing the toggle button in the mobile application to update automatically.

#### Shared State Variable

Both control paths use the same variable:

```cpp
bool currentLedState = false;
```

The variable is read and modified by:

- `write_callback()`, which handles app or cloud commands.
- `handleHardwareButton()`, which handles the physical button.

Using one shared state variable keeps the physical LED and cloud interface synchronized.

#### Non-Blocking Debouncing

The button handler does not use a long blocking `delay()`. Instead, it compares elapsed time using `millis()`:

```cpp
if (
  millis() - lastDebounceTime >
  DEBOUNCE_DELAY_MS
) {
  // Confirm the button state
}
```

This allows the ESP32-S3 to continue processing RainMaker communication, telemetry, and other tasks while the button input is being debounced.

--- 
# Step-by-Step Dashboard Setup on Evaluation Hub

## Access the Portal

Navigate to:

https://evaluation.rainmaker.espressif.com/

## Login / User Creation

Sign in using your existing ESP RainMaker account credentials.

If you do not have one:

- Register through the ESP RainMaker Mobile App (iOS or Android), or
- Create an account through the RainMaker Dashboard.

## Select Evaluation / Tryout Center

Under the **Evaluation Hub** menu, open:

- **Tryout Center**, or
- **Studio**

## Choose a Template

Select a pre-configured template such as:

- Switch
- Light
- Temperature Sensor

Ensure the template matches the parameters registered by your firmware.

## Provision and Pair the Node

1. Power on your ESP32-S3 running the RainMaker firmware.
2. Open the ESP RainMaker mobile app.
3. Tap **Add Device (+)**.
4. Scan the QR code displayed in the Arduino IDE Serial Monitor during BLE commissioning.

Alternatively, manually enter:

```text
POP: 12345678
```

## Link Dashboard with Account

Once the ESP32-S3 node is associated with your account:

1. Refresh the Evaluation Hub browser page.
2. Open:

   https://evaluation.rainmaker.espressif.com/

3. Locate your device under:

   - Associated Nodes
   - User Devices

## Verify Operation

You should now be able to:

- Toggle LED switches
- View real-time temperature graphs
- Monitor device status
- Send test commands
- Validate telemetry updates directly from the web dashboard
---
