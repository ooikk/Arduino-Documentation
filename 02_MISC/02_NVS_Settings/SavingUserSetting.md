# Saving User Settings in Non-Volatile Memory on ESP32-S3

**Hardware:** ESP32-S3 DevKitC-1  
**Framework:** Arduino for ESP32, with an ESP-IDF equivalent  
**Goal:** Preserve user-configurable settings across reboot and power loss.  
**Downloadable files:** `SavingUserSettings.ino`

---

# 1. Introduction: Why settings need NVS

Ordinary variables live in RAM. After a reboot, the program initializes them again. Removing power loses their contents. External PSRAM is also volatile.

Flash memory retains data without power. ESP32-S3 uses **Non-Volatile Storage (NVS)** to store small persistent values as key-value pairs inside a flash partition, normally labeled `nvs`.

NVS includes wear leveling. Updates append new entries, while obsolete entries are reclaimed later. This reduces flash erase frequency, although flash endurance remains finite. NVS is particularly suitable for small settings records. [ESP-IDF Programming Guide v6.1](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/storage/nvs_flash.html)

A practical application separates active settings from persistent settings:

- Load NVS into a RAM struct at startup.
- Use the RAM struct during normal operation.
- Modify RAM when the user changes a setting.
- Write to NVS when the user requests **Save**.

Changing a RAM variable does **not** automatically save it to flash.  
Reading settings does not consume flash program/erase endurance. Avoid writing them continuously in `loop()`.

---

# 2. Library choice

| Library            | Approach                                                                 | Recommended use                              |
|--------------------|--------------------------------------------------------------------------|----------------------------------------------|
| `Preferences.h`    | Arduino interface to NVS using namespaces and named keys                 | New Arduino ESP32 applications                 |
| `EEPROM.h`         | Emulates an EEPROM byte array using an NVS blob; requires `commit()`     | Existing legacy applications                   |
| `nvs.h` + `nvs_flash.h` | Native ESP-IDF API with handles and detailed error codes            | ESP-IDF applications or advanced storage control |

Espressif marks its Arduino ESP32 `EEPROM` library as deprecated and recommends `Preferences` for new applications. [arduino-esp32 EEPROM README](https://github.com/espressif/arduino-esp32/blob/master/libraries/EEPROM/README.md)

`Preferences` comes with the `esp32` by Espressif Systems board package. You do not need to install it separately.

For native ESP-IDF:

- `nvs_flash.h` provides partition initialization and erase operations.
- `nvs.h` provides key-value operations such as `nvs_open()` and `nvs_set_blob()`.

---

# 3. Settings structure and defaults

The application uses the requested structure:

```cpp
struct DeviceSettings {
  int brightness;       // 0–100
  bool autoMode;
  char deviceName[32];  // Maximum 31 bytes plus terminating '\0'
  float threshold;
};

const DeviceSettings DEFAULT_SETTINGS = {
  50,
  true,
  "ESP32-S3",
  25.0f
};
```

For this example, valid settings are:

| Field       | Validation                                                   |
|-------------|--------------------------------------------------------------|
| `brightness`| Integer from 0 to 100                                        |
| `autoMode`  | Stored representation must be 0 or 1                         |
| `deviceName`| Nonempty, null-terminated, maximum 31 bytes                  |
| `threshold` | Finite number between −1000 and +1000                        |

The `threshold` range is an example application rule. Replace it with limits appropriate to your sensor or control system.

## RAM structure versus stored record

The sketch converts `DeviceSettings` into a 52-byte stored record containing:

- A storage-format version.
- The four settings.
- CRC-32 for detecting accidental corruption.

```cpp
struct StoredSettings {
  uint32_t version;       // 4 bytes
  int32_t  brightness;    // 4 bytes
  uint8_t  autoMode;      // 1 byte
  uint8_t  reserved;      // 1 byte
  char     deviceName[32];// 32 bytes
  float    threshold;     // 4 bytes
  uint32_t crc;           // 4 bytes
};
```

The structure itself has an alignment requirement of 4 bytes because of `uint32_t`, `int32_t`, and `float`, the compiler rounds the total structure size up to the next multiple of 4. The `float threshold` requires **4-byte alignment** on ESP32-S3, so the compiler inserts **2 bytes of padding before threshold**.
The intended layout becomes: 


```text
Offset
0–3      version             4 bytes
4–7      brightness          4 bytes
8        autoMode            1 byte
9        reserved            1 byte
10–41    deviceName         32 bytes
42–43    padding             2 bytes
44–47    threshold           4 bytes
48–51    crc                 4 bytes
--------------------------------------
         sizeof()            52 bytes
```

and therefore:

```cpp
offsetof(StoredSettings, crc) == 48
sizeof(StoredSettings) == 52
```

This avoids blindly reading arbitrary flash bytes into a C++ `bool` and provides an explicit layout for future firmware updates.

The format is intended for ESP32-S3. For storage shared with other architectures, explicitly serialize byte order and floating-point representation.

> A CRC detects accidental corruption; it does not authenticate the data.

## Redefine padding as reserved byte
It is a good idea for a deliberately defined flash-storage format. You can explicitly reclaim the compiler's 2-byte padding by declaring it as a `uint8_t` array.
Instead of relying on implicit padding, you can make the layout explicit:

```cpp
struct StoredSettings {
  uint32_t version;          // 0–3
  int32_t brightness;        // 4–7
  uint8_t autoMode;          // 8
  uint8_t reserved;          // 9
  char deviceName[32];       // 10–41
  uint8_t reserved2[2];      // 42–43
  float threshold;           // 44–47
  uint32_t crc;              // 48–51
};
```

Then the layout becomes:

```text
Offset
0–3      version             4
4–7      brightness          4
8        autoMode            1
9        reserved            1
10–41    deviceName         32
42–43    reserved2           2
44–47    threshold           4
48–51    crc                 4
--------------------------------
         total              52
```

Making those two bytes explicit has advantages:
- The format is easier to understand.
- You can document exactly what every byte means.
- The two bytes can potentially be used in a future format revision.
- You are less dependent on the reader understanding compiler-generated padding.
- The `static_assert()` becomes a stronger check of your intended format.

For example, you could name them:
```cpp
uint8_t reserved2[2];
```

Therefore your initialization:

```cpp
StoredSettings r;
memset(&r, 0, sizeof(r));
```

is particularly useful because it guarantees:
```cpp
reserved     = 0
reserved2[0] = 0
reserved2[1] = 0
```

Your existing CRC calculation:
```cpp
r.crc = crc32(
  reinterpret_cast<const uint8_t *>(&r),
  offsetof(StoredSettings, crc)
);
```
will also include those two reserved bytes in the CRC.



---

# 4. Core logic

## On boot

The sketch opens the application namespace:

```cpp
preferences.begin("my-app", false);
```

Here, `false` means read/write access.

It then calls `loadSettings()` and handles the result:

| Result                          | Action                                                                 |
|---------------------------------|------------------------------------------------------------------------|
| Settings key missing            | Load defaults and save them                                            |
| Valid saved record              | Load settings into RAM                                                 |
| Matching format but invalid checksum/values | Restore and save defaults                                  |
| Unexpected version, type, or size | Use RAM defaults and preserve the existing record                   |
| Storage initialization/read failure | Report the failure without claiming persistence                    |

Preserving an unsupported format is useful after an OTA update or firmware rollback. The sketch blocks ordinary saves until the format is migrated or the user explicitly performs a factory reset.

## Loading

`loadSettings()` checks:

- Key existence.
- Stored type and length.
- Storage-format version.
- CRC.
- Field validity.

It updates the RAM settings only after validation succeeds.

## Saving on demand

For example:

```cpp
settings.brightness = 80;
dirty = true;

if (saveSettings()) {
  // It is now appropriate to report "Settings saved".
}
```

`Preferences` commits successful writes internally. There is no `Preferences.commit()` to call. The sketch checks that `putBytes()` returns the full record size. [Preferences.cpp](https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/src/Preferences.cpp)

---

# 5. Arduino setup and factory-reset button

## Arduino IDE setup

1. Install **esp32 by Espressif Systems** through Boards Manager.
2. Select **ESP32S3 Dev Module**, or your matching DevKitC-1 board definition.
3. Configure flash size and PSRAM according to the module installed on your board.
4. Select a partition scheme containing an NVS partition. Standard Arduino ESP32 schemes normally include one.
5. Upload the sketch.
6. Open Serial Monitor at **115200 baud**.
7. Select **Newline** or **Both NL & CR**.

NVS does not require PSRAM. A partition scheme needs OTA slots only if you also intend to use OTA. Flash capacity and NVS partition size are separate settings. [ESP-IDF Partition Tables](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/partition-tables.html)

For Serial output:

- **USB-to-UART connector:** Set **USB CDC On Boot: Disabled**.
- **Native USB connector:** Enable **USB CDC On Boot** and select the corresponding USB mode and port.

These settings determine where Arduino `Serial` sends its output. [Arduino ESP32 Troubleshooting](https://docs.espressif.com/projects/arduino-esp32/en/latest/troubleshooting.html)

## Method 1: BOOT button held for five seconds

DevKitC-1 already has a BOOT button connected to **GPIO 0**, active LOW. No additional wiring is needed.

For an external normally open button, connect:

| Button terminal | Connection |
|-----------------|------------|
| One side        | GPIO 0     |
| Other side      | GND        |

The sketch uses `INPUT_PULLUP`.

Let the application start normally, then hold BOOT for five seconds. GPIO 0 is a strapping pin; holding BOOT while resetting or powering up can select ROM download mode, where your sketch does not run. [ESP32-S3 DevKitC-1 User Guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html)

The implementation includes debounce, nonblocking timing, and one reset action per continuous press.

## Method 2: Serial command

Send:

```text
FACTORY_RESET
```

Both methods:

- Call `preferences.clear()`.
- Assign `DEFAULT_SETTINGS`.
- Save the defaults.

The defaults become active immediately. A reboot is unnecessary.

`clear()` removes keys from the currently open `my-app` namespace. It does not erase firmware or other namespaces. [Preferences API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/preferences.html)

---

# 6. Complete Arduino sketch

Create a sketch named `SavingUserSettings` and paste the following into `SavingUserSettings.ino`.

This example changes and saves configuration values. It does not drive a physical display, LED, or sensor.

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
  int brightness;               // 0–100
  bool autoMode;
  char deviceName[32];         // Up to 31 bytes plus '\0'
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
  char deviceName[32];
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
         s.deviceName[0] != '\0' &&
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

```

## 6.1 `static_assert()` in C++

`static_assert()` is a compile-time check in C++. It lets you tell the compiler:

> “This condition must always be true. If it isn't, stop compilation and show an error.”

It is not a function that runs on the ESP32. Nothing is executed at runtime.

### 1. Check the Size of `float`

```cpp
static_assert(sizeof(float) == 4,
              "This format requires a 32-bit float");
```

This checks:

```cpp
sizeof(float)
```

which tells you how many bytes a `float` occupies.

You require:

```text
float = 4 bytes = 32 bits
```

If the compiler confirms that:

```cpp
sizeof(float) == 4
```

the assertion passes and compilation continues.

If `float` were 8 bytes, compilation would fail. Conceptually, the compiler would report:

```text
error: static assertion failed:
This format requires a 32-bit float
```

This is useful because your NVS binary format assumes a 4-byte `float`.

### 2. Check Where `crc` Is Located

```cpp
static_assert(offsetof(StoredSettings, crc) == 48,
              "Storage layout changed");
```

`offsetof()` tells you the byte offset of a structure member.

With your explicit format:

```cpp
struct StoredSettings {
  uint32_t version;          // 0–3
  int32_t brightness;        // 4–7
  uint8_t autoMode;          // 8
  uint8_t reserved;          // 9
  char deviceName;           // 10–41
  uint8_t reserved2;         // 42–43
  float threshold;           // 44–47
  uint32_t crc;              // 48–51
};
```

you expect:

```cpp
offsetof(StoredSettings, crc)
```

to be:

```text
48
```

Therefore, the assertion is effectively checking:

```cpp
static_assert(48 == 48, "Storage layout changed");
```

The assertion passes.

However, suppose somebody later changes:

```cpp
char deviceName[32];
```

to:

```cpp
char deviceName[40];
```

The `crc` member would move from byte offset `48` to byte offset `56`.

The compiler would then stop with an error similar to:

```text
error: static assertion failed:
Storage layout changed
```

This is extremely useful for your NVS application because changing the structure layout can make existing flash data incompatible.

### 3. Check the Total Structure Size

```cpp
static_assert(sizeof(StoredSettings) == 52,
              "Storage layout changed");
```

This checks that the entire record is exactly 52 bytes.

Your intended format is:

```text
version        4 bytes
brightness     4 bytes
autoMode       1 byte
reserved       1 byte
deviceName    32 bytes
reserved2      2 bytes
threshold      4 bytes
crc            4 bytes
------------------------
              52 bytes
```

Therefore:

```cpp
sizeof(StoredSettings)
```

should return:

```text
52
```

If someone later changes the structure and it becomes 56 bytes, compilation fails instead of silently producing a different flash format.

### Why These Checks Are Useful for NVS

Your program stores the structure using:

```cpp
preferences.putBytes(
    SETTINGS_KEY,
    &record,
    sizeof(record)
);
```

You are effectively storing this binary structure in ESP32 NVS:

```text
ESP32 NVS

┌──────────────────────────────────────────────┐
│ version │ brightness │ ... │ threshold │ CRC │
└──────────────────────────────────────────────┘
                    52 bytes
```

Later, you read the structure using:

```cpp
preferences.getBytes(
    SETTINGS_KEY,
    &record,
    sizeof(record)
);
```

Therefore, your program depends on the structure having a known and stable layout.

The three assertions act as guard rails:

```text
             COMPILE TIME
                  │
                  ▼
       ┌──────────────────────┐
       │ static_assert checks │
       └──────────┬───────────┘
                  │
        ┌─────────┼─────────────┐
        ▼         ▼             ▼
     float      CRC offset   total size
      = 4         = 48         = 52
     bytes       bytes        bytes
        │         │             │
        └─────────┼─────────────┘
                  │
            All correct?
             /         \
           YES          NO
            │            │
            ▼            ▼
       Compile OK    Compile ERROR
```

### `static_assert` vs. Normal `assert`

This distinction is important.

#### `static_assert`

```cpp
static_assert(sizeof(StoredSettings) == 52,
              "Storage layout changed");
```

This check is performed during compilation.

#### Normal `assert`

```cpp
assert(sizeof(StoredSettings) == 52);
```

This is a runtime assertion. It is not what you want for validating a binary storage format.

Because `sizeof()` and `offsetof()` are known at compile time, `static_assert()` is the ideal choice.



## 6.2 Understanding `memchr()` in ESP32 C++ Code

`memchr()` is a C/C++ function used to search a block of memory for a particular byte.

In your ESP32 code, it is being used to answer:

> “Does `deviceName[32]` contain a null (`'\0'`) byte somewhere within its 32-byte buffer?”

### Basic Syntax

```cpp
memchr(memory, value, number_of_bytes);
```

For example:

```cpp
memchr(s.deviceName, '\0', sizeof(s.deviceName));
```

This means:

> Start at `s.deviceName` and search the next `sizeof(s.deviceName)` bytes for `'\0'`.

### 1. Why Use `memchr()`?

Your structure contains:

```cpp
char deviceName;
```

This is a fixed 32-byte buffer:

```text
deviceName

┌──────────────────────────────────────────────┐
│ 32 bytes                                     │
└──────────────────────────────────────────────┘
```

A normal C string must end with a null terminator:

```cpp
'\0'
```

For example:

```cpp
"ESP32-S3"
```

is actually stored in memory as:

```text
E S P 3 2 - S 3 \0
```

The `'\0'` tells functions such as `strlen()`, `strcmp()`, and:

```cpp
Serial.printf("%s");
```

where the string ends.

### 2. What Does This Code Do?

You have:

```cpp
memchr(s.deviceName, '\0', sizeof(s.deviceName)) != nullptr
```

Break it down as follows.

#### `s.deviceName`

This is the memory buffer being searched:

```cpp
char deviceName;
```

#### `'\0'`

This is the byte being searched for:

```text
0x00
```

#### `sizeof(s.deviceName)`

Because the field is declared as:

```cpp
char deviceName;
```

this expression evaluates to:

```text
32
```

Therefore:

```cpp
memchr(s.deviceName, '\0', 32)
```

means:

> Search all 32 bytes of `deviceName` for a zero byte.

### 3. What Does `memchr()` Return?

`memchr()` returns a pointer to the byte it finds.

If it finds the requested byte:

```text
E S P 3 2 - S 3 \0
              ↑
         Found byte
```

`memchr()` returns a pointer to the null byte.

If it does not find a null byte:

```text
E S P 3 2 - S 3 A B C D ...
```

it returns:

```cpp
nullptr
```

Therefore:

```cpp
memchr(...) != nullptr
```

means:

> A `'\0'` byte was found inside the buffer.

### 4. Why Not Use `strlen()`?

This is an important reason for using `memchr()`.

Suppose you have:

```cpp
char deviceName;
```

and the contents are corrupted:

```text
E S P 3 2 - S 3 A B C D E F ...
```

There is no null terminator within the 32-byte buffer.

If you call:

```cpp
strlen(s.deviceName);
```

`strlen()` does not know that the buffer is limited to 32 bytes.

It continues reading memory until it eventually finds a `'\0'`.

This can potentially read beyond the end of your structure.

`memchr()` is safer because you explicitly limit the search:

```cpp
sizeof(s.deviceName)
```

This restricts the search to exactly 32 bytes.

### 5. Complete Validation Function

Your validation function is:

```cpp
bool validateSettings(const DeviceSettings &s) {
  return s.brightness >= 0 &&
         s.brightness <= 100 &&
         s.deviceName != '\0' &&
         memchr(s.deviceName,
                '\0',
                sizeof(s.deviceName)) != nullptr &&
         isfinite(s.threshold) &&
         s.threshold >= -1000.0f &&
         s.threshold <= 1000.0f;
}
```

There are two different checks involving `deviceName`.

#### Check 1: The Name Is Not Empty

```cpp
s.deviceName != '\0'
```

This means:

> The first character cannot be `'\0'`.

Therefore, the name cannot be empty.

```text
""          → INVALID
"\0"        → INVALID
"ESP32-S3"  → VALID
```

#### Check 2: The Buffer Contains a Terminator

```cpp
memchr(s.deviceName,
       '\0',
       sizeof(s.deviceName)) != nullptr
```

This means:

> There must be a null terminator somewhere within the 32-byte buffer.

For example:

```text
E S P 3 2 - S 3 \0
↑               ↑
byte 0          terminator
```

This is valid.

However:

```text
E S P 3 2 - S 3 A B C D E F G ...
```

If all 32 bytes are occupied and there is no `'\0'`, the value is invalid.

### 6. Why This Matters When Reading NVS

Your NVS contains binary data:

```cpp
StoredSettings record{};
```

You cannot blindly assume that the `deviceName` field is a valid C string.

For example, corrupted flash could theoretically contain:

```text
deviceName:

E S P 3 2 - S 3 X X X X X X X X ...
```

with no terminating null byte.

If you then execute:

```cpp
Serial.printf("%s", candidate.deviceName);
```

the function could continue reading beyond the end of the `deviceName` field.

This is an out-of-bounds string read.

Your validation check protects against this:

```cpp
memchr(s.deviceName,
       '\0',
       sizeof(s.deviceName)) != nullptr
```

It requires the null terminator to exist inside the


## 6.3 Understanding the CRC-32 Function

This function calculates a CRC-32 checksum for a block of bytes.

In your NVS project, its purpose is to detect whether the saved `StoredSettings` record has been changed or corrupted.

```cpp
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
```

### 1. What Is CRC-32?

CRC stands for **Cyclic Redundancy Check**.

You can think of it as a fingerprint for binary data.

Suppose your ESP32 stores:

```text
brightness = 50
autoMode   = true
deviceName = "ESP32-S3"
threshold  = 25.0
```

The actual NVS record is stored as binary data:

```text
┌──────────────────────────────────────────────┐
│ version │ brightness │ ... │ threshold │ CRC │
└──────────────────────────────────────────────┘
                                      ↑
                                  checksum
```

The CRC is calculated from all data before the CRC field:

```text
Data
  ↓
CRC-32 algorithm
  ↓
0xXXXXXXXX
```

That CRC value is then stored together with the record.

When you later read the record:

```text
Flash
  ↓
StoredSettings
  ↓
Calculate CRC again
  ↓
Compare with stored CRC
```

If the two CRC values do not match, something changed.

### 2. Why Do We Need CRC?

Imagine the ESP32 saved:

```text
brightness = 50
autoMode   = 1
deviceName = "ESP32-S3"
threshold  = 25.0
```

and calculated:

```text
CRC = 0x12345678
```

Later, suppose one byte in flash becomes corrupted:

```text
brightness = 51
```

The newly calculated CRC will almost certainly be different:

```text
Stored CRC:       0x12345678
Calculated CRC:   0x8A.......
```

Therefore:

```text
stored CRC != calculated CRC
```

and your program rejects the record.

CRC is a data-integrity check. It is not encryption.

### 3. Function Declaration

```cpp
uint32_t crc32(const uint8_t *data, size_t length)
```

There are three important parts.

#### `uint32_t`

The function returns a 32-bit unsigned integer:

```text
32 bits = 4 bytes
```

This is the CRC-32 value.

#### `const uint8_t *data`

This is a pointer to the data being checked.

For example:

```cpp
crc32(
  reinterpret_cast<const uint8_t *>(&r),
  offsetof(StoredSettings, crc)
);
```

The `data` pointer points to the first byte of `StoredSettings`.

Conceptually:

```text
data
 ↓
┌──────────────────────────────────────┐
│ version │ brightness │ ... threshold │
└──────────────────────────────────────┘
```

`uint8_t` means that the structure is processed one byte at a time.

#### `size_t length`

This tells the function how many bytes to process.

In your code:

```cpp
offsetof(StoredSettings, crc)
```

is:

```text
48
```

Therefore:

```cpp
crc32(data, 48)
```

means:

> Calculate the CRC over the first 48 bytes.

The CRC field itself is not included.

### 4. Initial CRC Value

```cpp
uint32_t crc = 0xFFFFFFFFu;
```

The CRC starts with:

```text
FFFFFFFF
```

in hexadecimal.

That is:

```text
11111111 11111111 11111111 11111111
```

The `u` suffix means that the constant is an unsigned integer.

This initial value is part of the CRC-32 algorithm being used.

### 5. Process Every Byte

```cpp
while (length--) {
```

This keeps processing bytes until `length` becomes zero.

Suppose:

```cpp
length = 48;
```

The loop processes:

```text
byte 0
byte 1
byte 2
...
byte 47
```

That is a total of 48 bytes.

### 6. Get the Current Byte

```cpp
crc ^= *data++;
```

This line combines two operations.

#### `*data`

This means:

> Get the byte currently pointed to by `data`.

For example:

```text
data
 ↓
A5  34  78  12 ...
↑
current byte
```

The value of `*data` is:

```text
0xA5
```

#### `data++`

This moves the pointer to the next byte.

After processing `0xA5`:

```text
A5  34  78  12
    ↑
  data
```

The pointer now refers to `0x34`.

#### `^=`

The `^=` operator means XOR and assign.

This:

```cpp
crc ^= *data;
```

is equivalent to:

```cpp
crc = crc ^ *data;
```

The current byte is XORed into the CRC.

For example:

```text
CRC before:    FFFFFFFF
Current byte:  000000A5
               --------
XOR result:    FFFFFF5A
```

The internal CRC value continues changing as the algorithm processes each bit.

### 7. Why Process Eight Bits?

```cpp
for (int bit = 0; bit < 8; ++bit)
```

There are 8 bits in every byte.

For example:

```text
A5 = 10100101
     ↑↑↑↑↑↑↑↑
      8 bits
```

CRC-32 processes each bit individually.

For every byte, it processes:

```text
bit 0
bit 1
bit 2
bit 3
bit 4
bit 5
bit 6
bit 7
```

It then moves to the next byte.

### 8. The Main CRC Operation

```cpp
crc = (crc >> 1) ^
      ((crc & 1u) ? 0xEDB88320u : 0u);
```

This is the core of the CRC algorithm.

#### `crc & 1u`

This checks the least significant bit of the CRC.

For example:

```text
CRC = 10110110
             ↑
           bit 0
```

If the last bit is zero:

```cpp
crc & 1u == 0
```

If the last bit is one:

```cpp
crc & 1u == 1
```

The code is asking:

> Is the lowest bit of the CRC currently zero or one?

### 9. `crc >> 1`

This shifts the CRC right by one bit.

Before:

```text
10110110
```

After:

```text
01011011
```

For a 32-bit CRC:

```text
Step 1: Right shift

Before:
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                               ↓
                         discarded

After:
0xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
↑
new zero
```

The rightmost bit is removed, and a zero enters from the left.

### 10. Meaning of `0xEDB88320`

```cpp
0xEDB88320u
```

This is the CRC-32 polynomial represented in the form used by this right-shifting implementation.

It is a standard constant associated with the widely used CRC-32 algorithm.

The algorithm uses it when the outgoing bit is one.

### 11. The Conditional Operator

This part:

```cpp
((crc & 1u) ? 0xEDB88320u : 0u)
```

uses the conditional operator.

It means:

```cpp
if (crc & 1u) {
  use 0xEDB88320u;
} else {
  use 0u;
}
```

The complete operation can therefore be written more explicitly as:

```cpp
if (crc & 1u) {
  crc = (crc >> 1) ^ 0xEDB88320u;
} else {
  crc = crc >> 1;
}
```

The original code combines both cases into one line.

### 12. Invert the CRC

After every byte and every bit has been processed:

```cpp
return ~crc;
```

The `~` operator performs a bitwise NOT operation.

For example:

```text
crc:

10110010

~crc:

01001101
```

For 32 bits:

```text
FFFFFFFF
   ↓ ~
00000000
```

This final inversion is also part of the CRC-32 algorithm used here.

### 13. The Complete Algorithm

You can visualize the algorithm like this:

```text
                StoredSettings
                     │
                     ▼
              48 bytes of data
                     │
                     ▼
          ┌─────────────────────┐
          │ Initial CRC         │
          │ 0xFFFFFFFF          │
          └──────────┬──────────┘
                     │
                     ▼
                Take one byte
                     │
                     ▼
                XOR with CRC
                     │
                     ▼
          Process 8 individual bits
                     │
                     ├── LSB = 0
                     │      │
                     │      ▼
                     │   Shift right
                     │
                     └── LSB = 1
                            │
                            ▼
                    Shift right
                            +
                     XOR polynomial
                     0xEDB88320
                            │
                            ▼
                         Next bit
                            │
                         ... 8 bits
                            │
                            ▼
                       Next byte
                            │
                         ... 48 bytes
                            │
                            ▼
                           ~crc
                            │
                            ▼
                     32-bit CRC value
```

### 14. How It Fits Into the NVS Code

When saving the record:

```cpp
r.crc = crc32(
  reinterpret_cast<const uint8_t *>(&r),
  offsetof(StoredSettings, crc)
);
```

Your structure is arranged like this:

```text
0                       48                 52
│                        │                  │
▼                        ▼                  ▼
┌────────────────────────┬──────────────────┐
│          DATA          │       CRC        │
│        48 bytes        │      4 bytes     │
└────────────────────────┴──────────────────┘
                         ↑
                     CRC starts here
```

The program:

1. Calculates the CRC over data bytes 0 through 47.
2. Stores the result in bytes 48 through 51.
3. Writes the complete 52-byte record to NVS.

In other words:

```text
CRC(data bytes 0–47)
```

is stored in:

```text
bytes 48–51
```

### 15. Reading the Record Back

Your code checks the record using:

```cpp
if (r.version != SETTINGS_VERSION ||
    r.autoMode > 1 ||
    r.crc != crc32(
      reinterpret_cast<const uint8_t *>(&r),
      offsetof(StoredSettings, crc))) {
  return false;
}
```

The important comparison is:

```cpp
r.crc != calculated_crc
```

For example:

```text
Flash record:

Stored CRC:       0xA73F291C
Recalculated CRC: 0xA73F291C
                   ──────────
                      MATCH
```

The record is probably intact.

If one byte was corrupted:

```text
Stored CRC:       0xA73F291C
Recalculated CRC: 0x61B8D442
                   ──────────
                     DIFFERENT
```

Then:

```cpp
decodeRecord()
```

returns:

```cpp
false
```

and your program does not trust the record.

### Important Limitation

CRC provides error detection, not security.

It can detect accidental problems such as:

- Flash data corruption.
- Incomplete or incorrect data.
- Unexpected modifications.
- Memory or data errors.

However, CRC cannot protect against someone deliberately modifying the NVS data and recalculating the CRC.

For your ESP32 user-settings NVS tutorial, CRC-32 is therefore a good choice for integrity checking. It should not be described as encryption or tamper-proof protection.

### Summary

Your function takes the 48 bytes of settings data, processes every bit using the CRC-32 algorithm, and produces a 4-byte fingerprint.

That fingerprint is stored in the final 4 bytes of your 52-byte NVS record.

When loading the record, the program calculates the CRC again and compares the new value with the stored value:

```text
Matching CRCs:
Record is probably valid.

Different CRCs:
Record is rejected as corrupted or invalid.
```


## 6.4 Understanding `reinterpret_cast`

`reinterpret_cast` is a C++ type conversion that tells the compiler to treat the same memory address as a different type.

In your CRC code:

```cpp
reinterpret_cast<const uint8_t *>(&r)
```

it means:

> Take the address of the `StoredSettings` structure `r` and treat that address as a pointer to raw bytes (`uint8_t`).

This is useful for CRC calculation because CRC works on bytes, while `r` is a structure.

### 1. Start with `r`

You have:

```cpp
StoredSettings r;
```

For example, your structure occupies 52 bytes:

```text
StoredSettings r

┌──────────────────────────────────────────────┐
│ version                                      │
│ brightness                                   │
│ autoMode                                     │
│ reserved                                     │
│ deviceName                                   │
│ reserved2                                    │
│ threshold                                    │
│ crc                                          │
└──────────────────────────────────────────────┘
                  52 bytes
```

The variable `r` represents the complete structure.

### 2. What Does `&r` Mean?

The `&` operator means “address of”.

Therefore:

```cpp
&r
```

means:

> Give me the memory address where `r` starts.

For example, imagine:

```text
Address
0x3FC90000
     │
     ▼
┌───────────────────────────┐
│ StoredSettings r          │
│                           │
│ byte 0                    │
│ byte 1                    │
│ byte 2                    │
│ ...                       │
│ byte 51                   │
└───────────────────────────┘
```

Then:

```cpp
&r
```

might be:

```text
0x3FC90000
```

However, its type is:

```cpp
StoredSettings *
```

because it is a pointer to a `StoredSettings` object.

### 3. Why Cannot CRC Use `&r` Directly?

Your CRC function expects:

```cpp
uint32_t crc32(const uint8_t *data, size_t length)
```

Notice the first parameter:

```cpp
const uint8_t *data
```

It requires a pointer to bytes.

However, `&r` has the type:

```cpp
StoredSettings *
```

These are different pointer types.

Conceptually:

```text
&r
 │
 ▼
StoredSettings *
```

while the CRC function wants:

```text
const uint8_t *
```

Therefore, you explicitly convert the pointer.

### 4. What Does `reinterpret_cast` Do?

```cpp
reinterpret_cast<const uint8_t *>(&r)
```

Break it down:

```cpp
reinterpret_cast<
    const uint8_t *
>(
    &r
)
```

This means:

> Take the address `&r` and reinterpret that address as a pointer to `const uint8_t`.

Before the conversion:

```text
&r
 │
 ▼
StoredSettings *
```

After the conversion:

```text
reinterpret_cast<const uint8_t *>(&r)
 │
 ▼
const uint8_t *
```

The memory itself does not change.

That is the most important point.

### 5. It Does Not Convert the Structure into New Bytes

A common misunderstanding is that `reinterpret_cast` converts the structure into a new array of bytes.

It does not do this:

```text
StoredSettings
       │
       │ conversion
       ▼
52 new uint8_t values
```

Instead, it does this:

```text
                 Same memory
                      │
                      ▼
┌──────────────────────────────────────────┐
│ StoredSettings r                         │
│                                          │
│ 52 bytes of existing memory              │
└──────────────────────────────────────────┘
          ▲                         ▲
          │                         │
     StoredSettings *          uint8_t *
```

You are simply viewing the same memory through a different pointer type.

### 6. Why Is This Useful for CRC?

Your structure might contain the following fields:

```text
byte 0–3       version
byte 4–7       brightness
byte 8         autoMode
byte 9         reserved
byte 10–41     deviceName
byte 42–43     reserved2
byte 44–47     threshold
byte 48–51     crc
```

CRC does not care that bytes 0 through 3 represent an integer or that bytes 44 through 47 represent a floating-point value.

It simply sees:

```text
byte 0
byte 1
byte 2
...
byte 47
```

Therefore:

```cpp
crc32(
  reinterpret_cast<const uint8_t *>(&r),
  offsetof(StoredSettings, crc)
);
```

means:

> Start at the first byte of `r`, treat the structure as a sequence of bytes, and calculate the CRC over the first 48 bytes.

Visually:

```text
r
│
│ reinterpret_cast
▼
uint8_t *
│
▼
┌────────────────────────────────────────────────┐
│ 00 01 02 03 04 05 ... 44 45 46 47 │ 48...51    │
└────────────────────────────────────────────────┘
◄────────────── CRC input ────────────►  CRC
```

The CRC input length is:

```cpp
offsetof(StoredSettings, crc)
```

which is:

```text
48 bytes
```

Therefore, bytes 0 through 47 are processed.

### 7. What Does `const` Mean?

The target type is:

```cpp
const uint8_t *
```

rather than:

```cpp
uint8_t *
```

The `const` means:

> The CRC function is allowed to read these bytes, but it cannot modify them through this pointer.

That is appropriate because calculating the CRC should not modify `r`.

This is allowed:

```cpp
const uint8_t *data = ...;

uint8_t x = *data;
```

This is not allowed:

```cpp
*data = 123;
```

The second statement attempts to modify data through a pointer to `const`.

### 8. Why Use `uint8_t`?

`uint8_t` represents an unsigned 8-bit integer.

On the ESP32:

```text
uint8_t = 1 byte = 8 bits
```

Therefore:

```cpp
const uint8_t *
```

can be understood as:

> A pointer to individual raw bytes that the function is allowed to read only.

That is exactly what a CRC routine needs.

### Putting the Expression Together

This expression:

```cpp
reinterpret_cast<const uint8_t *>(&r)
```

can be read in plain English as:

> Take the address of `r` and treat that memory as a read-only sequence of 8-bit bytes.

Then:

```cpp
crc32(
  reinterpret_cast<const uint8_t *>(&r),
  offsetof(StoredSettings, crc)
);
```

means:

> Calculate a CRC-32 starting at the first byte of `r` and process the first 48 bytes.

### Useful Mental Model

Think of `reinterpret_cast` as changing your view of the memory, not changing the memory itself:

```text
                 Same memory
                     │
        ┌────────────┴────────────┐
        │                         │
        ▼                         ▼
 StoredSettings *             uint8_t *
 “view as structure”          “view as bytes”
```

This is a common embedded-systems technique when working with:

- Binary storage.
- NVS blobs.
- EEPROM.
- Flash records.
- Network packets.
- CRC calculations.


## 6.5 Understanding `0xEDB88320u` in CRC-32

Yes, you can change `0xEDB88320u`, but you will no longer be calculating the same CRC-32 algorithm.

The value is not an arbitrary constant. It represents the CRC polynomial used by the mathematical algorithm.

Your code contains:

```cpp
crc = (crc >> 1) ^
      ((crc & 1u) ? 0xEDB88320u : 0u);
```

The value:

```text
0xEDB88320
```

is a specific polynomial representation.

### 1. Mathematical Meaning

CRC is based on polynomial arithmetic over \( GF(2) \), where addition is equivalent to XOR.

The standard CRC-32 polynomial is commonly written as:


$$P(x)=x^{32}+x^{26}+x^{23}+x^{22}+x^{16}+x^{12}+x^{11}+x^{10}+x^8+x^7+x^5+x^4+x^2+x+1$$


This polynomial is commonly represented as:

```text
0x04C11DB7
```

However, your implementation processes bits least-significant-bit first, or LSB-first. It therefore uses the reflected representation:

```text
0xEDB88320
```

Therefore:

```text
0xEDB88320
```

is not a random magic number. It encodes the mathematical feedback polynomial used by this particular CRC implementation.

### 2. Why Is It XORed?

Consider this part of the algorithm:

```cpp
if (crc & 1u) {
  crc = (crc >> 1) ^ 0xEDB88320u;
} else {
  crc = crc >> 1;
}
```

The least significant bit tells the algorithm whether the polynomial division requires a correction step.

Conceptually:

```text
              Is outgoing bit 1?
                     │
             ┌───────┴───────┐
             │               │
            NO              YES
             │               │
             ▼               ▼
        Shift only       Shift + XOR
                            │
                            ▼
                       Polynomial
                      0xEDB88320
```

This performs polynomial division without using expensive division operations.

### 3. What Happens If You Change It?

Suppose you change:

```cpp
0xEDB88320u
```

to:

```cpp
0x12345678u
```

The code will still compile and run.

However, you have changed the generator polynomial. You have therefore created a different CRC algorithm.

For example:

```text
CRC_A:
polynomial = 0xEDB88320

CRC_B:
polynomial = 0x12345678
```

The same data will generally produce completely different CRC values:

```text
Same data
    │
    ├── CRC_A → 0x........
    │
    └── CRC_B → 0x........
```

Neither is necessarily mathematically “wrong”, but they are different CRC schemes.

### 4. Why This Matters for NVS Settings

This is particularly important in your ESP32 project.

Suppose you save the settings using:

```text
Settings data
     ↓
CRC using 0xEDB88320
     ↓
0xA37B9214
```

Later, you load the record and calculate the CRC using:

```text
0x12345678
```

You will almost certainly get a different result:

```text
Calculated CRC = 0x????????   ← different
Stored CRC     = 0xA37B9214
```

Therefore:

```text
calculated CRC != stored CRC
```

and your settings will be rejected as corrupted.

The polynomial must remain identical when saving and loading.

### 5. Can You Design Your Own CRC?

Yes, you could theoretically choose another polynomial.

However, you should not simply select an arbitrary number.

A CRC polynomial has mathematical properties that affect its ability to detect:

- Single-bit errors.
- Multiple-bit errors.
- Burst errors.
- Certain patterns of corrupted data.

Good CRC polynomials are carefully selected because they provide strong error-detection properties.

Therefore:

```text
0xEDB88320
```

is not merely a convenient constant. It is part of the well-known CRC-32/IEEE 802.3 algorithm.

### 6. CRC-32 Has More Than a Polynomial

It is important not to think of CRC-32 as only a polynomial.

A CRC algorithm is defined by several parameters:

```text
Width      = 32 bits
Polynomial = 0x04C11DB7
Init       = 0xFFFFFFFF
RefIn      = true
RefOut     = true
XorOut     = 0xFFFFFFFF
```

Your implementation corresponds to the common reflected CRC-32 variant.

That is why the function begins with:

```cpp
uint32_t crc = 0xFFFFFFFFu;
```

and ends with:

```cpp
return ~crc;
```

The complete mathematical recipe matters.

### 7. Why `0x04C11DB7` and `0xEDB88320`?

This is an important point.

The conventional CRC-32 polynomial is:

```text
0x04C11DB7
```

Your algorithm shifts right:

```cpp
crc >> 1
```

Therefore, it uses the reflected polynomial:

```text
0xEDB88320
```

You can think of the relationship approximately as bit reflection:

```text
0x04C11DB7
      ↓ bit reflection
0xEDB88320
```

The two constants represent the same underlying CRC polynomial in different orientations.

Therefore, you should not simply replace:

```cpp
0xEDB88320u
```

with:

```cpp
0x04C11DB7u
```

while keeping:

```cpp
crc >> 1
```

That would mix two different representations.

### 8. Mathematical Model

A simple conceptual model is:

```text
             CRC register
        ┌───────────────────┐
        │ 32-bit remainder  │
        └───────────────────┘
                  │
                  ▼
             Shift right
                  │
                  ▼
          Outgoing bit = 1?
             /          \
           NO            YES
           │              │
           ▼              ▼
       Continue       XOR with
                    polynomial
                   0xEDB88320
```

The polynomial determines how the CRC remainder evolves after each bit.

Therefore:

> Changing `0xEDB88320` changes the mathematical feedback rule, which changes the CRC algorithm and the resulting checksum.

### Recommendation for Your ESP32 Tutorial

Keep the constant unchanged:

```cpp
0xEDB88320u
```

You can describe it as:

> The reflected representation of the standard CRC-32/IEEE 802.3 generator polynomial. It determines the mathematical feedback operation used during each bit iteration.

This is more accurate than calling it simply a “CRC constant” or “magic number”.

## 6.6 `DeviceSettings candidate{};` vs. `DeviceSettings candidate;`

The difference is initialization.

These two declarations look almost identical:

```cpp
DeviceSettings candidate{};
```

and:

```cpp
DeviceSettings candidate;
```

However, they behave differently.

### 1. `DeviceSettings candidate{};`

The `{}` syntax value-initializes the object.

For your structure:

```cpp
struct DeviceSettings {
  int brightness;
  bool autoMode;
  char deviceName;
  float threshold;
};
```

this declaration:

```cpp
DeviceSettings candidate{};
```

initializes every field to zero or its equivalent:

```text
brightness = 0
autoMode   = false
deviceName = all '\0'
threshold  = 0.0
```

You can visualize it like this:

```text
candidate

┌──────────────────────┐
│ brightness = 0       │
│ autoMode   = false   │
│ deviceName = ""      │
│ threshold  = 0.0     │
└──────────────────────┘
```

For the character array:

```cpp
char deviceName;
```

all 32 bytes are initialized to zero:

```text
00 00 00 00 00 00 ... 00
```

### 2. `DeviceSettings candidate;`

This declaration creates the variable without initializing its contents.

For a local variable inside a function:

```cpp
void test() {
  DeviceSettings candidate;
}
```

the fields contain indeterminate values:

```text
candidate

┌──────────────────────┐
│ brightness = ?????   │
│ autoMode   = ?????   │
│ deviceName = ?????   │
│ threshold  = ?????   │
└──────────────────────┘
```

You must assign every field before relying on its value.

Reading an uninitialized field can produce unpredictable results and, in some cases, undefined behavior.

### Why Use `{}` in NVS Code?

Suppose you have:

```cpp
DeviceSettings candidate{};
```

and then decode the stored NVS record:

```cpp
candidate.brightness = r.brightness;
candidate.autoMode = r.autoMode;

strncpy(
  candidate.deviceName,
  r.deviceName,
  sizeof(candidate.deviceName)
);

candidate.threshold = r.threshold;
```

Using `{}` gives you a known, clean starting state.

This is especially useful for structures containing arrays such as:

```cpp
char deviceName;
```

Before copying data, the entire array contains zeros:

```text
00 00 00 00 00 00 ... 00
```

### 3. Difference from `DEFAULT_SETTINGS`

You may have defined:

```cpp
const DeviceSettings DEFAULT_SETTINGS = {
  50,
  true,
  "ESP32-S3",
  25.0f
};
```

Here, every field is explicitly initialized, so `{}` is not necessary.

However:

```cpp
DeviceSettings candidate{};
```

does not mean:

```cpp
candidate = DEFAULT_SETTINGS;
```

It produces these values:

```text
brightness → 0
autoMode   → false
name       → ""
threshold  → 0
```

It does not produce these default values:

```text
brightness → 50
autoMode   → true
name       → "ESP32-S3"
threshold  → 25.0
```

If you want to initialize the object with your defined defaults, write:

```cpp
DeviceSettings candidate = DEFAULT_SETTINGS;
```

### 4. Why Not Always Use `{}`?

For local variables, using:

```cpp
DeviceSettings candidate{};
```

is generally a good defensive programming habit when you want a known initial state.

It is safer than:

```cpp
DeviceSettings candidate;
```

because you cannot accidentally use an uninitialized field before assigning it.

For your NVS decoder, this is preferable:

```cpp
DeviceSettings candidate{};
```

The decoder is converting and validating stored data, so having a deterministic initial state is desirable.

### Simple Rule

| Declaration | Initial state |
|---|---|
| `DeviceSettings candidate;` | Uninitialized local object |
| `DeviceSettings candidate{};` | All fields zero-initialized |
| `DeviceSettings candidate = DEFAULT_SETTINGS;` | Initialized with your defined defaults |

The `{}` syntax is not specific to ESP32 or NVS. It is standard C++ initialization syntax.

In embedded code, it is particularly useful because it prevents your program from accidentally working with unpredictable memory contents.


## 6.7 Understanding `loadSettings()` and `saveSettings()`

These two functions are the core of your NVS settings system.

A simple way to remember them is:

```text
loadSettings()

NVS Flash ──────────► RAM
```

```text
saveSettings()

RAM ────────────────► NVS Flash
```

### 1. `loadSettings()`

```cpp
LoadStatus loadSettings()
```

This function reads saved settings from NVS, checks that they are valid, and then places them into the global `settings` variable.

#### Step 1: Check Whether NVS Is Available

```cpp
if (!nvsReady) {
  return LoadStatus::ReadError;
}
```

If NVS was not successfully initialized, the function stops immediately:

```text
NVS unavailable
      ↓
ReadError
```

#### Step 2: Check Whether the Settings Key Exists

```cpp
if (!preferences.isKey(SETTINGS_KEY)) {
  return LoadStatus::Missing;
}
```

This checks whether the `"settings"` key has ever been saved.

On the first boot, the key may not exist:

```text
No "settings" key
       ↓
    Missing
```

The caller can then use the default settings.

#### Step 3: Check the Stored Type and Size

```cpp
if (preferences.getType(SETTINGS_KEY) != PT_BLOB ||
    preferences.getBytesLength(SETTINGS_KEY) !=
      sizeof(StoredSettings)) {
  return LoadStatus::Incompatible;
}
```

Your settings are stored as a BLOB, which means raw binary data.

The code also checks that the stored size is exactly:

```cpp
sizeof(StoredSettings)
```

In your design, this is:

```text
52 bytes
```

The check is conceptually:

```text
Expected:

BLOB
52 bytes

        ↓

NVS:

Is it a BLOB?
Is it 52 bytes?

        ↓
      YES → Continue
       NO → Incompatible
```

This protects you from an old or incompatible storage format.

#### Step 4: Create an Empty Record

```cpp
StoredSettings record{};
```

This creates a local `StoredSettings` structure and initializes all its fields to zero.

The structure temporarily holds the data read from NVS.

```text
NVS
 ↓
record
 ↓
settings
```

#### Step 5: Read the NVS Data

```cpp
if (preferences.getBytes(
      SETTINGS_KEY,
      &record,
      sizeof(record)
    ) != sizeof(record)) {
  return LoadStatus::ReadError;
}
```

This reads the 52-byte NVS record into:

```cpp
record
```

The expression:

```cpp
&record
```

means:

> Give `getBytes()` the memory address where it should place the data.

The function expects exactly 52 bytes to be read.

If it receives fewer than 52 bytes:

```text
ReadError
```

#### Step 6: Check the Version

```cpp
if (record.version != SETTINGS_VERSION) {
  return LoadStatus::Incompatible;
}
```

The stored record contains:

```cpp
uint32_t version;
```

For example:

```text
Current version = 1
```

If the ESP32 finds:

```text
Stored version = 2
Current version = 1
```

it does not try to interpret the data.

Instead:

```text
Different format
      ↓
Incompatible
```

This allows you to change the storage format in future firmware versions.

#### Step 7: Validate and Decode the Record

```cpp
if (!decodeRecord(record, settings)) {
  return LoadStatus::Invalid;
}
```

This is an important step.

`decodeRecord()` takes the raw stored record and checks items such as:

- CRC.
- Brightness range.
- Whether `autoMode` is `0` or `1`.
- Whether `deviceName` has a valid null terminator.
- Whether `threshold` is valid.

If everything is valid, it copies the values into:

```cpp
settings
```

The process is:

```text
NVS
 ↓
record
 ↓ decodeRecord()
 ├── CRC OK?
 ├── Values OK?
 ├── Strings OK?
 └── Format OK?
 ↓
settings
```

#### Step 8: Mark the Settings as Clean

```cpp
dirty = false;
```

The settings have just been loaded from flash, so there are no unsaved changes.

Then:

```cpp
return LoadStatus::Loaded;
```

indicates success.

### 2. `saveSettings()`

```cpp
bool saveSettings()
```

This function performs the opposite operation.

It takes the current RAM settings, validates them, creates a 52-byte storage record, and writes it to NVS.

#### Step 1: Check Whether Saving Is Allowed

```cpp
if (!nvsReady || saveBlocked) {
  return false;
}
```

There are two reasons not to save:

```text
NVS is not ready
        OR
Saving has been blocked
```

If either condition is true:

```cpp
return false;
```

No flash write occurs.

#### Step 2: Validate the RAM Settings

```cpp
if (!validateSettings(settings)) {
  return false;
}
```

Before writing anything to flash, the function verifies that the current settings are valid.

For example:

```text
brightness = 0–100
autoMode   = 0 or 1
name       = valid
threshold  = valid
```

If the settings are invalid:

```cpp
return false;
```

This prevents bad data from being permanently stored.

#### Step 3: Check the `dirty` Flag

```cpp
if (!dirty) {
  return true;
}
```

If nothing has changed since the last save, there is no reason to write to flash.

```text
No changes
    ↓
No NVS write
```

This is useful because unnecessary writes consume flash and NVS write endurance.

The function returns `true` because there was no error. There simply was nothing to save.

#### Step 4: Create the Storage Record

```cpp
const StoredSettings record = makeRecord(settings);
```

This converts your RAM representation:

```text
DeviceSettings
```

into your NVS representation:

```text
StoredSettings
```

`makeRecord()` also calculates the CRC.

Conceptually:

```text
settings
   │
   ▼
makeRecord()
   │
   ├── version
   ├── brightness
   ├── autoMode
   ├── deviceName
   ├── threshold
   └── CRC
   │
   ▼
52-byte record
```

#### Step 5: Write the Record to NVS

```cpp
if (preferences.putBytes(
      SETTINGS_KEY,
      &record,
      sizeof(record)
    ) != sizeof(record)) {
  return false;
}
```

This writes the complete `StoredSettings` structure as a binary BLOB.

Expected result:

```text
52 bytes written
```

If fewer than 52 bytes are reported:

```text
Save failed
```

The `dirty` flag remains `true`.

This is important because the code deliberately does not execute:

```cpp
dirty = false;
```

when the write fails.

Therefore, the RAM changes remain marked as unsaved.

#### Step 6: Mark the Save as Successful

```cpp
dirty = false;
```

At this point, the RAM and NVS copies are synchronized.

```text
RAM settings
     │
     │ save
     ▼
NVS settings

Both are now identical
        ↓
   dirty = false
```

Then:

```cpp
return true;
```

indicates that the save succeeded.

### The Two Functions Together

The complete system can be visualized like this:

```text
                 ESP32
              ┌──────────┐
              │   RAM    │
              │ settings │
              └────┬─────┘
                   │
          ┌────────┴────────┐
          │                 │
        SAVE              LOAD
          │                 ▲
          ▼                 │
   makeRecord()             │
          │                 │
          ▼                 │
     52-byte record         │
          │                 │
          ▼                 │
      ┌─────────┐            │
      │   NVS   │────────────┘
      │  Flash  │
      └─────────┘
```

The design philosophy is:

```text
loadSettings():

“Do not trust what is in flash until I check it.”
```

```text
saveSettings():

“Do not write to flash unless the data is valid and has actually changed.”
```

This combination makes your NVS settings implementation much more robust than simply using `putBytes()` and `getBytes()` without validation.


---

# 7. Demonstrating persistence

Send these commands one at a time:

```text
BRIGHTNESS 80
AUTO 0
NAME Workshop S3
THRESHOLD 30.5
SAVE
REBOOT
```

After reboot, reconnect Serial Monitor if necessary and send:

```text
SHOW
```

The values should still be:

```text
brightness=80
autoMode=OFF
deviceName="Workshop S3"
threshold=30.50
```

Now disconnect power completely and reconnect it. Send `SHOW` again to verify retention after a power cycle.

## Demonstrating an unsaved change

Send:

```text
BRIGHTNESS 20
REBOOT
```

Brightness returns to `80`, because `20` was changed only in RAM.

## Example Serial Monitor output

The `>` lines represent commands you enter; the sketch does not print those prompts.

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

Also try invalid input:

```text
BRIGHTNESS 150
BRIGHTNESS abc
THRESHOLD nan
```

The sketch rejects these without changing the active settings.

---

# 8. ESP-IDF equivalent using `nvs_flash`

The native API follows the same storage pattern:

| Arduino Preferences         | ESP-IDF                                               |
|----------------------------|-------------------------------------------------------|
| Arduino core initializes default NVS | `nvs_flash_init()`                          |
| `begin("my-app", false)`   | `nvs_open("my-app", NVS_READWRITE, &handle)`          |
| `getBytesLength()` / `getBytes()` | `nvs_get_blob()`                              |
| `putBytes()`               | `nvs_set_blob()` followed by `nvs_commit()`           |
| `clear()`                  | `nvs_erase_all(handle)` followed by `nvs_commit()`    |
| `end()`                    | `nvs_close(handle)`                                   |

In ESP-IDF, explicitly call `nvs_commit()` after successful writes or deletions. Closing the handle does not substitute for committing. [nvs.h](https://github.com/espressif/esp-idf/blob/master/components/nvs_flash/include/nvs.h)

Use a C++ source file, `main.cpp`. Reuse these definitions and helpers from the Arduino sketch above:

- `DeviceSettings` and `DEFAULT_SETTINGS`
- `StoredSettings`, `SETTINGS_VERSION`, and static assertions
- `validateSettings()`, `crc32()`, `makeRecord()`, and `decodeRecord()`

Place them above the following code, omitting the Arduino/Preferences includes and globals. The downloadable `esp_idf_main.cpp` already contains the combined standalone source.

```cpp
#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "settings";

esp_err_t loadSettings(nvs_handle_t handle,
                       DeviceSettings &out) {
  size_t length = 0;

  // First query the stored size.
  esp_err_t err =
    nvs_get_blob(handle, "settings", nullptr, &length);

  if (err != ESP_OK) return err;

  if (length != sizeof(StoredSettings)) {
    return ESP_ERR_INVALID_SIZE;
  }

  StoredSettings record{};

  err = nvs_get_blob(handle, "settings", &record, &length);
  if (err != ESP_OK) return err;

  if (record.version != SETTINGS_VERSION) {
    return ESP_ERR_NOT_SUPPORTED;
  }

  return decodeRecord(record, out)
           ? ESP_OK
           : ESP_ERR_INVALID_CRC;
}

esp_err_t saveSettings(nvs_handle_t handle,
                       const DeviceSettings &settings) {
  if (!validateSettings(settings)) {
    return ESP_ERR_INVALID_ARG;
  }

  const StoredSettings record = makeRecord(settings);

  esp_err_t err =
    nvs_set_blob(handle, "settings", &record, sizeof(record));

  if (err != ESP_OK) return err;

  return nvs_commit(handle);
}

esp_err_t factoryReset(nvs_handle_t handle,
                       DeviceSettings &settings) {
  // Clears only the namespace associated with this handle.
  esp_err_t err = nvs_erase_all(handle);
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

    // Diagnose NO_FREE_PAGES / NEW_VERSION_FOUND deliberately.
    // This example does not automatically erase the partition.
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

  if (err == ESP_ERR_NVS_NOT_FOUND ||
      err == ESP_ERR_INVALID_CRC) {
    ESP_LOGI(TAG, "Missing/invalid record; saving defaults");
    err = saveSettings(handle, settings);
  }

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Load/save failed: %s; keeping RAM defaults",
             esp_err_to_name(err));

    nvs_close(handle);
    return;
  }

  ESP_LOGI(TAG, "brightness=%d auto=%d name=%s threshold=%.2f",
           settings.brightness,
           settings.autoMode,
           settings.deviceName,
           static_cast<double>(settings.threshold));

  // Call when a user requests a save:
  // settings.brightness = 80;
  // err = saveSettings(handle, settings);
  // Check err before reporting success or restarting.

  // Call for an explicit factory-reset action:
  // err = factoryReset(handle, settings);

  nvs_close(handle);
}
```

For the main component:

```cmake
idf_component_register(
  SRCS "main.cpp"
  INCLUDE_DIRS "."
  REQUIRES nvs_flash
)
```

Build and run:

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash monitor
```

This native example loads or initializes settings. Connect its save and reset functions to your application's UI events.

---

# 9. Best practices

## Namespace naming and handle lifetime

Use short, descriptive names such as:

```cpp
preferences.begin("my-app", false);
```

Namespace and key names have a maximum length of 15 characters. A namespace groups keys within a partition; it is not a separate partition.

Use read-only access when appropriate:

```cpp
preferences.begin("my-app", true);
```

Call `end()` when finished with a handle. It closes access without erasing saved values. [Preferences API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/preferences.html)

The complete sketch keeps its namespace open because commands may arrive throughout operation. It closes the handle before its controlled reboot. Opening and closing around occasional operations is also valid.

## `putBytes`/`getBytes` versus individual keys

| Approach            | Advantages                                      | Tradeoffs                                         |
|---------------------|-------------------------------------------------|---------------------------------------------------|
| One versioned blob  | Complete settings snapshot; one record checksum | Whole record written together; layout changes require migration |
| Individual keys     | Clear names; update one field independently     | Separate writes can leave a mixture of old/new fields if interrupted |

Individual-key storage could look like:

```cpp
preferences.putInt("brightness", settings.brightness);
preferences.putBool("auto", settings.autoMode);
preferences.putString("name", settings.deviceName);
preferences.putFloat("threshold", settings.threshold);

// Example read with fallback:
settings.brightness = preferences.getInt("brightness", 50);
```

Check each write's result in production. A getter returning a fallback does not prove that a valid saved value exists.

`getBytes()` returns bytes without preserving their C++ type. Your application must manage length and interpretation. Never persist pointers or an Arduino `String` by copying the object's memory; use a bounded character array or string storage. [Preferences tutorial](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html)

## Flash wear

- Save after explicit user action or after a quiet period following edits.
- Coalesce rapid slider changes.
- Avoid saving frequent counters or telemetry in the settings record.
- Wait for save success before reporting persistence or rebooting.
- Keep power stable during flash operations.

The sketch uses a `dirty` flag to skip saves when no changes are pending.

## Handling NVS full or unavailable

Check initialization and write results. On failure, retain the active RAM settings and tell the user they remain unsaved.

Enable **Core Debug Level: Error** or a more verbose level to inspect Preferences diagnostics. For detailed recovery, use native error codes:

| Error                        | Meaning                                                  |
|-----------------------------|----------------------------------------------------------|
| `ESP_ERR_NVS_NOT_ENOUGH_SPACE` | Write cannot allocate sufficient storage              |
| `ESP_ERR_NVS_NO_FREE_PAGES`    | Initialization cannot find usable free pages          |
| `ESP_ERR_NVS_NEW_VERSION_FOUND`| Partition contains an unsupported NVS format          |

Diagnose the partition and stored data before deciding to erase. `freeEntries()` counts entries rather than guaranteeing writable blob bytes. [nvs.h](https://github.com/espressif/esp-idf/blob/master/components/nvs_flash/include/nvs.h)

Possible remedies include removing obsolete application keys or allocating a larger/dedicated NVS partition.

`nvs_flash_erase()` is much broader than `preferences.clear()`: it erases the default NVS partition and its other namespaces, potentially including Wi-Fi configuration. Use it only as part of a deliberate recovery policy. [nvs_flash.h](https://github.com/espressif/esp-idf/blob/master/components/nvs_flash/include/nvs_flash.h)

## Interrupted operations

- Do not assume an interrupted save succeeded.
- Factory reset here consists of two operations: clear, then save. If power fails between them, the next boot finds the missing settings key and recreates defaults.
- A checksum detects an invalid record; it does not guarantee that the newest change survives power loss.

## Versioning for future OTA updates

Treat `SETTINGS_VERSION` as a storage-schema version, separate from firmware release numbers.

When changing the stored format:

- Keep a definition for the old record.
- Recognize and validate its version and size.
- Initialize the new structure from defaults.
- Copy compatible old fields.
- Save the new format after successful conversion.
- Consider how firmware rollback will handle it.

Ordinary application OTA updates generally preserve NVS when partition layout remains unchanged; the update targets application partitions. [ESP-IDF OTA](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/ota.html)

Changing `DEFAULT_SETTINGS` in firmware does not change already saved settings.

This sketch detects unsupported versions and preserves them. It does not implement migration. `FACTORY_RESET` explicitly discards the existing record.

---

# 10. Troubleshooting

| Symptom                              | Likely cause and action                                                                 |
|--------------------------------------|------------------------------------------------------------------------------------------|
| `Preferences.h` not found            | Install/select the Espressif ESP32 board package and ESP32-S3 target.                   |
| Settings disappear after reboot      | Send `SAVE`, verify success, and keep namespace/key names unchanged.                    |
| Defaults appear after firmware upload| Check full-flash erase options, partition changes, and schema changes.                  |
| `begin()` fails                      | Check the NVS partition and diagnostic logs.                                            |
| Write returns zero                   | Check read/write mode, key length/type, available space, and storage errors.            |
| Incompatible-record message          | Migrate the stored format or deliberately run `FACTORY_RESET`.                          |
| Repeated CRC/validation failures     | Check supply stability, stored-format assumptions, and flash health.                    |
| No Serial output                     | Match USB connector, CDC setting, selected port, and UART baud rate.                    |
| Commands do nothing                  | Select **Newline** or **Both NL & CR**; commands are case-sensitive.                    |
| Holding BOOT during power-on does nothing | Release BOOT, press EN, then hold BOOT after normal startup.                        |
| Settings change but hardware does not| Add display/PWM/sensor logic that consumes the RAM settings.                            |

**Verification:** The Arduino source passed host-side syntax and logic checks for validation, simulated persistence, failed saves, version preservation, and button timing. It has not been compiled with the ESP32 toolchain or tested on hardware here; use the reboot and complete power-cycle procedure above to verify your board.

# esp_idf_main.cpp
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
