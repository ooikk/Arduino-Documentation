# Saving User Settings in Non-Volatile Memory on ESP32-S3

**Hardware:** ESP32-S3 DevKitC-1  
**Framework:** Arduino for ESP32, with an ESP-IDF equivalent  
**Goal:** Preserve user settings across reboot and power loss.

## 1. Introduction: RAM, flash, and NVS

Ordinary variables live in RAM. When the ESP32-S3 restarts, the program initializes those variables again; removing power loses their contents. PSRAM does not provide permanent storage either.

Flash retains data without power. ESP32 uses **Non-Volatile Storage (NVS)** to organize small persistent values as key-value pairs inside a flash partition, normally labeled `nvs`. Your settings occupy part of that partition, separate from the application image. NVS includes wear leveling: updates append entries, and obsolete space is reclaimed later, reducing repeated erases of the same location. Flash endurance is still finite. [1]

The application should follow this pattern:

1. Read settings from NVS once at startup.
2. Keep the active settings in a RAM struct.
3. Modify RAM when the user changes a value.
4. Save to NVS when the user requests it.

**A change in RAM becomes persistent only after a successful save.** Reads do not consume flash program/erase endurance. Do not continuously write settings in `loop()`.

## 2. Choosing a library

| Option | How it stores settings | Recommendation |
|---|---|---|
| `Preferences.h` | Arduino wrapper around NVS, using namespaces and named keys | Main choice for new Arduino ESP32 projects |
| `EEPROM.h` | Emulates an EEPROM byte array using an NVS blob; uses `commit()` | Deprecated in Arduino-ESP32; retain for legacy compatibility |
| `nvs.h` + `nvs_flash.h` | Native ESP-IDF interface with handles and explicit error codes | Use in ESP-IDF or when detailed storage control is needed |

Espressif explicitly recommends Preferences instead of EEPROM for new ESP32 applications. Preferences is included with the **esp32 by Espressif Systems** board package; no separate library installation is needed. [2][3]

`nvs_flash.h` supplies partition initialization and erase functions. `nvs.h` supplies operations such as `nvs_open()`, `nvs_get_blob()`, and `nvs_set_blob()`.

## 3. Settings structure and defaults

```cpp
struct DeviceSettings {
  int brightness;       // 0-100
  bool autoMode;
  char deviceName[32];  // Up to 31 bytes plus terminating '\0'
  float threshold;
};

const DeviceSettings DEFAULT_SETTINGS = {
  50, true, "ESP32-S3", 25.0f
};
```

Here, `threshold` must be finite and between **-1000 and +1000**. This is an example application rule; replace it with the physical limits of your sensor or control algorithm.

The sketch maintains `DeviceSettings settings` in RAM. It saves a **52-byte `StoredSettings` record** containing the fields, a format version, and CRC-32.

Why use a separate stored record?

- A C++ struct can contain padding, and changing its fields can change its binary layout.
- A stored integer of 0 or 1 can be checked before converting it to `bool`.
- A format version identifies records from other firmware versions.
- A checksum and field validation help detect damaged or nonsensical data.

The CRC detects accidental corruption; it does not authenticate data. The stored format assumes ESP32-S3's byte order and 32-bit floating-point representation. For a format shared with other architectures, serialize each field explicitly.

## 4. Core logic

### Startup

`preferences.begin("my-app", false)` opens the application's namespace for reading and writing. The sketch then calls `loadSettings()`.

| Load result | Startup action |
|---|---|
| Missing key: first boot or cleared namespace | Load and save `DEFAULT_SETTINGS` |
| Valid record | Load saved values into the RAM struct |
| Matching format but bad checksum/invalid values | Restore and save defaults |
| Unexpected version, type, or record size | Use defaults in RAM and block ordinary saves, preserving the existing record |
| Namespace open/read failure | Report the error; do not claim persistence |

`loadSettings()` checks the stored length, version, CRC, boolean encoding, brightness, name termination, and threshold. RAM is updated only after the record passes validation.

`saveSettings()` validates RAM, builds the stored record, and checks that `putBytes()` returns its complete size. Failed saves leave the pending-change flag set. Preferences performs its commit internally; there is no `Preferences.commit()`. [4]

### Saving on demand

```cpp
settings.brightness = 80;
dirty = true;
if (saveSettings()) {
  // The application may now report that the setting was saved.
}
```

The Serial commands below perform the same sequence. Repeating `SAVE` with no pending changes skips the write.

## 5. Arduino IDE setup and reset-button wiring

1. Install the **esp32 by Espressif Systems** board package in Boards Manager.
2. Select **ESP32S3 Dev Module**, or a board definition matching your DevKitC-1.
3. Set flash size and PSRAM to match the module fitted to your board. NVS itself does not require PSRAM.
4. Choose a partition scheme containing an NVS partition. Standard Arduino ESP32 schemes normally include one; a custom scheme must include it explicitly. An OTA-capable scheme is required only if you also use OTA.
5. For the board's **USB-to-UART connector**, use **USB CDC On Boot: Disabled** so `Serial` uses UART0. For native USB serial, enable USB CDC On Boot and use the matching USB mode/connector. [5]
6. Upload the sketch and open Serial Monitor at **115200 baud**, with **Newline** or **Both NL & CR** selected.

### Method 1: Hold the BOOT button for five seconds

DevKitC-1 already has a BOOT button on **GPIO 0**, active LOW. No extra wiring is needed. An optional external normally open pushbutton can connect **GPIO 0 to GND**; the sketch enables `INPUT_PULLUP`.

**Start the application normally, then press and hold BOOT.** GPIO 0 is also a boot-strapping pin: holding BOOT while resetting/powering up can select the ROM download mode, where the sketch cannot run. [6][7]

The sketch debounces the button, uses nonblocking hold timing, performs one reset per continuous press, and requires release before another reset. Do not press the EN/RESET button during the factory-reset hold.

### Method 2: Serial command

Send:

```text
FACTORY_RESET
```

Both methods call `preferences.clear()`, assign `DEFAULT_SETTINGS`, and save them. The defaults take effect immediately; rebooting is unnecessary.

`clear()` deletes the keys in **`my-app` only**. It does not erase the firmware or the entire NVS partition. Other namespaces are preserved. [3]

## 6. Complete Arduino sketch

Create a sketch named **SavingUserSettings**, then paste this into **SavingUserSettings.ino**. All required libraries come with the ESP32 board package.

This demonstrates storage and configuration; it does not drive an actual LED/display or sensor.

```cpp
#include <Arduino.h>
#include <Preferences.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

struct DeviceSettings {
  int brightness;               // 0-100
  bool autoMode;
  char deviceName[32];           // Up to 31 bytes plus '\0'
  float threshold;
};

const DeviceSettings DEFAULT_SETTINGS = {50, true, "ESP32-S3", 25.0f};

// Stable format for flash. Do not deserialize arbitrary flash bytes into bool.
struct StoredSettings {
  uint32_t version;
  int32_t brightness;
  uint8_t autoMode;              // Must be 0 or 1
  uint8_t reserved[3];
  char deviceName[32];
  float threshold;
  uint32_t crc;
};

constexpr uint32_t SETTINGS_VERSION = 1;
static_assert(sizeof(float) == 4, "This format requires a 32-bit float");
static_assert(offsetof(StoredSettings, crc) == 48, "Storage layout changed");
static_assert(sizeof(StoredSettings) == 52, "Storage layout changed");

// Application rule: threshold must be finite and between -1000 and +1000.
bool validateSettings(const DeviceSettings &s) {
  return s.brightness >= 0 && s.brightness <= 100 &&
         s.deviceName[0] != '\0' &&
         memchr(s.deviceName, '\0', sizeof(s.deviceName)) != nullptr &&
         isfinite(s.threshold) &&
         s.threshold >= -1000.0f && s.threshold <= 1000.0f;
}

uint32_t crc32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xFFFFFFFFu;
  while (length--) {
    crc ^= *data++;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
    }
  }
  return ~crc;
}

StoredSettings makeRecord(const DeviceSettings &s) {
  StoredSettings r;
  memset(&r, 0, sizeof(r));       // Initialize reserved bytes deterministically
  r.version = SETTINGS_VERSION;
  r.brightness = s.brightness;
  r.autoMode = s.autoMode ? 1 : 0;
  // Call only after validateSettings() succeeds.
  memcpy(r.deviceName, s.deviceName, strlen(s.deviceName));
  r.threshold = s.threshold;
  r.crc = crc32(reinterpret_cast<const uint8_t *>(&r),
                offsetof(StoredSettings, crc));
  return r;
}

bool decodeRecord(const StoredSettings &r, DeviceSettings &out) {
  if (r.version != SETTINGS_VERSION || r.autoMode > 1 ||
      r.crc != crc32(reinterpret_cast<const uint8_t *>(&r),
                     offsetof(StoredSettings, crc))) {
    return false;
  }
  DeviceSettings candidate{};
  candidate.brightness = r.brightness;
  candidate.autoMode = (r.autoMode == 1);
  memcpy(candidate.deviceName, r.deviceName, sizeof(candidate.deviceName));
  candidate.threshold = r.threshold;
  if (!validateSettings(candidate)) return false;
  out = candidate;               // Update RAM only after all checks pass
  return true;
}

enum class LoadStatus { Loaded, Missing, Invalid, Incompatible, ReadError };

Preferences preferences;
DeviceSettings settings = DEFAULT_SETTINGS;
constexpr char NVS_NAMESPACE[] = "my-app";
constexpr char SETTINGS_KEY[] = "settings";
constexpr uint8_t RESET_PIN = 0;
constexpr uint32_t RESET_HOLD_MS = 5000;
bool nvsReady = false;
bool saveBlocked = false;
bool dirty = true;

// Explicit declarations also avoid Arduino prototype-order surprises.
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
  if (!preferences.isKey(SETTINGS_KEY)) return LoadStatus::Missing;
  // Preserve an unexpected type/layout rather than overwrite it silently.
  if (preferences.getType(SETTINGS_KEY) != PT_BLOB ||
      preferences.getBytesLength(SETTINGS_KEY) != sizeof(StoredSettings)) {
    return LoadStatus::Incompatible;
  }
  StoredSettings record{};
  if (preferences.getBytes(SETTINGS_KEY, &record, sizeof(record)) !=
      sizeof(record)) {
    return LoadStatus::ReadError;
  }
  if (record.version != SETTINGS_VERSION) return LoadStatus::Incompatible;
  if (!decodeRecord(record, settings)) return LoadStatus::Invalid;
  dirty = false;
  return LoadStatus::Loaded;
}

bool saveSettings() {
  if (!nvsReady || saveBlocked) {
    Serial.println("[NVS] Save blocked: storage unavailable or incompatible.");
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
  if (preferences.putBytes(SETTINGS_KEY, &record, sizeof(record)) !=
      sizeof(record)) {
    Serial.println("[NVS] Save FAILED; RAM changes are still pending.");
    return false;
  }
  // Preferences commits internally; there is no Preferences.commit().
  dirty = false;
  Serial.println("[NVS] Saved 52-byte settings record.");
  return true;
}

void factoryReset() {
  if (!nvsReady || !preferences.clear()) {
    Serial.println("[RESET] Clear FAILED; defaults were not applied.");
    return;
  }
  settings = DEFAULT_SETTINGS;
  saveBlocked = false;
  dirty = true;
  Serial.println("[RESET] Namespace cleared. Factory defaults loaded.");
  if (!saveSettings()) {
    Serial.println("[RESET] Defaults active in RAM, but saving failed.");
  }
  printSettings();
}

void printSettings() {
  Serial.printf("brightness=%d, autoMode=%s, deviceName=\"%s\", "
                "threshold=%.2f, pending=%s\n",
                settings.brightness, settings.autoMode ? "ON" : "OFF",
                settings.deviceName, settings.threshold,
                dirty ? "YES" : "NO");
}

void printHelp() {
  Serial.println("Commands (send a newline):");
  Serial.println("  SHOW | BRIGHTNESS 0..100 | AUTO 0|1");
  Serial.println("  NAME <1..31 bytes> | THRESHOLD -1000..1000");
  Serial.println("  SAVE | REBOOT | FACTORY_RESET | HELP");
  Serial.println("Changes affect RAM immediately; SAVE writes flash.");
}

bool parseInteger(const String &text, long &value) {
  const char *start = text.c_str();
  char *end = nullptr;
  errno = 0;
  value = strtol(start, &end, 10);
  return end != start && *end == '\0' && errno != ERANGE;
}

bool parseFloatValue(const String &text, float &value) {
  const char *start = text.c_str();
  char *end = nullptr;
  errno = 0;
  value = strtof(start, &end);
  return end != start && *end == '\0' && errno != ERANGE && isfinite(value);
}

void handleCommand(String command) {
  command.trim();
  if (command.length() == 0) return;
  if (command == "SHOW") { printSettings(); return; }
  if (command == "HELP") { printHelp(); return; }
  if (command == "SAVE") { saveSettings(); return; }
  if (command == "FACTORY_RESET") { factoryReset(); return; }
  if (command == "REBOOT") {
    Serial.println(dirty ? "[BOOT] Rebooting WITHOUT saving RAM changes."
                         : "[BOOT] Rebooting...");
    if (nvsReady) preferences.end();
    Serial.flush();
    delay(100);
    ESP.restart();
    return;
  }

  DeviceSettings candidate = settings;
  long number;
  if (command.startsWith("BRIGHTNESS ")) {
    String value = command.substring(11); value.trim();
    if (!parseInteger(value, number) || number < 0 || number > 100) {
      Serial.println("[INPUT] Brightness must be an integer 0..100."); return;
    }
    candidate.brightness = static_cast<int>(number);
  } else if (command.startsWith("AUTO ")) {
    String value = command.substring(5); value.trim();
    if (!parseInteger(value, number) || (number != 0 && number != 1)) {
      Serial.println("[INPUT] AUTO must be 0 or 1."); return;
    }
    candidate.autoMode = (number == 1);
  } else if (command.startsWith("NAME ")) {
    String value = command.substring(5); value.trim();
    if (value.length() == 0 || value.length() >= sizeof(candidate.deviceName)) {
      Serial.println("[INPUT] Name must contain 1..31 bytes."); return;
    }
    memset(candidate.deviceName, 0, sizeof(candidate.deviceName));
    value.toCharArray(candidate.deviceName, sizeof(candidate.deviceName));
  } else if (command.startsWith("THRESHOLD ")) {
    String value = command.substring(10); value.trim();
    if (!parseFloatValue(value, candidate.threshold)) {
      Serial.println("[INPUT] Invalid finite number."); return;
    }
  } else {
    Serial.println("[INPUT] Unknown command. Type HELP."); return;
  }
  if (!validateSettings(candidate)) {
    Serial.println("[INPUT] Value outside the allowed range."); return;
  }
  // Compare fields, not raw structs: C++ structs can contain padding bytes.
  const bool changed = candidate.brightness != settings.brightness ||
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
  // Bound each pass so continuous serial input cannot starve the button.
  for (int count = 0; count < 64 && Serial.available(); ++count) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r') continue;
    if (c == '\n') {
      if (overflow) Serial.println("[INPUT] Line too long; discarded.");
      else handleCommand(line);
      line = "";
      overflow = false;
    } else if (!overflow) {
      if (line.length() < 95) line += c;
      else overflow = true;
    }
  }
}

// Button is active LOW. Debounce and hold timing are nonblocking.
int buttonSample = HIGH;
int buttonStable = HIGH;
uint32_t buttonChangedAt = 0;
uint32_t buttonPressedAt = 0;
bool buttonArmed = false;        // Require release after startup/reset action

void pollResetButton() {
  const uint32_t now = millis();
  const int sample = digitalRead(RESET_PIN);
  if (sample != buttonSample) {
    buttonSample = sample;
    buttonChangedAt = now;
  }
  if (sample != buttonStable && uint32_t(now - buttonChangedAt) >= 30) {
    buttonStable = sample;
    if (buttonStable == LOW && buttonArmed) {
      buttonPressedAt = now;
      Serial.println("[BUTTON] Keep holding BOOT for 5 seconds to reset.");
    }
  }
  if (buttonStable == HIGH) buttonArmed = true;
  if (buttonStable == LOW && buttonArmed &&
      uint32_t(now - buttonPressedAt) >= RESET_HOLD_MS) {
    buttonArmed = false;         // One action per continuous press
    factoryReset();
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);                   // Do not wait indefinitely for a USB host
  pinMode(RESET_PIN, INPUT_PULLUP);
  buttonSample = buttonStable = digitalRead(RESET_PIN);
  buttonChangedAt = millis();
  Serial.println("\nSaving User Settings on ESP32-S3");

  // false = read/write; default partition label is "nvs".
  nvsReady = preferences.begin(NVS_NAMESPACE, false);
  if (!nvsReady) {
    Serial.println("[NVS] Open failed. Using defaults in RAM only.");
  } else {
    const LoadStatus status = loadSettings();
    if (status == LoadStatus::Loaded) {
      Serial.println("[NVS] Valid settings loaded from flash.");
    } else if (status == LoadStatus::Missing || status == LoadStatus::Invalid) {
      Serial.println(status == LoadStatus::Missing
                       ? "[NVS] First boot / empty namespace. Using defaults."
                       : "[NVS] Invalid record. Restoring defaults.");
      settings = DEFAULT_SETTINGS;
      dirty = true;
      saveSettings();
    } else {
      // Do not silently overwrite another version or a failed read.
      saveBlocked = true;
      Serial.println("[NVS] Unreadable/incompatible record. Using RAM defaults;");
      Serial.println("      writes blocked. Diagnose or use FACTORY_RESET.");
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
```

## 7. Proving persistence

Send these commands one at a time:

```text
BRIGHTNESS 80
AUTO 0
NAME Workshop S3
THRESHOLD 30.5
SAVE
REBOOT
```

After reconnecting Serial Monitor, send `SHOW`. The four settings should retain their saved values. Then disconnect power completely, reconnect, and send `SHOW` again.

For comparison, change brightness without saving:

```text
BRIGHTNESS 20
REBOOT
```

It returns to **80**, because the RAM change to 20 was never saved.

### Example Serial Monitor output

The `>` lines below represent commands you enter, not prompts printed by the sketch. Initial boot messages may pass before a native USB Serial Monitor reconnects; use `SHOW` to inspect the current settings.

```text
Saving User Settings on ESP32-S3
[NVS] First boot / empty namespace. Using defaults.
[NVS] Saved 52-byte settings record.
brightness=50, autoMode=ON, deviceName="ESP32-S3", threshold=25.00, pending=NO

> BRIGHTNESS 80
brightness=80, autoMode=ON, deviceName="ESP32-S3", threshold=25.00, pending=YES
> SAVE
[NVS] Saved 52-byte settings record.
> REBOOT
[BOOT] Rebooting...

Saving User Settings on ESP32-S3
[NVS] Valid settings loaded from flash.
brightness=80, autoMode=ON, deviceName="ESP32-S3", threshold=25.00, pending=NO

> FACTORY_RESET
[RESET] Namespace cleared. Factory defaults loaded.
[NVS] Saved 52-byte settings record.
brightness=50, autoMode=ON, deviceName="ESP32-S3", threshold=25.00, pending=NO
```

You can also try `BRIGHTNESS 150`, `BRIGHTNESS abc`, or `THRESHOLD nan`; the sketch rejects them.

## 8. ESP-IDF equivalent using nvs_flash

The corresponding native sequence is:

| Arduino Preferences | ESP-IDF |
|---|---|
| Core startup initializes default NVS | `nvs_flash_init()` |
| `begin("my-app", false)` | `nvs_open("my-app", NVS_READWRITE, &handle)` |
| `getBytesLength()` / `getBytes()` | `nvs_get_blob()` first for size, then for data |
| `putBytes()` | `nvs_set_blob()` followed by `nvs_commit()` |
| `clear()` | `nvs_erase_all(handle)` followed by `nvs_commit()` |
| `end()` | `nvs_close(handle)` |

**ESP-IDF requires an explicit `nvs_commit()` after successful writes/deletions. Closing the handle does not replace it.** [1]

Use C++ (`main.cpp`) to share the structure and validation helpers. Place the Arduino sketch's `DeviceSettings`, `DEFAULT_SETTINGS`, `StoredSettings`, `SETTINGS_VERSION`, static assertions, and four helpers (`validateSettings`, `crc32`, `makeRecord`, `decodeRecord`) above the following code. Omit the Arduino/Preferences includes and globals. The downloadable `esp_idf_main.cpp` already contains the combined standalone source.

```cpp
#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "settings";

esp_err_t loadSettings(nvs_handle_t handle, DeviceSettings &out) {
  size_t length = 0;
  esp_err_t err = nvs_get_blob(handle, "settings", nullptr, &length);
  if (err != ESP_OK) return err;
  if (length != sizeof(StoredSettings)) return ESP_ERR_INVALID_SIZE;

  StoredSettings record{};
  err = nvs_get_blob(handle, "settings", &record, &length);
  if (err != ESP_OK) return err;
  if (record.version != SETTINGS_VERSION) return ESP_ERR_NOT_SUPPORTED;
  return decodeRecord(record, out) ? ESP_OK : ESP_ERR_INVALID_CRC;
}

esp_err_t saveSettings(nvs_handle_t handle, const DeviceSettings &settings) {
  if (!validateSettings(settings)) return ESP_ERR_INVALID_ARG;
  const StoredSettings record = makeRecord(settings);
  esp_err_t err = nvs_set_blob(handle, "settings", &record, sizeof(record));
  if (err != ESP_OK) return err;
  return nvs_commit(handle);     // Explicit commit is required in ESP-IDF
}

esp_err_t factoryReset(nvs_handle_t handle, DeviceSettings &settings) {
  esp_err_t err = nvs_erase_all(handle);  // Only this handle's namespace
  if (err != ESP_OK) return err;
  err = nvs_commit(handle);
  if (err != ESP_OK) return err;
  settings = DEFAULT_SETTINGS;
  return saveSettings(handle, settings);
}

extern "C" void app_main(void) {
  esp_err_t err = nvs_flash_init();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(err));
    // ESP_ERR_NVS_NO_FREE_PAGES / ESP_ERR_NVS_NEW_VERSION_FOUND
    // need deliberate recovery. This example preserves the partition.
    return;
  }

  nvs_handle_t handle;
  err = nvs_open("my-app", NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(err));
    return;
  }

  DeviceSettings settings = DEFAULT_SETTINGS;
  err = loadSettings(handle, settings);
  if (err == ESP_ERR_NVS_NOT_FOUND || err == ESP_ERR_INVALID_CRC) {
    ESP_LOGI(TAG, "Missing/invalid record; saving defaults");
    err = saveSettings(handle, settings);
  }
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Load/save failed: %s; keeping RAM defaults",
             esp_err_to_name(err));
    nvs_close(handle);           // Preserve incompatible/unreadable record
    return;
  }

  ESP_LOGI(TAG, "brightness=%d auto=%d name=%s threshold=%.2f",
           settings.brightness, settings.autoMode, settings.deviceName,
           static_cast<double>(settings.threshold));

  // In an application, call this when a user requests a save:
  // settings.brightness = 80;
  // err = saveSettings(handle, settings);
  // Check err before reporting success or calling esp_restart().

  // An explicit factory-reset action can call:
  // err = factoryReset(handle, settings);

  nvs_close(handle);             // Closing is not a substitute for commit
}
```

For an ESP-IDF project, save the combined source as `main/main.cpp` and use:

```cmake
idf_component_register(SRCS "main.cpp" INCLUDE_DIRS "." REQUIRES nvs_flash)
```

Then run `idf.py set-target esp32s3`, `idf.py build`, and `idf.py -p <PORT> flash monitor`. The native example loads/seeds settings; connect its save/reset functions to your application's UI events. Its storage format is compatible with the Arduino example when using the same NVS partition.

## 9. Best practices

### Namespaces, access modes, and handle lifetime

Keep namespace and key names short, descriptive, and consistent. Both are limited to **15 characters**. A namespace groups related keys; it is not a separate flash partition. [3]

```cpp
Preferences p;
if (p.begin("my-app", false)) {  // false: read/write
  p.putInt("brightness", 80);   // Check the return value in real code
  p.end();
}
```

Use `begin("my-app", true)` for read-only access to an existing namespace. Do not write through a read-only handle.

The full sketch keeps its namespace open because Serial and button actions may need it throughout the program. It calls `end()` before its controlled reboot. Opening/closing around occasional operations is also valid. `end()` releases the handle; it does not erase settings. [3]

### putBytes/getBytes versus individual keys

| Approach | Advantages | Tradeoffs |
|---|---|---|
| One versioned blob, as shown | One complete settings snapshot; simple record checksum | Whole record changes together; requires a stable layout and migration policy |
| `putInt`, `putBool`, `putString`, `putFloat` | Clear field names; can update only one setting; easier field-by-field evolution | Must validate each field; several calls are separate saves and can leave a mixture if interrupted |

Example of storing fields separately:

```cpp
preferences.putInt("brightness", settings.brightness);
preferences.putBool("auto", settings.autoMode);
preferences.putString("name", settings.deviceName);
preferences.putFloat("threshold", settings.threshold);

settings.brightness = preferences.getInt("brightness", 50);
```

Each call needs its own error check in production. A default returned by a getter does not prove that a valid stored value was found.

`getBytes()` returns raw bytes, not a self-describing C++ object. Check length before reading and version before interpreting the format. Never persist an Arduino `String`, a pointer, or an object containing pointers by copying its memory; its addresses are meaningless after reboot. Use a bounded character array or `putString()`. [8]

### Flash wear and interrupted saves

- Save after an explicit user action, or after a quiet period following edits.
- Coalesce rapid slider changes; do not save every intermediate value.
- Keep frequent counters/logs out of this settings record.
- Wait for a successful save before reporting persistence or restarting.
- Supply stable power during flash operations.

NVS is designed to recover from interrupted operations, but a save interrupted by power loss must not be treated as successful. A CRC does not guarantee that the latest update survives. Also, factory reset here consists of **clear, then save**: if power fails between them, the next boot sees a missing record and recreates defaults. [1]

### Handling full or unavailable NVS

Check `begin()` and every write. If saving fails, keep working settings in RAM and report that they are unsaved. Enable **Core Debug Level: Error** or a more verbose level to inspect Preferences diagnostics.

Native APIs expose useful errors, including `ESP_ERR_NVS_NOT_ENOUGH_SPACE` for a write and `ESP_ERR_NVS_NO_FREE_PAGES` or `ESP_ERR_NVS_NEW_VERSION_FOUND` during initialization. Diagnose partition layout, unused data, and format compatibility before choosing recovery. `freeEntries()` reports entries, not a guaranteed number of writable blob bytes. [1][8]

If needed, remove obsolete application keys, or allocate a larger/dedicated NVS partition. Increasing the board's flash-size setting alone does not enlarge the NVS partition.

**`nvs_flash_erase()` erases the default NVS partition, including other namespaces such as stored Wi-Fi configuration.** It is broader than this tutorial's factory reset. Do not add an automatic full-partition erase to every startup; reserve it for a deliberate recovery policy. [1]

### Versioning for future OTA updates

Treat `SETTINGS_VERSION` as a **storage-schema version**, independent of firmware release numbers.

When changing the stored layout:

1. Preserve the old record definition and its validation rules.
2. Recognize its version/size and load it using that definition.
3. Start the new settings from defaults, then copy compatible old fields.
4. Validate and save the new format only after successful conversion.
5. Consider rollback: older firmware may need to read the new format or preserve a separate compatible record.

An ordinary application OTA update generally preserves NVS when partition layout remains unchanged. Changing defaults in firmware does not change values already saved. This sketch preserves unsupported layouts/versions and blocks `SAVE`; it implements detection, not automatic migration. `FACTORY_RESET` deliberately discards that record.

## 10. Troubleshooting

| Symptom | Likely cause / action |
|---|---|
| `Preferences.h: No such file or directory` | Install/select the Espressif ESP32 board package and an ESP32-S3 target. |
| Settings disappear after reboot | Send `SAVE`, check its success message, and keep namespace/key names unchanged. |
| Defaults appear after uploading firmware | Check full-flash erase options, partition changes, and schema changes; ordinary RAM defaults are not proof of a failed save. |
| `begin()` fails | Check for the `nvs` partition, available storage, and debug output. |
| Write returns zero | Check read/write mode, key length/type, free space, and storage logs. |
| Incompatible-record message | Firmware expects a different version, size, or type. Migrate it, or deliberately run `FACTORY_RESET`. |
| CRC/validation failure | The matching-format record is invalid; defaults are restored. Repeated failures warrant checking supply stability and flash health. |
| No Serial output | Match connector and USB CDC setting; use 115200 baud for UART and verify the selected port. |
| Serial commands do nothing | Select Newline or Both NL & CR; commands are case-sensitive. |
| Holding BOOT at power-on does nothing | The chip may be in download mode. Release BOOT, press EN, then hold BOOT after startup. |
| Button resets repeatedly | This sketch prevents repeated reset during one hold. Check wiring/contact bounce if adapting the implementation. |
| Brightness setting changes but no LED changes | The tutorial saves values only. Add display/PWM control using the RAM settings. |

**Verification scope:** The Arduino source was syntax-checked with host API stubs and exercised for validation, save failures, simulated reloads, version preservation, and button timing. It has not been built with the ESP32 toolchain or hardware-tested here. Follow the reboot and complete power-cycle procedure on your board.

## esp_idf_main.cpp
This file is not as written for a normal Arduino IDE sketch. `esp_idf_main.cpp` was provided for a native ESP-IDF project.

The difference is the application entry point:

| Framework | Your application provides |
|---|---|
| Arduino ESP32 | `setup()` and `loop()` |
| Native ESP-IDF | `app_main()` |

Arduino’s ESP32 core already provides `app_main()` and uses it to initialize Arduino and run `setup()`/`loop()`. Defining your own can conflict with or bypass that startup. [GitHub](https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/main.cpp)

The underlying NVS functions can still be used under Arduino IDE. To adapt that file:

- Include `Arduino.h`.
- Remove its `app_main()` entry point.
- Move the startup/load logic into `setup()` and add `loop()`.
- Keep the storage functions, structures, and validation helpers.
- Prefer `Serial.printf()` for visible Serial Monitor messages.

Renaming `.cpp` to `.ino` alone is insufficient.

For your current Arduino project, use `SavingUserSettings.ino` from the tutorial. It already implements persistence, Serial commands, and both factory-reset methods using `Preferences`.

## References

1. Espressif: [ESP32-S3 Non-Volatile Storage](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/storage/nvs_flash.html)
2. Espressif: [EEPROM deprecation notice](https://github.com/espressif/arduino-esp32/blob/master/libraries/EEPROM/README.md)
3. Espressif: [Preferences API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/preferences.html)
4. Espressif: [Preferences implementation](https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/src/Preferences.cpp)
5. Espressif: [Arduino ESP32 troubleshooting](https://docs.espressif.com/projects/arduino-esp32/en/latest/troubleshooting.html)
6. Espressif: [ESP32-S3-DevKitC-1 user guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html)
7. Espressif: [ESP32-S3 GPIO reference](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/gpio.html)
8. Espressif: [Preferences tutorial](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html)
