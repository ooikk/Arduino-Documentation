# Display Sync and Display Time

To display the local time on your ESP32-S3 aligned with Singapore Time (SGT, which is UTC+8 with no Daylight Saving Time), pass Singapore's time zone configuration string to `configTime()` and format the resulting system time using C++ standard library functions.

## 1. Time Zone Configuration String

Singapore operates on a fixed offset of UTC+8 hours. In standard POSIX time zone format, the string for Singapore is:

```text
"SGT-8"
```

### How POSIX Strings Work

The offset sign is inverted relative to standard UTC notation. `SGT-8` tells the C runtime library that Singapore Time (SGT) is 8 hours ahead of UTC:

```text
UTC = Local Time - 8 hours
```

Because Singapore does not observe Daylight Saving Time, no additional DST rule is needed.

## 2. Updated `configTime()` and Print Function

Update your `configTime()` call to pass `SGT-8` as the time zone rule, and use `getLocalTime()` or `localtime_r()` to format and print the output.

```cpp
#include <time.h>

// ----------------------------
// Sync NTP with Singapore Time Zone
// ----------------------------
void syncTimeSingapore() {
  // POSIX time zone string for Singapore (UTC+8, no DST)
  const char* ntpServer1 = "pool.ntp.org";
  const char* ntpServer2 = "time.nist.gov";
  const char* timeZone   = "SGT-8";

  // configTzTime applies the time zone rule directly during sync
  configTzTime(timeZone, ntpServer1, ntpServer2);

  Serial.print("Syncing time with NTP (Singapore UTC+8)");

  time_t now = time(nullptr);

  // Wait until NTP updates the clock past epoch timestamp 1650000000
  // This timestamp corresponds to a date in 2022.
  while (now < 1650000000) {
    delay(250);
    Serial.print(".");
    time(&now);
  }

  Serial.println(" -> Synced!");
}

// ----------------------------
// Helper Function to Print Formatted Local Time
// ----------------------------
void printLocalTime() {
  struct tm timeinfo;

  // getLocalTime() handles conversion to the local time zone
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  // Format options:
  // %Y = Year (4 digits)
  // %m = Month (2 digits)
  // %d = Day (2 digits)
  // %H = Hour (24-hour format)
  // %M = Minute
  // %S = Second
  // %A = Day of the week
  // Ensure square brackets [] are used so timeStringBuff is a char array (char*)
  char timeStringBuff[50];

  strftime(
    timeStringBuff,
    sizeof(timeStringBuff),
    "%Y-%m-%d %H:%M:%S (%A)",
    &timeinfo
  );

  Serial.print("Current Singapore Time: ");
  Serial.println(timeStringBuff);
}
```

## 3. Integration into Your Existing `setup()` and `loop()`

Call `syncTimeSingapore()` inside `setup()` immediately after `connectWiFi()`. Call `printLocalTime()` whenever telemetry is published.

```cpp
void setup() {
  Serial.begin(115200);
  delay(500);

  connectWiFi();

  // Sync NTP aligned to Singapore Time
  syncTimeSingapore();

  // Print the current time once during bootup
  printLocalTime();

  // Attach the root certificate for Adafruit IO HTTPS/TLS
  secureClient.setCACert(adafruit_root_ca);
  secureClient.setHandshakeTimeout(30);

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqtt.connected()) {
    connectMQTT();
  }

  mqtt.loop();

  // Print local time periodically along with telemetry
  if (millis() - lastTelemetryMs >= TELEMETRY_PERIOD_MS) {
    lastTelemetryMs = millis();

    // Example:
    // Current Singapore Time: 2026-08-14 22:05:58 (Friday)
    printLocalTime();

    publishTelemetry();
  }

  delay(10);
}
```

## `strftime()` Specifiers Quick Reference

| Specifier | Description | Example |
|---|---|---|
| `%Y` | 4-digit year | `2026` |
| `%m` | 2-digit month | `08` |
| `%d` | 2-digit day | `14` |
| `%H` | Hour in 24-hour format | `22` |
| `%I` | Hour in 12-hour format | `10` |
| `%p` | AM/PM indicator | `PM` |
| `%M` | Minute | `05` |
| `%S` | Second | `58` |
| `%A` | Full weekday name | `Friday` |


## World Time & Date
To handle any location in the world—including automatic adjustment for Daylight Saving Time (DST)—we rely on POSIX time zone strings.

Instead of hardcoding a fixed offset such as UTC+8, POSIX strings specify the standard offset, the DST offset, and the exact rules for when the clock changes.

## 1. How Daylight Saving Time Works in POSIX

In places like New York, the time changes twice a year:

- **Standard Time (EST):** UTC−5 hours; begins on the first Sunday of November.
- **Daylight Saving Time (EDT):** UTC−4 hours; begins on the second Sunday of March.

Rather than writing custom code to check the date and add or subtract an hour, the underlying C runtime library in the ESP32, such as `time.h` and Newlib, calculates the offset automatically using this string:

```text
"EST5EDT,M3.2.0,M11.1.0"
```

### Deconstructing the New York String

- `EST5`: Standard time name (`EST`) and offset from UTC, meaning 5 hours behind UTC.
- `EDT`: Daylight Saving Time name (`EDT`). If no offset follows, it defaults to 1 hour ahead of standard time, which is UTC−4.
- `M3.2.0`: DST starts in month 3, March, on week 2, day 0, Sunday, at 02:00.
- `M11.1.0`: DST ends in month 11, November, on week 1, day 0, Sunday, at 02:00.

When you pass this string to `setenv("TZ", rule, 1)`, calling `getLocalTime()` or `localtime_r()` automatically applies DST or Standard Time according to the current date.

## 2. Global Time-Printing Function

The following flexible C++ function for the ESP32-S3 converts the current synchronized system time to any time zone on demand:

```cpp
#include <Arduino.h>
#include <time.h>

// -------------------------------------------------------------
// POSIX Time Zone Strings for Common World Regions
// -------------------------------------------------------------
const char* TZ_SINGAPORE = "SGT-8";                       // UTC+8 (No DST)
const char* TZ_NEW_YORK  = "EST5EDT,M3.2.0,M11.1.0";      // US Eastern (Automatic DST)
const char* TZ_LONDON    = "GMT0BST,M3.5.0/1,M10.5.0/2";   // UK Time (Automatic BST/GMT)
const char* TZ_TOKYO     = "JST-9";                        // Japan (No DST)
const char* TZ_PARIS     = "CET-1CEST,M3.5.0,M10.5.0/3";   // Central Europe (Automatic DST)

// -------------------------------------------------------------
// Print Time for Any World Location
// -------------------------------------------------------------
void printWorldTime(const char* locationName, const char* tzRule) {
  // 1. Temporarily set the environment time zone rule
  setenv("TZ", tzRule, 1);
  tzset(); // Apply the change

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  // 2. Format the output string
  char timeBuffer;

  // %Z prints the active time zone abbreviation, such as EST or EDT
  strftime(
    timeBuffer,
    sizeof(timeBuffer),
    "%Y-%m-%d %H:%M:%S (%A) %Z",
    &timeinfo
  );

  Serial.printf("%-12s: %s\n", locationName, timeBuffer);

  // 3. Reset to Singapore local time zone as the default
  setenv("TZ", TZ_SINGAPORE, 1);
  tzset();
}
```

## 3. Practical Example Code

You can test multiple world locations in `loop()` or `setup()` after NTP synchronization:

```cpp
void setup() {
  Serial.begin(115200);
  delay(500);

  connectWiFi();

  // Initial NTP synchronization using pool servers
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Syncing NTP Time");

  time_t now = time(nullptr);

  while (now < 1650000000) {
    delay(250);
    Serial.print(".");
    time(&now);
  }

  Serial.println(" -> Synced!\n");

  // Print current times across the world
  Serial.println("=== Global Clock Readout ===");

  printWorldTime("Singapore", TZ_SINGAPORE);
  printWorldTime("New York",  TZ_NEW_YORK);
  printWorldTime("London",    TZ_LONDON);
  printWorldTime("Tokyo",     TZ_TOKYO);
}

void loop() {
  // Nothing is needed here for a one-shot test
}
```

### Expected Serial Monitor Output

```text
=== Global Clock Readout ===
Singapore   : 2026-08-15 12:45:10 (Saturday) SGT
New York    : 2026-08-15 00:45:10 (Saturday) EDT
London      : 2026-08-15 05:45:10 (Saturday) BST
Tokyo       : 2026-08-15 13:45:10 (Saturday) JST
```

New York automatically displays `EDT`, or Eastern Daylight Time, in August because DST is active.

## Cheat Sheet: Common POSIX Strings

| Location | POSIX String | DST Notes |
|---|---|---|
| Singapore / Malaysia | `SGT-8` | No DST |
| US Eastern, New York | `EST5EDT,M3.2.0,M11.1.0` | Starts in March, week 2; ends in November, week 1 |
| US Pacific, Los Angeles | `PST8PDT,M3.2.0,M11.1.0` | Starts in March, week 2; ends in November, week 1 |
| United Kingdom, London | `GMT0BST,M3.5.0/1,M10.5.0/2` | Starts on the last Sunday in March; ends on the last Sunday in October |
| Europe, Berlin/Paris | `CET-1CEST,M3.5.0,M10.5.0/3` | Starts on the last Sunday in March; ends on the last Sunday in October |
| Japan / Korea | `JST-9` | No DST |

## POSIX Time Zone
To find the exact POSIX Time Zone string, also known as a TZ string, for any country or city, you can reference standardized databases and tools that compile the IANA (Olson) Time Zone Database into POSIX format.

Here are the best sources and methods for obtaining these strings.

## 1. Official and Curated GitHub Repositories

### Quickest Reference

The most comprehensive, community-maintained list specifically formatted for embedded microcontrollers, such as ESP32 and ESP8266, is hosted on GitHub:

[GNU ARM Embedded `zoneinfo` repository](https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv)

This repository provides a `zones.csv` file that maps standard IANA names, such as `America/New_York`, `Asia/Singapore`, and `Europe/London`, directly to their POSIX string equivalents.

[ESP32 Community Wiki Zone Information](https://www.google.com/search?q=https://github.com/espressif/arduino-esp32/blob/master/tools/sdk/esp32/include/newlib/platform_include/sys/time.h&authuser=2)

Espressif uses standard GCC Newlib time zone rules derived from the GNU C Library (glibc) database.

## 2. Generate a String Using Linux or macOS Terminal

If you use Linux, macOS, or WSL on Windows, the system's `zdump` tool can output time zone information for any region in the IANA database.

```bash
# Get time zone information for New York
zdump -v /usr/share/zoneinfo/America/New_York | tail -n 2

# Get time zone information for London
zdump -v /usr/share/zoneinfo/Europe/London | tail -n 2
```

## 3. How POSIX TZ Strings Are Formatted

If a city is not included in a lookup table, you can construct or verify its POSIX string using the standard POSIX syntax:

```text
STDoffset[DST[offset][,start[/time],end[/time]]]
```

### Field Breakdown

- **Standard Time Name (`EST`):** Three or more alphabetic characters identifying Standard Time.
- **Standard Offset (`5`):** The number of hours behind UTC. For example, UTC−5 is represented as `5`, while UTC+8 is represented as `-8`.
- **Important:** POSIX reverses the mathematical sign convention used for UTC offsets.
- **Daylight Time Name (`EDT`):** Three or more characters identifying Daylight Saving Time.
- **DST Offset:** Optional. If omitted, it defaults to 1 hour ahead of Standard Time.
- **Start Rule (`M3.2.0`):**
  - `M3`: Month 3, March.
  - `.2`: The second occurrence of the specified weekday in that month.
  - `.0`: Day of the week, where `0 = Sunday`, `1 = Monday`, through `6 = Saturday`.
- **End Rule (`M11.1.0`):**
  - `M11`: Month 11, November.
  - `.1`: The first occurrence of the specified weekday in that month.
  - `.0`: Sunday.

## Cheat Sheet for Major International Hubs

| City / Region | IANA Identifier | POSIX TZ String |
|---|---|---|
| Singapore | `Asia/Singapore` | `SGT-8` |
| New York, US East | `America/New_York` | `EST5EDT,M3.2.0,M11.1.0` |
| Los Angeles, US West | `America/Los_Angeles` | `PST8PDT,M3.2.0,M11.1.0` |
| Chicago, US Central | `America/Chicago` | `CST6CDT,M3.2.0,M11.1.0` |
| London, UK | `Europe/London` | `GMT0BST,M3.5.0/1,M10.5.0/2` |
| Paris / Berlin, CET | `Europe/Paris` | `CET-1CEST,M3.5.0,M10.5.0/3` |
| Tokyo, Japan | `Asia/Tokyo` | `JST-9` |
| Sydney, Australia | `Australia/Sydney` | `AEST-10AEDT,M10.1.0,M4.1.0/3` |


---

# References

https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-time.c

https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/Time/SimpleTime/SimpleTime.ino

https://github.com/espressif/newlib-esp32/blob/master/newlib/libc/time/strftime.c

https://github.com/m5stack/azure_iothub_arduino_lib_esp32/blob/master/hardware/espressif/esp32/tools/sdk/include/newlib/time.h

