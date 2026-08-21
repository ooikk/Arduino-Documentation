#include <WiFi.h>
#include <RMaker.h>
#include <WiFiProv.h>
#include <esp_wifi.h>

//#define PROVISION_WIFI
//#define TELEMETRY_VALUE

// ------------------------------------------------------------------
// Original topic names kept for reference.
//
// ESP RainMaker does not publish directly to these topics.
// ------------------------------------------------------------------
/*
const char* TOPIC_STATUS = "ooikk/feeds/status";
const char* TOPIC_TEMP = "ooikk/feeds/temperature";
const char* TOPIC_RSSI = "ooikk/feeds/rssi";
const char* TOPIC_UPTIME = "ooikk/feeds/uptime";
const char* TOPIC_LED_SET = "ooikk/feeds/led-control";
const char* TOPIC_LED_STATE = "ooikk/feeds/led-state";
const char* TOPIC_BUTTON = "ooikk/feeds/button";
*/

// ------------------------------------------------------------------
// RainMaker parameter names
// ------------------------------------------------------------------
const char* PARAM_STATUS = "status";
const char* PARAM_TEMP = "temperature";
const char* PARAM_RSSI = "rssi";
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
const int LED_PIN = 1;
const int BUTTON_PIN = 2;

// ------------------------------------------------------------------
// Timing
// RainMaker's backend rate limit of 10 MQTT messages per minute per node (with a maximum burst capacity of 10 messages).
// ------------------------------------------------------------------
const uint32_t TELEMETRY_INTERVAL_MS = 30000;
const uint32_t BUTTON_DEBOUNCE_MS = 50;

// ------------------------------------------------------------------
// Runtime state
// ------------------------------------------------------------------
static bool led_state = false;
static volatile bool wifi_connected = false;


// ------------------------------------------------------------------
// RainMaker parameter handles
// ------------------------------------------------------------------

// Dynamic device pointers (Allocated after RMaker.initNode)
static Device* led_device = NULL;
static Device* temp_device = NULL;
static Device* telemetry_device = NULL;
static Device* button_device = NULL;

// ==================================================================
// LED control
// ==================================================================
static void applyLedState(bool state) {
  led_state = state;
  digitalWrite(LED_PIN, led_state);
  led_device->updateAndReportParam(PARAM_LED_STATE, led_state);
  led_device->updateAndReportParam(PARAM_LED_CONTROL, led_state);
  Serial.printf("LED set to %s\n", led_state ? "ON" : "OFF");
}

static void led_write_cb(Device* device, Param* param,
                         const param_val_t val,
                         void* priv_data, write_ctx_t* ctx) {
  if (!param) return;
  //const char *device_name = device->getDeviceName();
  if (strcmp(param->getParamName(), PARAM_LED_CONTROL) == 0) {
    applyLedState(val.val.b);  // if your header uses a plain union: val.b
  }
}

// ------------------------------------------------------------------
// Telemetry reporting
//
// This is the RainMaker equivalent of publishing to the feed topics.
// ------------------------------------------------------------------
void reportTelemetry() {
  //if (WiFi.status() != WL_CONNECTED || !telemetry_device) return;
  // Query ESP-IDF Wi-Fi driver directly
  wifi_ap_record_t ap_info;
  if (!telemetry_device || esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) return;

  float temp_c = random(200, 500) / 10.0f;  //temperatureRead();
  //int rssi = (int)WiFi.RSSI();
  int rssi = ap_info.rssi;
  uint32_t uptime_seconds = millis() / 1000;  // Use uint32_t for rollover safety

  telemetry_device->updateAndReportParam(PARAM_STATUS, "online");

  if (temp_device) {
    temp_device->updateAndReportParam(PARAM_TEMP, temp_c);
  }
  
#ifdef TELEMETRY_VALUE
  // Send raw numeric values so the dashboard can chart them
  telemetry_device->updateAndReportParam(PARAM_RSSI, rssi);
  telemetry_device->updateAndReportParam(PARAM_UPTIME, (int)uptime_seconds);
#else // Display as text string
  char rssi_str[16];
  snprintf(rssi_str, sizeof(rssi_str), "%d dBm", rssi);
  telemetry_device->updateAndReportParam(PARAM_RSSI, rssi_str);

  char uptime_str[16];
  snprintf(uptime_str, sizeof(uptime_str), "%d s", uptime_seconds);
  telemetry_device->updateAndReportParam(PARAM_UPTIME, uptime_str);
#endif


  Serial.printf("Telemetry: temp=%.2f C, RSSI=%d dBm, uptime=%u s\n",
                temp_c, rssi, uptime_seconds);
}

// ------------------------------------------------------------------
// Button handling
//
// This is the RainMaker equivalent of publishing to TOPIC_BUTTON.
// ------------------------------------------------------------------
void handleButton() {
  static bool pressed = false;
  static uint32_t last_check = 0;

  if (millis() - last_check < BUTTON_DEBOUNCE_MS) return;
  last_check = millis();

  bool active = (digitalRead(BUTTON_PIN) == LOW);

  if (active && !pressed) {
    pressed = true;
    if (button_device) {
      button_device->updateAndReportParam(PARAM_BUTTON, "pressed");
    }
    Serial.println("Button pressed");
  } else if (!active && pressed) {
    pressed = false;
    if (button_device) {
      button_device->updateAndReportParam(PARAM_BUTTON, "released");
    }
    Serial.println("Button released");
  }
}


void sysProvEvent(arduino_event_t* sys_event) {
  Serial.printf("EVENT: %d\n", sys_event->event_id);

  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_INIT:
      //wifi_prov_mgr_disable_auto_stop(10000);
      Serial.println("PROV_INIT");
      break;

    case ARDUINO_EVENT_PROV_START:
#ifdef PROVISION_WIFI
      Serial.println("WiFi Provisioning Started: PROV_START");
      WiFiProv.printQR(PROV_SERVICE_NAME, PROV_POP, "softap");
      // Manual fallback (if needed)
      Serial.println("Manual print QR Code link");
      char payload[150];
      snprintf(payload, sizeof(payload),
               "{\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"%s\"}",
               PROV_SERVICE_NAME, PROV_POP, "softap");
      Serial.printf("QR URL: https://rainmaker.espressif.com/qrcode.html?data=%s\n", payload);

#else
      Serial.println("BLE Provisioning Started: PROV_START");
      WiFiProv.printQR(PROV_SERVICE_NAME, PROV_POP, "ble");
      // Manual fallback (if needed)
      Serial.println("Manual print QR Code link");
      char payload[150];
      snprintf(payload, sizeof(payload),
               "{\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"%s\"}",
               PROV_SERVICE_NAME, PROV_POP, "ble");
      Serial.printf("QR URL: https://rainmaker.espressif.com/qrcode.html?data=%s\n", payload);

#endif
      break;
    case ARDUINO_EVENT_PROV_CRED_RECV:
      Serial.println("PROV_CRED_RECV");
      break;

    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      //wifi_prov_mgr_stop_provisioning();
      Serial.println("WiFi credentials received: PROV_CRED_SUCCESS");
      break;
    case ARDUINO_EVENT_PROV_END:
      Serial.println("PROV_END");
      break;

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WIFI_CONNECTED");
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WIFI_DISCONNECTED");
      break;
    case ARDUINO_EVENT_PROV_CRED_FAIL:
      Serial.println("[PROV] Wi-Fi provisioning failed (Check SSID/Password)");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.printf("GOT_IP: %s\n",
                    WiFi.localIP().toString().c_str());
      break;
    case 34:
    default:
      Serial.printf("Unknown event: %d\n", sys_event->event_id);
      break;
  }
}



// ------------------------------------------------------------------
// Setup
// ------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(2000);
  // Press and hold BOOT for 3 seconds to erase
  // RainMaker credentials and enter provisioning mode
  //RMakerFactoryReset(3);

  /*
  // 1. Initialize Default NVS Flash
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
*/

  Serial.println("\n----------------------------------------");
  Serial.println("ESP32-S3 ESP RainMaker Telemetry");
  Serial.println("----------------------------------------");

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, led_state);


  // ----------------------------------------------------------------
  // 1. Create the RainMaker node
  // ----------------------------------------------------------------
  // 1. Initialize Node
  Node my_node = RMaker.initNode("OOIKK ESP32-S3");

  // 2. LED Device Setup
  led_device = new Device("LED", "esp.device.switch");
  if (led_device) {
    led_device->addNameParam();
    Param led_power_param(PARAM_LED_CONTROL, "esp.param.power", value(false), PROP_FLAG_READ | PROP_FLAG_WRITE | PROP_FLAG_PERSIST);
    Param led_state_param(PARAM_LED_STATE, "custom.param.led_state", value(false), PROP_FLAG_READ);

    led_device->addParam(led_power_param);
    led_device->addParam(led_state_param);
    led_device->addCb(led_write_cb);
    my_node.addDevice(*led_device);
  }

  // 3. Temperature Device Setup
  temp_device = new Device("Temperature", "esp.device.temperature-sensor");
  if (temp_device) {
    temp_device->addNameParam();
    Param temp_param(PARAM_TEMP, "esp.param.temperature", value(0.0f), PROP_FLAG_READ | PROP_FLAG_TIME_SERIES);
    temp_device->addParam(temp_param);
    my_node.addDevice(*temp_device);
  }


  // 4. Custom Telemetry Device Setup
  telemetry_device = new Device("Telemetry", "custom.device.telemetry");
  if (telemetry_device) {
    telemetry_device->addNameParam();

    Param status_param(PARAM_STATUS, "esp.param.text", value("boot"), PROP_FLAG_READ);
    status_param.addUIType(ESP_RMAKER_UI_TEXT);
    telemetry_device->addParam(status_param);

#ifdef TELEMETRY_VALUE
    // 1. RSSI as a read-only range (Renders as a numeric card, not a slider)
    //Param rssi_param(PARAM_RSSI, "esp.param.range", value(0), PROP_FLAG_READ | PROP_FLAG_TIME_SERIES);
    //rssi_param.addBounds(value(-100), value(0), value(1));

    // 1. RSSI as a read-only range and enable timeseries
    Param rssi_param(PARAM_RSSI, "esp.param.text", value(0), PROP_FLAG_READ | PROP_FLAG_TIME_SERIES);
    //Param rssi_param(PARAM_RSSI, "esp.param.speed", value(0), PROP_FLAG_READ | PROP_FLAG_TIME_SERIES);
#else
    Param rssi_param(PARAM_RSSI, "esp.param.text", value("0 dBm"), PROP_FLAG_READ );
    rssi_param.addUIType(ESP_RMAKER_UI_TEXT);
#endif
    telemetry_device->addParam(rssi_param);

#ifdef TELEMETRY_VALUE
    // 2. Uptime as a read-only range
    //Param uptime_param(PARAM_UPTIME, "esp.param.range", value(0), PROP_FLAG_READ | PROP_FLAG_TIME_SERIES);
    //uptime_param.addBounds(value(0), value(2147483647), value(1));

    // 2. Uptime as a read-only range and enable timeseries
    Param uptime_param(PARAM_UPTIME, "esp.param.text", value(0), PROP_FLAG_READ | PROP_FLAG_TIME_SERIES);
    //Param uptime_param(PARAM_UPTIME, "esp.param.speed", value(0), PROP_FLAG_READ | PROP_FLAG_TIME_SERIES);
#else
    Param uptime_param(PARAM_UPTIME, "esp.param.text", value("0 s"), PROP_FLAG_READ);
    uptime_param.addUIType(ESP_RMAKER_UI_TEXT);
#endif    
    telemetry_device->addParam(uptime_param);

    my_node.addDevice(*telemetry_device);
  }

  // 5. Custom Button Device Setup (Appears as a separate card in app)
  button_device = new Device("Button", "custom.device.button");
  if (button_device) {
    button_device->addNameParam();

    Param button_param(PARAM_BUTTON, "esp.param.text", value("idle"), PROP_FLAG_READ);
    button_param.addUIType(ESP_RMAKER_UI_TEXT);

    button_device->addParam(button_param);
    my_node.addDevice(*button_device);
  }

  Serial.println("Telemetry device added");


  // Enable Services

  //RMaker.enableOTA(OTA_USING_TOPICS);
  RMaker.enableTZService();
  RMaker.enableSchedule();
  RMaker.enableScenes();


  // Start RainMaker now that WiFi is ready
  if (RMaker.start() == ESP_OK) {
    Serial.println("RMaker started successfully");
  } else {
    Serial.println("Failed to start RMaker");
  }

  WiFi.onEvent(sysProvEvent);

  WiFiProv.beginProvision(
#ifdef PROVISION_WIFI
    WIFI_PROV_SCHEME_SOFTAP,        // for WiFi
    WIFI_PROV_SCHEME_HANDLER_NONE,  // for WiFi
#else
    WIFI_PROV_SCHEME_BLE,                // for BLE
    WIFI_PROV_SCHEME_HANDLER_FREE_BTDM,  // for BLE
#endif
    WIFI_PROV_SECURITY_1,
    PROV_POP,
    PROV_SERVICE_NAME);


  Serial.printf("Provisioning service: %s, PoP: %s\n", PROV_SERVICE_NAME, PROV_POP);
}



// ------------------------------------------------------------------
// Main loop
// ------------------------------------------------------------------
void loop() {
  // Safe Factory Reset: Type 'r' in the Serial Monitor to erase RainMaker NVS

  if (Serial.available() > 0) {
    char ch = Serial.read();
    if (ch == 'r' || ch == 'R') {
      Serial.println("\n[RESET] Clearing RainMaker credentials and rebooting...");
      delay(1000);
      esp_rmaker_factory_reset(0, 2);  // Standard SDK reset call
    }
  }


  static uint32_t last_telemetry = 0;
  if (millis() - last_telemetry >= TELEMETRY_INTERVAL_MS) {
    last_telemetry = millis();
    reportTelemetry();
  }
  handleButton();
  delay(10);
}
