#include <Arduino.h>
#include <Preferences.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

struct DeviceSettings {
  int brightness;               // 0–100
  bool autoMode;
  char deviceName;           // Up to 31 bytes plus '\0'
  float threshold;
};

const DeviceSettings DEFAULT_SETTINGS = {
  50, true, "ESP32-S3", 25.0f
};

// Stable format for flash.
// Do not deserialize arbitrary flash bytes directly into bool.
struct StoredSettings {
  uint32_t version;
  int32_t brightness;
  uint8_t autoMode;              // Must be 0 or 1
  uint8_t reserved;
  char deviceName;
  float threshold;
  uint32_t crc;
};

constexpr uint32_t SETTINGS_VERSION = 1;

static_assert(sizeof(float) == 4,
              "This format requires a 32-bit float");
static_assert(offsetof(StoredSettings, crc) == 48,
              "Storage layout changed");
static_assert(sizeof(StoredSettings) == 52,
              "Storage layout changed");

// Example application rule:
// threshold must be finite and between -1000 and +1000.
bool validateSettings(const DeviceSettings &s) {
  return s.brightness >= 0 && s.brightness <= 100 &&
         s.deviceName != '\0' &&
         memchr(s.deviceName, '\0',
                sizeof(s.deviceName)) != nullptr &&
         isfinite(s.threshold) &&
         s.threshold >= -1000.0f &&
         s.threshold <= 1000.0f;
}

uint32_t crc32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xFFFFFFFFu;

  while (length--) {
    crc ^= *data++;

    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^
            ((crc & 1u) ? 0xEDB88320u : 0u);
    }
  }

  return ~crc;
}

StoredSettings makeRecord(const DeviceSettings &s) {
  StoredSettings r;

  // Initialize reserved bytes deterministically.
  memset(&r, 0, sizeof(r));

  r.version = SETTINGS_VERSION;
  r.brightness = s.brightness;
  r.autoMode = s.autoMode ? 1 : 0;

  // Call only after validateSettings() succeeds.
  memcpy(r.deviceName, s.deviceName, strlen(s.deviceName));

  r.threshold = s.threshold;
  r.crc = crc32(
    reinterpret_cast<const uint8_t *>(&r),
    offsetof(StoredSettings, crc)
  );

  return r;
}

bool decodeRecord(const StoredSettings &r,
                  DeviceSettings &out) {
  if (r.version != SETTINGS_VERSION ||
      r.autoMode > 1 ||
      r.crc != crc32(
        reinterpret_cast<const uint8_t *>(&r),
        offsetof(StoredSettings, crc))) {
    return false;
  }

  DeviceSettings candidate{};
  candidate.brightness = r.brightness;
  candidate.autoMode = (r.autoMode == 1);

  memcpy(candidate.deviceName, r.deviceName,
         sizeof(candidate.deviceName));

  candidate.threshold = r.threshold;

  if (!validateSettings(candidate)) return false;

  // Update RAM only after all checks pass.
  out = candidate;
  return true;
}

enum class LoadStatus {
  Loaded,
  Missing,
  Invalid,
  Incompatible,
  ReadError
};

Preferences preferences;
DeviceSettings settings = DEFAULT_SETTINGS;

constexpr char NVS_NAMESPACE[] = "my-app";
constexpr char SETTINGS_KEY[] = "settings";
constexpr uint8_t RESET_PIN = 0;
constexpr uint32_t RESET_HOLD_MS = 5000;

bool nvsReady = false;
bool saveBlocked = false;
bool dirty = true;

// Explicit declarations avoid Arduino prototype-order surprises.
bool validateSettings(const DeviceSettings &s);
uint32_t crc32(const uint8_t *data, size_t length);
StoredSettings makeRecord(const DeviceSettings &s);
bool decodeRecord(const StoredSettings &r, DeviceSettings &out);
LoadStatus loadSettings();
bool saveSettings();
void factoryReset();
void printSettings();
void printHelp();
bool parseInteger(const String &text, long &value);
bool parseFloatValue(const String &text, float &value);
void handleCommand(String command);
void pollSerial();
void pollResetButton();

LoadStatus loadSettings() {
  if (!nvsReady) return LoadStatus::ReadError;

  if (!preferences.isKey(SETTINGS_KEY)) {
    return LoadStatus::Missing;
  }

  // Preserve an unexpected type/layout.
  if (preferences.getType(SETTINGS_KEY) != PT_BLOB ||
      preferences.getBytesLength(SETTINGS_KEY) !=
        sizeof(StoredSettings)) {
    return LoadStatus::Incompatible;
  }

  StoredSettings record{};

  if (preferences.getBytes(
        SETTINGS_KEY, &record, sizeof(record)) !=
      sizeof(record)) {
    return LoadStatus::ReadError;
  }

  if (record.version != SETTINGS_VERSION) {
    return LoadStatus::Incompatible;
  }

  if (!decodeRecord(record, settings)) {
    return LoadStatus::Invalid;
  }

  dirty = false;
  return LoadStatus::Loaded;
}

bool saveSettings() {
  if (!nvsReady || saveBlocked) {
    Serial.println(
      "[NVS] Save blocked: storage unavailable or incompatible."
    );
    return false;
  }

  if (!validateSettings(settings)) {
    Serial.println("[NVS] Save rejected: invalid RAM settings.");
    return false;
  }

  if (!dirty) {
    Serial.println("[NVS] No pending changes; no write needed.");
    return true;
  }

  const StoredSettings record = makeRecord(settings);

  if (preferences.putBytes(
        SETTINGS_KEY, &record, sizeof(record)) !=
      sizeof(record)) {
    Serial.println(
      "[NVS] Save FAILED; RAM changes are still pending."
    );
    return false;
  }

  // Preferences commits internally.
  dirty = false;
  Serial.println("[NVS] Saved 52-byte settings record.");
  return true;
}

void factoryReset() {
  if (!nvsReady || !preferences.clear()) {
    Serial.println(
      "[RESET] Clear FAILED; defaults were not applied."
    );
    return;
  }

  settings = DEFAULT_SETTINGS;
  saveBlocked = false;
  dirty = true;

  Serial.println(
    "[RESET] Namespace cleared. Factory defaults loaded."
  );

  if (!saveSettings()) {
    Serial.println(
      "[RESET] Defaults active in RAM, but saving failed."
    );
  }

  printSettings();
}

void printSettings() {
  Serial.printf(
    "brightness=%d, autoMode=%s, deviceName=\"%s\", "
    "threshold=%.2f, pending=%s\n",
    settings.brightness,
    settings.autoMode ? "ON" : "OFF",
    settings.deviceName,
    settings.threshold,
    dirty ? "YES" : "NO"
  );
}

void printHelp() {
  Serial.println("Commands (send a newline):");
  Serial.println("  SHOW | BRIGHTNESS 0..100 | AUTO 0|1");
  Serial.println("  NAME <1..31 bytes> | THRESHOLD -1000..1000");
  Serial.println("  SAVE | REBOOT | FACTORY_RESET | HELP");
  Serial.println(
    "Changes affect RAM immediately; SAVE writes flash."
  );
}

bool parseInteger(const String &text, long &value) {
  const char *start = text.c_str();
  char *end = nullptr;

  errno = 0;
  value = strtol(start, &end, 10);

  return end != start &&
         *end == '\0' &&
         errno != ERANGE;
}

bool parseFloatValue(const String &text, float &value) {
  const char *start = text.c_str();
  char *end = nullptr;

  errno = 0;
  value = strtof(start, &end);

  return end != start &&
         *end == '\0' &&
         errno != ERANGE &&
         isfinite(value);
}

void handleCommand(String command) {
  command.trim();
  if (command.length() == 0) return;

  if (command == "SHOW") {
    printSettings();
    return;
  }

  if (command == "HELP") {
    printHelp();
    return;
  }

  if (command == "SAVE") {
    saveSettings();
    return;
  }

  if (command == "FACTORY_RESET") {
    factoryReset();
    return;
  }

  if (command == "REBOOT") {
    Serial.println(
      dirty
        ? "[BOOT] Rebooting WITHOUT saving RAM changes."
        : "[BOOT] Rebooting..."
    );

    if (nvsReady) preferences.end();

    Serial.flush();
    delay(100);
    ESP.restart();
    return;
  }

  DeviceSettings candidate = settings;
  long number;

  if (command.startsWith("BRIGHTNESS ")) {
    String value = command.substring(11);
    value.trim();

    if (!parseInteger(value, number) ||
        number < 0 || number > 100) {
      Serial.println(
        "[INPUT] Brightness must be an integer 0..100."
      );
      return;
    }

    candidate.brightness = static_cast<int>(number);

  } else if (command.startsWith("AUTO ")) {
    String value = command.substring(5);
    value.trim();

    if (!parseInteger(value, number) ||
        (number != 0 && number != 1)) {
      Serial.println("[INPUT] AUTO must be 0 or 1.");
      return;
    }

    candidate.autoMode = (number == 1);

  } else if (command.startsWith("NAME ")) {
    String value = command.substring(5);
    value.trim();

    if (value.length() == 0 ||
        value.length() >= sizeof(candidate.deviceName)) {
      Serial.println("[INPUT] Name must contain 1..31 bytes.");
      return;
    }

    memset(candidate.deviceName, 0,
           sizeof(candidate.deviceName));

    value.toCharArray(candidate.deviceName,
                      sizeof(candidate.deviceName));

  } else if (command.startsWith("THRESHOLD ")) {
    String value = command.substring(10);
    value.trim();

    if (!parseFloatValue(value, candidate.threshold)) {
      Serial.println("[INPUT] Invalid finite number.");
      return;
    }

  } else {
    Serial.println("[INPUT] Unknown command. Type HELP.");
    return;
  }

  if (!validateSettings(candidate)) {
    Serial.println("[INPUT] Value outside the allowed range.");
    return;
  }

  // Compare fields rather than raw structs, which may have padding.
  const bool changed =
    candidate.brightness != settings.brightness ||
    candidate.autoMode != settings.autoMode ||
    strcmp(candidate.deviceName, settings.deviceName) != 0 ||
    candidate.threshold != settings.threshold;

  settings = candidate;
  dirty = dirty || changed;

  printSettings();
}

void pollSerial() {
  static String line;
  static bool overflow = false;

  // Limit each pass so serial traffic cannot starve button polling.
  for (int count = 0;
       count < 64 && Serial.available();
       ++count) {
    const char c = static_cast<char>(Serial.read());

    if (c == '\r') continue;

    if (c == '\n') {
      if (overflow) {
        Serial.println("[INPUT] Line too long; discarded.");
      } else {
        handleCommand(line);
      }

      line = "";
      overflow = false;

    } else if (!overflow) {
      if (line.length() < 95) {
        line += c;
      } else {
        overflow = true;
      }
    }
  }
}

// Active-LOW button; nonblocking debounce and hold timing.
int buttonSample = HIGH;
int buttonStable = HIGH;
uint32_t buttonChangedAt = 0;
uint32_t buttonPressedAt = 0;

// Require release after startup and after each reset action.
bool buttonArmed = false;

void pollResetButton() {
  const uint32_t now = millis();
  const int sample = digitalRead(RESET_PIN);

  if (sample != buttonSample) {
    buttonSample = sample;
    buttonChangedAt = now;
  }

  if (sample != buttonStable &&
      uint32_t(now - buttonChangedAt) >= 30) {
    buttonStable = sample;

    if (buttonStable == LOW && buttonArmed) {
      buttonPressedAt = now;
      Serial.println(
        "[BUTTON] Keep holding BOOT for 5 seconds to reset."
      );
    }
  }

  if (buttonStable == HIGH) {
    buttonArmed = true;
  }

  if (buttonStable == LOW &&
      buttonArmed &&
      uint32_t(now - buttonPressedAt) >= RESET_HOLD_MS) {
    buttonArmed = false;  // One action per continuous press
    factoryReset();
  }
}

void setup() {
  Serial.begin(115200);

  // Do not wait indefinitely for a USB host.
  delay(300);

  pinMode(RESET_PIN, INPUT_PULLUP);
  buttonSample = buttonStable = digitalRead(RESET_PIN);
  buttonChangedAt = millis();

  Serial.println("\nSaving User Settings on ESP32-S3");

  // false = read/write; default partition label is "nvs".
  nvsReady = preferences.begin(NVS_NAMESPACE, false);

  if (!nvsReady) {
    Serial.println(
      "[NVS] Open failed. Using defaults in RAM only."
    );

  } else {
    const LoadStatus status = loadSettings();

    if (status == LoadStatus::Loaded) {
      Serial.println(
        "[NVS] Valid settings loaded from flash."
      );

    } else if (status == LoadStatus::Missing ||
               status == LoadStatus::Invalid) {
      Serial.println(
        status == LoadStatus::Missing
          ? "[NVS] First boot / empty namespace. Using defaults."
          : "[NVS] Invalid record. Restoring defaults."
      );

      settings = DEFAULT_SETTINGS;
      dirty = true;
      saveSettings();

    } else {
      // Preserve an unsupported format or failed read.
      saveBlocked = true;

      Serial.println(
        "[NVS] Unreadable/incompatible record. Using RAM defaults;"
      );
      Serial.println(
        "      writes blocked. Diagnose or use FACTORY_RESET."
      );
    }
  }

  printSettings();
  printHelp();
}

void loop() {
  pollResetButton();
  pollSerial();
  delay(1);
}
