# Organizing an ESP32-S3 Arduino Project

For ESP32-S3 projects, use:

- One short `.ino` file to coordinate the application.
- Separate `.h` and `.cpp` files for each feature.
- Arduino libraries for code reused across projects.

Arduino supports this directly: a sketch is a folder, not only an `.ino` file. `.cpp` files compile separately, while additional `.ino` tabs are concatenated in a defined order and receive Arduino's automatic function prototypes.

Tabs are useful for quickly splitting a sketch, but `.h`/`.cpp` pairs are a better foundation for reusable code.

See the [Arduino CLI sketch build process](https://docs.arduino.cc/arduino-cli/sketch-build-process).

## Suggested Project Structure

```text
Esp32TelegramDisplay/
├── Esp32TelegramDisplay.ino    // setup(), loop(), and feature coordination
├── BoardConfig.h               // GPIO assignments, SPI speeds, feature settings
├── SdStore.h
├── SdStore.cpp                 // SD initialization and file operations
├── TelegramService.h
├── TelegramService.cpp         // Bot polling, messages, document handling
├── DisplayService.h
├── DisplayService.cpp          // TFT drawing and touch input
├── DownloadService.h
├── DownloadService.cpp         // HTTPS transfer into an open file
├── secrets.h                   // Wi-Fi password and bot token; keep private
└── secrets.example.h           // Required setting names; no real values
```

The division should follow responsibility rather than creating one file per command or one class per function.

| Component | Owns | Should Call |
|---|---|---|
| `SdStore` | SD initialization, readiness, and opening or closing files. | SD and SPI APIs. |
| `TelegramService` | Bot polling, command parsing, and sending replies. | Download and display services when needed. |
| `DownloadService` | HTTPS requests, progress, byte counts, and transfer errors. | `SdStore` to obtain the destination file. |
| `DisplayService` | Screen drawing, touch events, and screen state. | TFT and touch libraries. |
| Main `.ino` | Initialization order and application flow. | The services above. |

For example, receiving a Telegram document is an application workflow:

1. Telegram identifies the file.
2. The download component transfers its bytes.
3. SD storage saves them.
4. Telegram reports the result.

Keeping these stages separate makes transfer failures easier to locate.

## Header and Implementation Files

A header declares what other files may use:

### `SdStore.h`

```cpp
#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <FS.h>

class SdStore {
public:
  SdStore(
    SPIClass& spi,
    uint8_t csPin,
    uint32_t frequency
  );

  bool begin();

  bool ready() const;

  fs::File openForWrite(
    const char* path
  );

private:
  SPIClass& spi_;
  uint8_t csPin_;
  uint32_t frequency_;
  bool ready_ = false;
};
```

The `.cpp` file contains the implementation:

### `SdStore.cpp`

```cpp
#include "SdStore.h"
#include <SD.h>

SdStore::SdStore(
  SPIClass& spi,
  uint8_t csPin,
  uint32_t frequency
)
  : spi_(spi),
    csPin_(csPin),
    frequency_(frequency) {
}

bool SdStore::begin() {
  // Initialize the SPI bus with the board's
  // pins before calling this.
  ready_ =
    SD.begin(
      csPin_,
      spi_,
      frequency_
    );

  return ready_;
}

bool SdStore::ready() const {
  return ready_;
}

fs::File SdStore::openForWrite(
  const char* path
) {
  if (!ready_) {
    return fs::File{};
  }

  return SD.open(
    path,
    FILE_WRITE
  );
}
```

Your existing `sdSPI` object can be passed into `SdStore`. The storage code does not need to know which ESP32-S3 board or GPIO assignments the project uses.

The current ESP32 Arduino SD API accepts a `SPIClass` reference and a frequency in `SD.begin()`.

See the [Arduino-ESP32 SD header](https://github.com/espressif/arduino-esp32/blob/master/libraries/SD/src/SD.h).

## Simplified Main Sketch

The main sketch then becomes readable at a glance:

```cpp
#include "BoardConfig.h"
#include "SdStore.h"
#include "TelegramService.h"
#include "DisplayService.h"

// Construct services using the existing
// SPI, bot, and display objects.
SdStore storage(
  sdSPI,
  SD_CS_PIN,
  SD_FREQUENCY
);

void setup() {
  Serial.begin(115200);

  sdSPI.begin(
    SD_SCK_PIN,
    SD_MISO_PIN,
    SD_MOSI_PIN,
    SD_CS_PIN
  );

  if (!storage.begin()) {
    Serial.println(
      "[SD] Initialization failed"
    );
  }

  // Initialize Wi-Fi, display, and Telegram
  // in the required order.
}

void loop() {
  // Poll Telegram, process touch events,
  // and update the display.
}
```

`BoardConfig.h` contains the actual board wiring.

Keep pin numbers and SPI setup there. Keep file handling in `SdStore`.

If the TFT, touch controller, and SD card share an SPI bus, also document:

- Which devices share the bus.
- The chip-select pin for each device.
- Which device is active during each transaction.

This is a hardware and initialization concern, not something to hide inside the Telegram handler.

## Convert a Module into a Library

First, move a feature into `.h` and `.cpp` files within the current sketch.

Once its interface works without references to:

- The project's screen.
- The bot token.
- Fixed GPIO values.
- Other project-specific globals.

turn it into a library:

```text
Documents/Arduino/libraries/Esp32SdStore/
├── library.properties
├── src/
│   ├── Esp32SdStore.h
│   └── Esp32SdStore.cpp
└── examples/
    └── BasicWrite/
        └── BasicWrite.ino
```

Other projects can then use:

```cpp
#include <Esp32SdStore.h>
```

Arduino's library format compiles source files under `src/`, while `examples/` provides small sketches that exercise the library independently.

See the [Arduino library specification](https://docs.arduino.cc/arduino-cli/library-specification).

Do not extract everything into one large `MyESP32Utils` library.

Separate reusable modules are easier to maintain:

- SD module.
- Telegram module.
- Display module.

Keep project-specific commands, such as downloading a Telegram document and showing progress on a particular TFT, inside the project.

## Practical Migration Order

1. Move `BoardConfig.h` and private credentials out of the main `.ino`.
2. Exclude `secrets.h` from Git.
3. Keep `secrets.example.h` as a configuration guide.
4. Extract SD initialization and file operations.
5. Compile and test the SD card independently.
6. Extract HTTPS downloading.
7. Have the downloader report bytes written and an error code.
8. Let the caller decide what to print or send to Telegram.
9. Extract Telegram polling and command parsing.
10. Pass storage and download services into Telegram handlers instead of using scattered global flags such as `sdReady`.
11. Extract display and touch handling.
12. Keep drawing code out of network callbacks.
13. Promote genuinely reusable modules to libraries.
14. Add a minimal example sketch for each library.

## Debugging Module Boundaries

Log activity at each boundary:

```text
[TG] file received
[HTTP] status/bytes
[SD] open/write/close
[UI] state changed
```

Return success or failure to the caller instead of hiding errors inside a module.

A library example that initialises the SD card and writes one file is particularly useful. It shows whether a later fault is in:

- The storage system itself.
- The Telegram download workflow.
- The display or UI layer.

You can also record core and library versions in an Arduino CLI sketch profile when reproducible builds are required.

See the [Arduino CLI sketch project file documentation](https://docs.arduino.cc/arduino-cli/sketch-project-file).

## Recommended Next Edit

Start with:

```text
SdStore.h
SdStore.cpp
```

Use the existing SD initialisation call:

```cpp
SD.begin(
  SD_CS_PIN,
  sdSPI,
  SD_FREQUENCY
);
```

This is a small extraction and establishes the pattern for the larger Telegram and TFT modules.

# Building a Multi-File Arduino Sketch

Open:

```text
Esp32TelegramDisplay.ino
```

Then click **Verify** or **Upload**.

Arduino treats the entire `Esp32TelegramDisplay` folder as one sketch, provided that:

- The folder is named `Esp32TelegramDisplay`.
- The primary `.ino` file is named `Esp32TelegramDisplay.ino`.

Arduino automatically compiles the `.cpp` files located in that sketch folder.

See the [Arduino sketch specification](https://docs.arduino.cc/arduino-cli/sketch-specification).

## Include Headers Explicitly

Headers are not included automatically.

The file that uses a declaration must include its corresponding header:

```cpp
// Esp32TelegramDisplay.ino

#include "BoardConfig.h"
#include "SdStore.h"
#include "TelegramService.h"
#include "DisplayService.h"
#include "DownloadService.h"
#include "secrets.h"
```

Likewise, `SdStore.cpp` should begin with:

```cpp
#include "SdStore.h"
```

`secrets.example.h` is only a template. Do not include it in the build.

Unlike `.ino` files, `.cpp` files do not automatically receive:

- Arduino's automatic `Arduino.h` include.
- Arduino's generated function prototypes.

Therefore, put required includes and declarations in the relevant header or `.cpp` files.

See the [Arduino sketch build process](https://docs.arduino.cc/arduino-cli/sketch-build-process).

## Rebuilding After Changes

Edits are picked up during the next **Verify** or **Upload** operation.

- Changing a `.cpp` file recompiles that source file.
- Changing a header recompiles files that include it.
- Arduino may reuse unaffected compiled files.

Save all edits before building.

## Folder Layout

For the following layout, keep all `.h` and `.cpp` files beside the primary `.ino` file:

```text
Esp32TelegramDisplay/
├── Esp32TelegramDisplay.ino
├── BoardConfig.h
├── SdStore.h
├── SdStore.cpp
├── TelegramService.h
├── TelegramService.cpp
├── DisplayService.h
├── DisplayService.cpp
├── DownloadService.h
├── DownloadService.cpp
├── secrets.h
└── secrets.example.h
```

Arduino compiles code in:

- The sketch root.
- The `src/` directory and its subdirectories.

An arbitrary subfolder such as:

```text
modules/SdStore.cpp
```

is not automatically compiled as part of the sketch.

See the [Arduino sketch specification](https://docs.arduino.cc/arduino-cli/sketch-specification).

# Splitting SD Functions into a Header (Alternate Method)

For the current sketch, moving the SD functions into `SdCardFeature.h` is a sensible first step.

Include it near the top of `Esp32TelegramDisplay.ino`:

```cpp
#include "SdCardFeature.h"
```

A header containing function definitions can work this way. Arduino does not include the header automatically; the `#include` directive brings its contents into the sketch.

See the [Arduino sketch build process](https://docs.arduino.cc/arduino-cli/sketch-build-process).

## Example

### `SdCardFeature.h`

```cpp
#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

inline bool initSdCard(
  SPIClass& spi,
  uint8_t csPin,
  uint32_t frequency
) {
  return SD.begin(
    csPin,
    spi,
    frequency
  );
}
```

### `Esp32TelegramDisplay.ino`

```cpp
#include "SdCardFeature.h"
```

In `setup()`, after configuring `sdSPI`:

```cpp
sdReady =
  initSdCard(
    sdSPI,
    SD_CS_PIN,
    SD_FREQUENCY
  );
```

## Moving Longer SD Functions

You can also move existing longer SD functions into the header with minimal changes.

If the header is included only by the main sketch, ordinary function definitions will work.

The `inline` keyword is used above so the function remains safe to include from multiple `.cpp` files later.

## Practical Rules

### Use `#pragma once`

Put this at the top of the header:

```cpp
#pragma once
```

This prevents repeated inclusion within one source file.

### Define Shared Globals in One Place

Define shared globals such as:

```cpp
SPIClass sdSPI;
bool sdReady;
```

in one source file only.

Prefer passing them into functions:

```cpp
sdReady =
  initSdCard(
    sdSPI,
    SD_CS_PIN,
    SD_FREQUENCY
  );
```

instead of defining a second copy inside the header.

## Recommended Migration Path

Start by splitting the large `.ino` file into feature headers to improve navigation.

When a feature:

- Is shared across several projects.
- Has significant dependencies.
- Requires independent testing.
- Contains substantial implementation code.

move its implementation into a `.cpp` file and keep only declarations in the `.h` file.
