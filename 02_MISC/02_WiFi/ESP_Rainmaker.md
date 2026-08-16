

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
---

# ESP RainMaker Web Dashboard

Setting up the ESP RainMaker Web Dashboard allows you to monitor your ESP32-S3 from a desktop browser, view historical charts, and build custom control panels.

The Web Dashboard and Mobile App use the same cloud backend, so you do not need to provision the device again. Since the `OOIKK ESP32-S3` node was already provisioned using the phone application, it should automatically appear on the web dashboard.

This guide explains how to configure and customize the dashboard using the C++ code described earlier.

## Step 1: Log In to the RainMaker Web Dashboard

1. Open a web browser and visit:

   [ESP RainMaker Web Dashboard](https://rainmaker.espressif.com/)

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
