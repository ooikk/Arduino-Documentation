# Arduino IDE 2.x Persistent Build Cache

This is a textbook example of the Arduino IDE 2.x persistent build-cache problem.

## Root Cause

The problem is caused by a stale build cache on the computer, not by the ESP32.

Arduino IDE 2.x caches compiled object files between sessions and associates them with the sketch name and path. Arduino's support documentation confirms that cached files are preserved between IDE sessions:

> “If you're using IDE 2, the cached files are preserved between sessions.”

See the [Arduino support documentation](https://support.arduino.cc/hc/en-us/articles/7213793867036-Improve-compilation-time-with-Arduino-IDE).

## What Happened

### Timeline

```text
02_WiFi_RainMaker_QWEN
    Original buggy sketch
    Compiled first
    Broken object files were cached

02_WiFi_RainMaker_QWEN_1
    Fixed sketch created using Save As
    Different sketch name
    New cache key
    Clean compilation
    Fixed binary works

02_WiFi_RainMaker_QWEN
    Renamed back to the original name
    Original cache key was reused
    Stale object files were reused
    Boot loop returned
```

### Step-by-Step Explanation

1. The original sketch was compiled:

   ```text
   02_WiFi_RainMaker_QWEN
   ```

2. Arduino IDE cached the compiled object files using a key derived from the sketch name and path.

3. The original binary contained a bug that caused boot loops.

4. The sketch was saved under a new name:

   ```text
   02_WiFi_RainMaker_QWEN_1
   ```

5. Because the name was different, Arduino IDE created a new cache key and performed a clean compilation.

6. The fixed binary worked correctly.

7. The sketch was renamed back to:

   ```text
   02_WiFi_RainMaker_QWEN
   ```

8. Arduino IDE found the old cache for that name and reused the stale object files.

9. The resulting binary was effectively the original broken build, so the boot loop returned.

An Arduino forum moderator described the same behavior:

> “The only way you could avoid it would be to also rename the sketch, or otherwise change its path, when you renamed the files, which would cause a new build folder to be used.”

See the [Arduino forum discussion](https://forum.arduino.cc/t/how-to-clear-cache-from-ide/1374114/9).

## Why Erase-All-Flash Does Not Fix It

The Arduino IDE option:

```text
Erase All Flash Before Sketch Upload
```

runs a flash-erase operation on the ESP32, typically through `esptool.py`.

It erases data on the ESP32, including:

- NVS data.
- Wi-Fi credentials.
- OTA metadata.
- RainMaker credentials.
- Files stored in flash.

However, it does not remove the build cache from the computer.

The problem is that the compiled binary itself is incorrect because it was created from stale cached object files.

Therefore:

```text
Erase ESP32 flash
    +
Upload stale or broken binary
    =
Boot loop remains
```

## Arduino IDE Cache Bug History

There was a reported Arduino IDE 2.3.9 issue where cached objects from `.ino` files were reused even after the source code had been modified.

This was fixed in Arduino IDE 2.3.10.

See the [Arduino forum announcement](https://forum.arduino.cc/t/arduino-ide-2-3-10-is-now-available/1447778).

Even without that specific bug, Arduino IDE 2.x is designed to preserve compiled objects between sessions to reduce compilation time.

## Why It Can Affect RainMaker and ESP32-S3 Projects

The sketch name becomes the build project name in the Arduino ESP32 build system.

Conceptually:

```text
Sketch name
    ↓
build.project_name
    ↓
ESP-IDF PROJECT_NAME
```

The project name is included in the firmware application descriptor, represented by:

```cpp
esp_app_desc_t
```

The application descriptor contains information such as:

- Project name.
- Firmware version.
- Compile date.
- Compile time.
- SHA-256 application hash.

If stale object files are mixed with newly compiled files, the resulting binary can contain inconsistent code or metadata.

This may cause unexpected boot behavior.

The ESP RainMaker framework also depends on:

- NVS initialization.
- Node-ID initialization.
- Cloud credentials.
- Secure storage.
- MQTT and TLS setup.
- Background FreeRTOS tasks.

Known RainMaker boot-loop scenarios include failures such as:

```text
Failed to initialise Node Id
```

or NVS-related initialization errors.

A stale or inconsistent build can trigger these failure paths.

## How to Fix the Problem

### Method 1: Clear the Build Cache

This is the recommended method.

1. Close Arduino IDE.
2. Delete the Arduino build-cache folders.

#### Windows

Delete folders matching:

```text
C:\Users\<yourUsername>\AppData\Local\Temp\arduino-sketch-*
```

Depending on the Arduino IDE version, also check:

```text
C:\Users\<yourUsername>\AppData\Local\arduino\
```

#### Linux

Check and remove:

```text
/tmp/arduino-sketch-*
```

or:

```text
~/.cache/arduino/
```

#### macOS

Check and remove:

```text
~/Library/Caches/arduino/
```

and:

```text
/tmp/arduino-sketch-*
```

3. Reopen Arduino IDE.
4. Open:

   ```text
   02_WiFi_RainMaker_QWEN
   ```

5. Compile the sketch again.

Arduino IDE should now perform a fresh compilation using the current source files.

### Find the Exact Cache Path

To find the exact cache location:

1. Open Arduino IDE.
2. Go to:

   ```text
   File → Preferences
   ```

3. Enable:

   ```text
   Show verbose output during compilation
   ```

4. Compile the sketch.
5. Inspect the output panel.
6. Look for lines such as:

   ```text
   Using previously compiled file:
   ```

These messages can reveal which object files are being reused.

See the [Arduino forum discussion about clearing the cache](https://forum.arduino.cc/t/cache-clear-option/1040106).

## Method 2: Change a Board Setting

Changing a board option can force Arduino IDE to use a different build configuration.

For example:

1. Change:

   ```text
   Tools → Flash Mode
   ```

   from `QIO` to `DIO`.

2. Compile the sketch.
3. Change the setting back.
4. Compile again.

Alternatively, temporarily change:

```text
Upload Speed
Flash Frequency
Partition Scheme
PSRAM setting
```

Then restore the original setting and perform another compilation.

> This is a workaround. Manually clearing the cache is more reliable.

## Method 3: Use Arduino CLI

If you use `arduino-cli`, compile with the `--clean` option:

```bash
arduino-cli compile \
  --clean \
  -b esp32:esp32:esp32s3 \
  02_WiFi_RainMaker_QWEN
```

The `--clean` option forces a complete recompilation without reusing cached build artifacts.

See the [Arduino CLI compile documentation](https://arduino.github.io/arduino-cli/dev/commands/arduino-cli_compile/).

## Method 4: Use a Different Sketch Name

Saving the sketch under a new name creates a new cache key:

```text
02_WiFi_RainMaker_QWEN_1
```

This is why the `Save As` method worked.

However, it is only a workaround. The stale cache for the original name remains and may cause the problem again if the sketch is renamed back.

## Summary Table

| Action | Cache Behavior | Result |
|---|---|---|
| `02_WiFi_RainMaker_QWEN` with original buggy code | Broken object files are cached | Boot loop |
| `02_WiFi_RainMaker_QWEN_1` with fixed code | New name creates a new cache key and clean build | Works |
| Rename fixed code back to `02_WiFi_RainMaker_QWEN` | Original stale cache is reused | Boot loop returns |
| Erase all ESP32 flash and upload again | Erases ESP32 flash but not the PC cache | Still broken |
| Delete the Arduino build cache | Forces a fresh compilation | Fixes stale-build problem |
| Compile with `arduino-cli --clean` | Disables reuse of cached build artifacts | Fresh build |

## Final Recommendation

Before recompiling the sketch under its original name:

```text
02_WiFi_RainMaker_QWEN
```

do the following:

1. Close Arduino IDE.
2. Delete the Arduino build-cache folders.
3. Reopen Arduino IDE.
4. Enable **Erase All Flash Before Sketch Upload** if the partition scheme or RainMaker credentials have also changed.
5. Compile and upload the sketch again.
6. Provision the ESP32-S3 again if the flash was erased.

Remember:

```text
Clearing the PC build cache
    fixes stale compiled code.

Erasing ESP32 flash
    clears device-side data.

They are different operations and may both be required.
```

---

# ESP32-S3 Partition Scheme

In Arduino IDE, a **Partition Scheme**, also called a **Partition Table**, determines how the physical flash memory on the ESP32-S3 is divided into logical sections.

Think of the ESP32-S3 flash memory as a computer hard drive. Just as a hard drive can be divided into a `C:` drive for Windows and a `D:` drive for files, the partition scheme tells the ESP32 how much space to allocate for:

- The bootloader.
- Application code.
- Wi-Fi credentials.
- File storage.
- Over-the-Air (OTA) updates.
- Debugging information.

## 1. Why Is the Partition Scheme Important?

ESP32-S3 boards commonly include 4 MB, 8 MB, or 16 MB of flash memory. However, the Arduino compiler does not automatically know how that memory should be used.

If the wrong partition scheme is selected, several problems may occur.

### `Sketch too big` Error

Your application, such as one using ESP RainMaker, may be larger than the space allocated to the application partition.

### Boot Loops or Crashes

The ESP32 may try to read Wi-Fi credentials, non-volatile storage, or file-system data from a memory address that is not valid for the selected partition scheme.

### OTA Failures

An OTA update generally requires a second application partition. The new firmware is downloaded to the unused application partition while the current firmware continues running.

If no second application partition exists, OTA updates cannot work in the normal way.

## 2. Anatomy of an ESP32 Partition Table

A standard partition table may include the following sections.

### Bootloader

A small protected section that initializes the chip and determines which application should run.

### NVS

**NVS**, or Non-Volatile Storage, stores data that must survive a reboot, such as:

- Wi-Fi SSIDs and passwords.
- ESP RainMaker credentials.
- Device configuration.
- Calibration data.
- Other persistent settings.

### Application

The application partition stores the compiled Arduino C++ firmware.

When OTA is enabled, the application space is normally divided into two partitions:

```text
App0
App1
```

One partition runs the current firmware while the other receives the new firmware.

### SPIFFS or LittleFS

A file system used to store:

- HTML and CSS files.
- JavaScript files.
- JSON configuration files.
- Sensor logs.
- Other user data.

### Core Dump and PHY Initialization

Reserved areas may be used for:

- Crash debugging.
- Core dumps.
- Wi-Fi and Bluetooth radio initialization data.

## 3. Common Arduino IDE Partition Schemes

Open:

```text
Tools → Partition Scheme
```

You may see options similar to the following:

| Scheme Name | Approximate App Size | OTA | File Storage | Best Used For |
|---|---:|---:|---:|---|
| Default 4 MB with SPIFFS | 1.2 MB | Yes | 1.2 MB | Basic IoT projects and simple sensors |
| Minimal SPIFFS | 1.8 MB | Yes | 190 KB | Larger applications that need little file storage |
| No OTA (2 MB APP) | 2.0 MB | No | 1 MB | Medium-sized applications that do not require wireless updates |
| Huge APP (3 MB No OTA) | 3.0 MB | No | 1 MB | Large libraries such as ESP RainMaker, AWS IoT, and audio libraries |
| 8 MB Flash (3 MB APP, 1.5 MB FAT) | 3.0 MB | No | 1.5 MB | ESP32-S3 boards with 8 MB of physical flash |
| 16 MB Flash (3 MB APP, 9 MB FAT) | 3.0 MB | No | 9.0 MB | ESP32-S3 boards with 16 MB of flash and large data-logging requirements |

The exact names and sizes may vary according to the ESP32 Arduino core version and board package.

## 4. Which Scheme Should You Use for ESP RainMaker?

ESP RainMaker is a relatively large framework. It may include:

- TLS and SSL encryption.
- MQTT clients.
- Wi-Fi provisioning.
- BLE or SoftAP provisioning.
- JSON processing.
- AWS IoT Core components.
- Cloud certificates.
- Device-management services.

A compiled ESP RainMaker sketch can consume a significant amount of flash space.

### ESP32-S3 with 4 MB Flash

For a board with 4 MB of flash, select:

```text
Huge APP (3 MB No OTA / 1 MB SPIFFS)
```

If the default scheme is used, the compiler may report:

```text
Sketch too big
```

The firmware may also fail to boot if the application or required data cannot fit into the selected partitions.

### ESP32-S3 with 8 MB or 16 MB Flash

For an ESP32-S3 board with 8 MB or 16 MB of flash, select the corresponding scheme if it is available in your Arduino core version:

```text
8 MB Flash
```

or:

```text
16 MB Flash
```

These schemes provide approximately 3 MB for the application, along with additional space for:

- Logs.
- Configuration files.
- Local web pages.
- Sensor data.
- File-system storage.

> **Note:** No-OTA schemes, such as `Huge APP`, provide more application space but sacrifice the normal dual-partition OTA layout. You will usually need to connect the ESP32-S3 to a computer through USB to upload new firmware.

## 5. OTA Considerations

A no-OTA partition scheme typically contains only one large application partition:

```text
Large App
```

An OTA-capable layout normally requires two application partitions:

```text
App0
App1
```

With two application partitions:

1. The ESP32 runs firmware from `App0`.
2. New firmware is downloaded to `App1`.
3. The device verifies the update.
4. The ESP32 reboots into `App1`.
5. The previous firmware remains available as a fallback.

Therefore:

- **No OTA:** More space for one application.
- **OTA enabled:** Less space per application, but wireless firmware updates are possible.

For production ESP RainMaker devices, use an OTA-capable partition scheme if the firmware fits. If the application is too large, you may need to reduce the firmware size, use a larger flash device, or choose a different OTA layout.

## 6. How to Change the Partition Scheme

1. Open the sketch in Arduino IDE.
2. Select the correct board:

   ```text
   Tools → Board → ESP32 Arduino → ESP32S3 Dev Module
   ```

3. Open:

   ```text
   Tools → Partition Scheme
   ```

4. Select:

   ```text
   Huge APP (3 MB No OTA / 1 MB SPIFFS)
   ```

   or select the appropriate 8 MB or 16 MB scheme if supported by the board.

5. Compile the sketch.
6. Upload the firmware.

## 7. Erase Flash When Changing Schemes

When changing partition schemes, it is often best to erase the existing flash contents.

In Arduino IDE, look for an option similar to:

```text
Tools → Erase All Flash Before Sketch Upload → Enabled
```

Then upload the sketch again.

This removes old:

- NVS data.
- Wi-Fi credentials.
- RainMaker provisioning data.
- File-system contents.
- Partition-dependent configuration.

> **Warning:** Erasing the flash removes stored Wi-Fi credentials, RainMaker provisioning information, and files. The device will need to be provisioned again.

## 8. Troubleshooting Partition Problems

### `Sketch too big`

The application does not fit in the selected application partition.

Try:

1. Open:

   ```text
   Tools → Partition Scheme
   ```

2. Select a scheme with a larger application partition.
3. For example, move from:

   ```text
   Default
   ```

   to:

   ```text
   Huge APP
   ```

4. Compile and upload again.

### Guru Meditation Error or Core Panic During Boot

This may occur when the partition scheme was changed but old flash data remains.

Try:

1. Enable:

   ```text
   Erase All Flash Before Sketch Upload
   ```

2. Upload the firmware again.
3. Provision the ESP32-S3 again.

### RainMaker Provisioning Fails

If RainMaker cannot save Wi-Fi credentials, the NVS partition may be missing, corrupted, or incompatible with the new partition layout.

Try:

1. Erase the entire flash.
2. Upload the firmware again.
3. Open the Serial Monitor.
4. Start provisioning from the ESP RainMaker application.

### Wi-Fi Credentials Are Lost After Reboot

Check that:

- The selected partition scheme includes an NVS partition.
- The flash was not being erased during every upload.
- The firmware is not explicitly clearing NVS at startup.
- The partition table matches the flash size of the board.

## 9. Practical Recommendations

| Project Requirement | Recommended Approach |
|---|---|
| Small sensor application | Default partition scheme |
| Large ESP32-S3 application without OTA | Huge APP |
| ESP RainMaker prototype with 4 MB flash | Huge APP, then provision again after erasing flash |
| ESP RainMaker product requiring OTA | OTA-capable scheme with sufficient application space |
| Large log files | 8 MB or 16 MB flash with a large file-system partition |
| Local web server plus RainMaker | Larger flash device and a scheme with sufficient app and file-system space |
| Frequent wireless firmware updates | OTA-capable partition scheme |

## 10. Summary

The partition scheme determines how the ESP32-S3 allocates flash memory for:

```text
Bootloader
NVS
Application
OTA application
SPIFFS or LittleFS
Core dump
PHY initialization
```

For ESP RainMaker:

- Use **Huge APP** when the firmware is too large for the default application partition.
- Use an **OTA-capable scheme** when wireless updates are required.
- Use an **8 MB or 16 MB scheme** when the board has additional flash and needs file storage.
- Erase the flash after changing partition schemes if boot or provisioning problems occur.
- Verify the selected flash size and partition scheme match the physical ESP32-S3 board.

---
# Partition Schemes Options and Custom Partitions

These questions come down to understanding that every entry in the Arduino IDE partition menu is a fixed, pre-written CSV file—a partition table—not a layout that automatically adapts to your chip.

## 1. Why Does `Huge APP` Stop at 3 MB?

The following entry was designed for boards with 4 MB of flash:

```text
Huge APP (3 MB No OTA / 1 MB SPIFFS)
```

It uses a layout similar to:

```text
Bootloader and partition table
NVS
PHY initialization
3 MB application partition
1 MB SPIFFS partition
```

The 3 MB size is not a hardware limit of the ESP32-S3. It is simply the size Espressif selected when designing that particular 4 MB partition layout.

The menu entries do not automatically scale to the available flash size. Each menu entry refers to a static CSV file stored in the Arduino ESP32 core, typically in:

```text
tools/partitions/*.csv
```

The ESP32-S3 flash cache and MMU can execute applications considerably larger than 3 MB.

The important rules are:

- Application partitions must start at a `0x10000`-aligned offset.
- The first approximately 64 KB is reserved for the bootloader, partition table, and early data.
- The total size of all partitions must fit within the flash size selected under:

  ```text
  Tools → Flash Size
  ```

- OTA requires two application slots, such as `ota_0` and `ota_1`.
- Because OTA needs two application partitions, OTA layouts generally provide less space for each individual application.

For example:

```text
No OTA:
  One large application partition

OTA:
  ota_0 + ota_1
  Two smaller application partitions
```

## Recommended Settings for an N16R8

For an ESP32-S3 N16R8, which generally has 16 MB of flash and 8 MB of PSRAM, use:

```text
Tools → Flash Size: 16 MB
Tools → PSRAM:      OPI PSRAM
Partition Scheme:   16M Flash (3 MB APP / 9.9 MB FATFS)
```

The predefined 16 MB scheme may still keep the application partition at approximately 3 MB because the remaining flash space is assigned to FATFS storage instead of the application.

For most projects, 3 MB is already sufficient. A complete RainMaker build commonly fits within approximately 1.4–2 MB, depending on the selected features and libraries.

## 2. What Is the `Custom` Partition Scheme?

The `Custom` option allows you to write your own partition CSV instead of using one of Espressif's predefined layouts.

This is the solution if you need an application partition larger than 3 MB.

### How Custom Partitions Work in Arduino IDE

1. Create a file named:

   ```text
   partitions.csv
   ```

2. Place it in the same folder as your `.ino` sketch.
3. Select:

   ```text
   Tools → Partition Scheme → Custom
   ```

4. The Arduino build system will use your custom CSV file instead of a preset partition table.

## 3. Partition CSV Format

The CSV format is based on the ESP-IDF partition-table format:

```csv
# Name,     Type, SubType, Offset,   Size,     Flags
nvs,        data, nvs,     0x9000,   0x6000,
otadata,    data, ota,     0xe000,   0x2000,
app0,       app,  ota_0,   0x10000,  6M,
spiffs,     data, spiffs,  0x610000, 0x9F0000,
```

The exact offsets and sizes must be calculated so that the partitions do not overlap and fit within the selected flash size.

## 4. CSV Column Meanings

| Column | Meaning |
|---|---|
| `Name` | Partition label. You can choose a suitable name. |
| `Type` | `app` for firmware or `data` for configuration and storage. |
| `SubType` | For applications: `factory`, `ota_0`, or `ota_1`. For data: `nvs`, `phy`, `spiffs`, or `fat`. |
| `Offset` | Starting address. A blank value allows automatic placement after the preceding partition. |
| `Size` | Partition size, specified in hexadecimal or using `K` and `M`, such as `0x6000`, `6M`, or `512K`. |
| `Flags` | Usually left empty. |

## 5. Important Partition Rules

### Keep NVS

The NVS partition stores important persistent information, including:

- Wi-Fi credentials.
- RainMaker data.
- Device configuration.
- Other non-volatile settings.

Without a valid NVS partition, Wi-Fi and RainMaker provisioning may not work correctly.

### Keep PHY Initialization Data

The PHY initialization partition stores radio calibration data used by Wi-Fi and Bluetooth.

A typical entry is:

```csv
phy_init, data, phy, 0xf000, 0x1000,
```

### Start the First Application at `0x10000`

Application partitions must use a 64 KB-aligned address.

A typical starting offset is:

```text
0x10000
```

### Keep the First Data Partition Near `0x9000`

A common partition layout begins with:

```text
nvs,      data, nvs,  0x9000,  0x6000
phy_init, data, phy,  0xf000, 0x1000
```

The space before `0x9000` is reserved for the bootloader and partition table.

### Stay Within the Flash Size

For a 16 MB flash device:

```text
16 MB = 0x1000000 bytes
```

The total size of all partitions must not exceed:

```text
0x1000000
```

### OTA Layouts Require Two Application Partitions

If OTA is required, replace a single `factory` partition with two application partitions:

```text
ota_0
ota_1
```

You must also include an `otadata` partition.

Example structure:

```csv
# Name,     Type, SubType, Offset,   Size,   Flags
nvs,        data, nvs,     0x9000,   0x6000,
otadata,    data, ota,     0xe000,   0x2000,
app0,       app,  ota_0,   0x10000,  3M,
app1,       app,  ota_1,   0x310000, 3M,
spiffs,     data, spiffs,  0x610000, 9.9M,
```

The exact values must be adjusted so that all partitions fit within the physical flash.

## 6. Example Large-Application Layout

If you later need a large application—for example, because you add TensorFlow Lite, audio processing, or a large local web interface—you could create a layout with a larger application partition.

Example concept:

```csv
# Name,     Type, SubType, Offset,   Size,   Flags
nvs,        data, nvs,     0x9000,   0x6000,
phy_init,   data, phy,     0xf000,   0x1000,
factory,    app,  factory, 0x10000,  6M,
fatfs,      data, fat,     0x610000, 9.9M,
```

This layout provides:

```text
Application: approximately 6 MB
Data storage: approximately 9.9 MB
```

The actual layout must be calculated carefully to avoid overlapping partitions and to remain within the 16 MB flash limit.

## 7. Practical Tips

### Erase Flash After Changing the Partition Scheme

After changing from a preset to another preset or to a custom partition table, perform one upload with:

```text
Tools → Erase All Flash Before Sketch Upload → Enabled
```

Old NVS data at incompatible offsets can cause:

- Boot loops.
- Provisioning failures.
- Wi-Fi credentials not being saved.
- RainMaker association problems.
- Unexpected crashes.

> **Warning:** Erasing all flash removes stored Wi-Fi credentials, RainMaker provisioning data, and file-system contents.

### Fixing `Sketch too big`

The `Sketch too big` error means the compiled firmware is larger than the application partition in the selected scheme.

To fix it:

1. Create a custom `partitions.csv`.
2. Assign a larger application partition.
3. Select:

   ```text
   Tools → Partition Scheme → Custom
   ```

4. Erase the flash.
5. Compile and upload again.

### RainMaker Presets

The RainMaker 4 MB and RainMaker 8 MB entries are tuned layouts for boards with those flash sizes.

They are not necessarily the best choices for a 16 MB N16R8 board.

For an N16R8, use either:

```text
16M Flash (3 MB APP / 9.9 MB FATFS)
```

or a custom CSV layout.

## 8. Bottom Line

The 3 MB application limit in `Huge APP` is a preset-layout choice, not a limitation of the ESP32-S3 chip.

The ESP32-S3 can run applications larger than 3 MB when the flash size and partition table support it.

For an N16R8 board:

```text
Flash Size:      16 MB
PSRAM:           OPI PSRAM
Partition Scheme: 16M Flash or Custom
```

The `Custom` partition scheme allows you to create an application partition of:

```text
6 MB
10 MB
```

or another size that fits within the 16 MB flash device.

The essential requirement is that all partitions—including NVS, PHY initialization, application storage, OTA data, and file storage—fit within the selected flash size without overlapping.
