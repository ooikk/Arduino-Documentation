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
```


They are different operations and may both be required.
```
