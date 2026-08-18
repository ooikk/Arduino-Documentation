#include "RMaker.h"
#include "WiFiProv.h"


#define HARDWARE_BUTTON

// -----------------------------------------------------------------------------
// HARDWARE & TIMING CONFIG
// -----------------------------------------------------------------------------

const int LED_PIN = 1;  // ESP32-S3 GPIO for onboard/external LED
const uint32_t TELEMETRY_PERIOD_MS = 10000;
uint32_t lastTelemetryMs = 0;
bool currentLedState = false;

#ifdef HARDWARE_BUTTON
const int BUTTON_PIN = 2;
// Button state tracking and debouncing
bool lastButtonReading = HIGH;

uint32_t lastDebounceTime = 0;
const uint32_t DEBOUNCE_DELAY_MS = 50;
#endif


// BLE Provisioning Credentials (Used for first-time pairing via phone app)
const char *service_name = "PROV_ESP32S3";
const char *pop = "12345678";  // Proof of Possession / PIN

// -----------------------------------------------------------------------------
// RAINMAKER NODES & DEVICES
// -----------------------------------------------------------------------------
static Node my_node;
// FIX 1: Cast (void*)&LED_PIN to void* to prevent const void* conversion error
static Switch my_switch("LED Control", (void *)&LED_PIN);
static TemperatureSensor my_temp_sensor("Temperature Sensor");

// Generic device to hold system telemetry parameters
static Device sys_telemetry("System Stats", "custom.device.system");

// Parameter Pointers for dynamic reporting
static Param *p_status = NULL;
static Param *p_rssi = NULL;
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

  if ((strcmp(device_name, "LED Control") == 0) && (strcmp(param_name, "Power") == 0)) {
      currentLedState = val.val.b;
      digitalWrite(LED_PIN, currentLedState ? HIGH : LOW);
      param->updateAndReport(val);
      Serial.printf("[Cloud Command] LED toggled -> %s\n", currentLedState ? "ON" : "OFF");
    }
}

#ifdef HARDWARE_BUTTON
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
    millis() - lastDebounceTime > DEBOUNCE_DELAY_MS) {
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
          currentLedState ? HIGH : LOW);

        // Report the local state change to RainMaker
        my_switch.updateAndReportParam(
          "Power",
          currentLedState);

        Serial.printf(
          "[Local Button] Toggled LED -> %s "
          "(Synced to App)\n",
          currentLedState ? "ON" : "OFF");
      }
    }
  }

  lastButtonReading = reading;
}
#endif

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

#ifdef HARDWARE_BUTTON
  // Configure the button with the internal pull-up resistor
  pinMode(BUTTON_PIN, INPUT_PULLUP);
#endif
  // 1. Initialize Node
  my_node = RMaker.initNode("ESP32S3_Node");

  // 2. Configure LED Switch Device
  my_switch.addCb(write_callback);
  my_node.addDevice(my_switch);

  // 3. Configure Temperature Sensor Device
  my_node.addDevice(my_temp_sensor);

  // 4. Create System Telemetry Parameters using v3.x esp_rmaker_* value helpers
  p_status = new Param("Status", "custom.param.status", esp_rmaker_str("online"), PROP_FLAG_READ);
  p_rssi = new Param("RSSI", "custom.param.rssi", esp_rmaker_int(0), PROP_FLAG_READ);
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
#ifdef HARDWARE_BUTTON
  // 1. Continuously check the local hardware input
  handleHardwareButton();
#endif
  // 2. Periodically publish system telemetry
  if (millis() - lastTelemetryMs >= TELEMETRY_PERIOD_MS) {
    lastTelemetryMs = millis();
    if (WiFi.status() == WL_CONNECTED) {
      publishTelemetry();
    }
  }
  delay(10);
}