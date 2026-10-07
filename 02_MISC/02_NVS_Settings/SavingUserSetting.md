# Saving User Settings in Non-Volatile Memory on ESP32-S3

**Hardware:** ESP32-S3 DevKitC-1  
**Framework:** Arduino for ESP32, with an ESP-IDF equivalent  
**Goal:** Preserve user-configurable settings across reboot and power loss.  
**Downloadable files:** `SavingUserSettings.ino`

---

## 1. Introduction: Why settings need NVS

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

## 2. Library choice

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

## 3. Settings structure and defaults

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

### RAM structure versus stored record

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

### Redefine padding as reserved byte
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

## 4. Core logic

### On boot

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

### Loading

`loadSettings()` checks:

- Key existence.
- Stored type and length.
- Storage-format version.
- CRC.
- Field validity.

It updates the RAM settings only after validation succeeds.

### Saving on demand

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

## 5. Arduino setup and factory-reset button

### Arduino IDE setup

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

### Method 1: BOOT button held for five seconds

DevKitC-1 already has a BOOT button connected to **GPIO 0**, active LOW. No additional wiring is needed.

For an external normally open button, connect:

| Button terminal | Connection |
|-----------------|------------|
| One side        | GPIO 0     |
| Other side      | GND        |

The sketch uses `INPUT_PULLUP`.

Let the application start normally, then hold BOOT for five seconds. GPIO 0 is a strapping pin; holding BOOT while resetting or powering up can select ROM download mode, where your sketch does not run. [ESP32-S3 DevKitC-1 User Guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html)

The implementation includes debounce, nonblocking timing, and one reset action per continuous press.

### Method 2: Serial command

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

## 6. Complete Arduino sketch

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

### 6.1 `static_assert()` in C++

`static_assert()` is a compile-time check in C++. It lets you tell the compiler:

> “This condition must always be true. If it isn't, stop compilation and show an error.”

It is not a function that runs on the ESP32. Nothing is executed at runtime.

#### 1. Check the Size of `float`

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

#### 2. Check Where `crc` Is Located

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

#### 3. Check the Total Structure Size

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

#### Why These Checks Are Useful for NVS

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

#### `static_assert` vs. Normal `assert`

This distinction is important.

##### `static_assert`

```cpp
static_assert(sizeof(StoredSettings) == 52,
              "Storage layout changed");
```

This check is performed during compilation.

##### Normal `assert`

```cpp
assert(sizeof(StoredSettings) == 52);
```

This is a runtime assertion. It is not what you want for validating a binary storage format.

Because `sizeof()` and `offsetof()` are known at compile time, `static_assert()` is the ideal choice.





---

## 7. Demonstrating persistence

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

### Demonstrating an unsaved change

Send:

```text
BRIGHTNESS 20
REBOOT
```

Brightness returns to `80`, because `20` was changed only in RAM.

### Example Serial Monitor output

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

## 8. ESP-IDF equivalent using `nvs_flash`

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

## 9. Best practices

### Namespace naming and handle lifetime

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

### `putBytes`/`getBytes` versus individual keys

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

### Flash wear

- Save after explicit user action or after a quiet period following edits.
- Coalesce rapid slider changes.
- Avoid saving frequent counters or telemetry in the settings record.
- Wait for save success before reporting persistence or rebooting.
- Keep power stable during flash operations.

The sketch uses a `dirty` flag to skip saves when no changes are pending.

### Handling NVS full or unavailable

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

### Interrupted operations

- Do not assume an interrupted save succeeded.
- Factory reset here consists of two operations: clear, then save. If power fails between them, the next boot finds the missing settings key and recreates defaults.
- A checksum detects an invalid record; it does not guarantee that the newest change survives power loss.

### Versioning for future OTA updates

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

## 10. Troubleshooting

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
