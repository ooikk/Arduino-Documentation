# Time and Date Functions

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


# Extracting Integer Time Values on the ESP32-S3

To extract individual integer values—year, month, day, hour, minute, and second—from synchronized time on the ESP32-S3, read the member fields directly from the standard `struct tm` populated by `getLocalTime()`.

## Structure of `struct tm`

The `struct tm` structure in the C standard library breaks down date and time into integer fields:

| Field | Variable Name | Data Type | Range / Note |
|---|---|---|---|
| Year | `timeinfo.tm_year` | `int` | Years since 1900; add 1900 |
| Month | `timeinfo.tm_mon` | `int` | 0 to 11; `0 = January`, so add 1 |
| Day of month | `timeinfo.tm_mday` | `int` | 1 to 31 |
| Hour | `timeinfo.tm_hour` | `int` | 0 to 23; 24-hour format |
| Minute | `timeinfo.tm_min` | `int` | 0 to 59 |
| Second | `timeinfo.tm_sec` | `int` | 0 to 59 |
| Day of week | `timeinfo.tm_wday` | `int` | 0 to 6; `0 = Sunday`, `1 = Monday`, etc. |
| Day of year | `timeinfo.tm_yday` | `int` | 0 to 365; zero-based day index |

> **Important:** Always add `1900` to `tm_year` and `1` to `tm_mon`.

## C++ Function to Extract Integers

The following standalone function extracts each time field as an integer:

```cpp
#include <Arduino.h>
#include <time.h>

void getTimeAsIntegers() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  // Extract individual fields into integers
  int year   = timeinfo.tm_year + 1900; // tm_year is years since 1900
  int month  = timeinfo.tm_mon + 1;     // tm_mon is zero-indexed (0 = January)
  int day    = timeinfo.tm_mday;        // 1-31
  int hour   = timeinfo.tm_hour;        // 0-23
  int minute = timeinfo.tm_min;         // 0-59
  int second = timeinfo.tm_sec;         // 0-59
  int wday   = timeinfo.tm_wday;        // 0 = Sunday, 1 = Monday, ..., 6 = Saturday

  // Print extracted integers
  Serial.println("=== Extracted Time Integers ===");
  Serial.printf("Year   : %d\n", year);
  Serial.printf("Month  : %d\n", month);
  Serial.printf("Day    : %d\n", day);
  Serial.printf("Hour   : %d\n", hour);
  Serial.printf("Minute : %d\n", minute);
  Serial.printf("Second : %d\n", second);
  Serial.printf("Day/Wk : %d\n", wday);

  // Example: Combine into a single numeric timestamp format (YYYYMMDDHHMMSS)
  uint64_t numericTimestamp = (uint64_t)year * 10000000000ULL +
                              (uint64_t)month * 100000000ULL +
                              (uint64_t)day * 1000000ULL +
                              (uint64_t)hour * 10000ULL +
                              (uint64_t)minute * 100ULL +
                              (uint64_t)second;

  Serial.printf("Compact Numeric ID: %llu\n", numericTimestamp);
}
```

## Getting Raw Unix Epoch Time

If you need a single 32-bit or 64-bit integer representing the total elapsed seconds since January 1, 1970, the Unix Epoch, use `time(nullptr)` or `mktime()`:

```cpp
time_t now = time(nullptr);

// Returns the current Unix timestamp as an integer,
// for example, 1786799416.
Serial.printf("Unix Epoch Timestamp: %ld\n", (long)now);
```

# Date-Based SD Card Logging on the ESP32-S3

Logging data to an SD card using date-based file names, such as `/logs/2026-08-15.csv`, is a reliable pattern for long-running ESP32-S3 data loggers.

By extracting date integers from `struct tm`, you can automatically create a new CSV file at midnight or append data to the existing file for the current day.

## SD Card Wiring

### SPI Mode on the ESP32-S3

| SD Card Module | ESP32-S3 Default SPI Pin |
|---|---|
| CS (Chip Select) | GPIO 10, or any available GPIO |
| MOSI | GPIO 11 |
| MISO | GPIO 13 |
| SCK (CLK) | GPIO 12 |
| VCC | 5V or 3.3V, depending on the module regulator |
| GND | GND |

## Complete Implementation

This sketch initializes Wi-Fi and NTP, mounts the SD card, builds daily log file names based on date integers, and appends data entries.

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <SPI.h>
#include <SD.h>

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// SD card CS pin
#define SD_CS_PIN 10

// Track the current log file name to detect midnight transitions
char currentFileName = "";

// -------------------------------------------------------------
// Initialize Wi-Fi and NTP Time Synchronization
// -------------------------------------------------------------
void initTime() {
  WiFi.begin(ssid, password);

  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println(" Connected!");

  // Set Singapore time zone (UTC+8) and synchronize using NTP
  configTzTime("SGT-8", "pool.ntp.org", "time.nist.gov");

  struct tm timeinfo;

  while (!getLocalTime(&timeinfo)) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nTime Synchronized!");
}

// -------------------------------------------------------------
// Generate a Daily CSV Filename Using Date Integers
// -------------------------------------------------------------
void getDailyFileName(char* buffer, size_t maxLen) {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    snprintf(buffer, maxLen, "/logs/error.csv");
    return;
  }

  // Extract year, month, and day integers
  int year  = timeinfo.tm_year + 1900;
  int month = timeinfo.tm_mon + 1;
  int day   = timeinfo.tm_mday;

  // Format filename: /logs/YYYY-MM-DD.csv
  // Example: /logs/2026-08-15.csv
  snprintf(
    buffer,
    maxLen,
    "/logs/%04d-%02d-%02d.csv",
    year,
    month,
    day
  );
}

// -------------------------------------------------------------
// Append a Log Entry to the Current Daily File
// -------------------------------------------------------------
void logData(float sensorValue) {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to get time for logging");
    return;
  }

  // Determine today's file path
  char fileName;
  getDailyFileName(fileName, sizeof(fileName));

  // Check whether the file exists before opening it
  bool fileExists = SD.exists(fileName);
  // Open this file for appending. If it doesn't exist, create it.
  File file = SD.open(fileName, FILE_APPEND); 

  if (!file) {
    Serial.printf("Failed to open file: %s\n", fileName);
    return;
  }

  // Write the header if this is a newly created daily file
  if (!fileExists) {
    file.println(
      "Timestamp,Year,Month,Day,Hour,Minute,Second,SensorValue"
    );

    Serial.printf("Created new daily log file: %s\n", fileName);
  }

  // Extract time integers for structured output
  int year   = timeinfo.tm_year + 1900;
  int month  = timeinfo.tm_mon + 1;
  int day    = timeinfo.tm_mday;
  int hour   = timeinfo.tm_hour;
  int minute = timeinfo.tm_min;
  int second = timeinfo.tm_sec;

  // Write the CSV row
  file.printf(
    "%04d-%02d-%02dT%02d:%02d:%02d,%d,%d,%d,%d,%d,%d,%.2f\n",
    year,
    month,
    day,
    hour,
    minute,
    second,
    year,
    month,
    day,
    hour,
    minute,
    second,
    sensorValue
  );

  file.close();

  Serial.printf("Logged to %s: %.2f\n", fileName, sensorValue);
}

// -------------------------------------------------------------
// Setup and Loop
// -------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  initTime();

  // Initialize the SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Card Mount Failed!");
    return;
  }

  // Ensure that the /logs directory exists
  if (!SD.exists("/logs")) {
    SD.mkdir("/logs");
  }

  Serial.println("SD Card Logging Ready.");
}

void loop() {
  // Simulate a sensor reading, such as voltage or temperature
  float mockSensorData = random(200, 350) / 10.0;

  // Log the data entry
  logData(mockSensorData);

  // Wait 10 seconds before the next log entry
  delay(10000);
}
```

## Key Operational Features

### Zero-Padded Date Formatting

Using `%04d-%02d-%02d` inside `snprintf()` ensures that dates such as August 5, 2026, are formatted as:

```text
2026-08-05.csv
```

instead of:

```text
2026-8-5.csv
```

This keeps directory listings naturally sorted by date.

### Automatic Header Creation

The `SD.exists()` check determines whether the daily file already exists. When a new day begins, the sketch automatically creates a new CSV file and writes its header before recording the first data row.

### Directory Management

`SD.mkdir("/logs")` organizes daily log files in a dedicated directory, preventing clutter in the root directory of the FAT filesystem.


# Calculating Elapsed Days on the ESP32-S3

Here is a robust function for calculating the number of days elapsed from a past date, or the number of days remaining until a future date, relative to the current NTP-synchronized time on your ESP32-S3.

## C++ Function Implementation

This implementation uses the standard C library functions `mktime()` and `difftime()`. It automatically handles leap years and variations in month length.

```cpp
#include <Arduino.h>
#include <time.h>

/**
 * Calculates the number of days elapsed since a given date (YYYY, MM, DD).
 *
 * Returns:
 *   - A positive integer for past dates.
 *   - A negative integer for future dates.
 *   - -999999 if the time has not yet synchronized through NTP.
 */
long getDaysSince(int year, int month, int day) {
  time_t now = time(nullptr);

  // Guard check: Ensure that NTP has synchronized a valid time
  if (now < 1650000000) {
    Serial.println("Error: NTP time not synced!");
    return -999999;
  }

  // 1. Build a struct tm for the target date at midnight
  struct tm targetDate = {0};

  targetDate.tm_year  = year - 1900; // Years since 1900
  targetDate.tm_mon   = month - 1;   // Months 0-11 (0 = January)
  targetDate.tm_mday  = day;         // Day of month, 1-31
  targetDate.tm_hour  = 0;
  targetDate.tm_min   = 0;
  targetDate.tm_sec   = 0;
  targetDate.tm_isdst = -1;          // Automatically detect DST

  // 2. Convert the target date to Unix Epoch seconds
  time_t targetEpoch = mktime(&targetDate);

  // 3. Calculate the difference in seconds and convert it to days
  double secondsDiff = difftime(now, targetEpoch);
  long days = (long)(secondsDiff / 86400.0);

  return days;
}
```

## String Parsing Variant

If the date is stored as a string, such as data read from an SD card or a configuration file, use `sscanf()` to parse it directly.

The expected format is `YYYY-MM-DD`.

```cpp
long getDaysSinceString(const char* dateStr) {
  int year;
  int month;
  int day;

  // Parse the format "YYYY-MM-DD"
  if (sscanf(dateStr, "%d-%d-%d", &year, &month, &day) == 3) {
    return getDaysSince(year, month, day);
  }

  Serial.println("Error: Invalid date format. Use YYYY-MM-DD.");
  return -999999;
}
```

## Usage Example in a Sketch

```cpp
void setup() {
  Serial.begin(115200);
  delay(1000);

  // 1. Connect to Wi-Fi and synchronize NTP
  WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }

  configTzTime("SGT-8", "pool.ntp.org", "time.nist.gov");

  // Wait for the clock to synchronize
  time_t now = time(nullptr);

  while (now < 1650000000) {
    delay(250);
    time(&now);
  }

  // 2. Calculate the number of days since specific dates
  long daysNewYear = getDaysSince(2026, 1, 1);

  Serial.printf(
    "Days since January 1, 2026: %ld days\n",
    daysNewYear
  );

  long daysFromStr = getDaysSinceString("2024-02-28");

  Serial.printf(
    "Days since February 28, 2024: %ld days\n",
    daysFromStr
  );
}

void loop() {
}
```

## Key Details

### `mktime()` Normalization

The standard library function `mktime()` automatically accounts for:

- Different month lengths, including 28, 30, and 31 days.
- Leap years.
- Daylight Saving Time rules when applicable.

### Division by `86400.0`

Dividing by `86400.0`, the number of seconds in a standard day, converts the time difference from seconds to days.

Casting the result to `long` truncates fractional days, preventing partial hours or seconds since midnight from affecting the integer day count.

# Calculating Differences in Years, Months, and Days

Calculating differences in terms of years, months, and days is slightly different from calculating total elapsed days because calendar months vary in length: 28, 30, or 31 days.

To obtain an exact, human-readable breakdown, use standard date arithmetic to step from the past date to the current NTP-synchronized date.

## Structure for Holding the Result

First, define a `struct` to return all three integer components:

```cpp
struct DateDiff {
  int years;
  int months;
  int days;
  bool isValid;
};
```

## C++ Function Implementation

This function compares a target date with the current local date on the ESP32-S3:

```cpp
#include <Arduino.h>
#include <time.h>

struct DateDiff {
  int years;
  int months;
  int days;
  bool isValid;
};

/**
 * Calculates the exact difference between a target date
 * (YYYY, MM, DD) and the current NTP-synchronized local date.
 *
 * The result is returned as years, months, and days.
 */
DateDiff getDateDifference(
  int startYear,
  int startMonth,
  int startDay
) {
  DateDiff diff = {0, 0, 0, false};

  struct tm nowInfo;

  if (!getLocalTime(&nowInfo)) {
    Serial.println("Error: Failed to obtain local time from NTP!");
    return diff;
  }

  // Current date integers
  int endYear  = nowInfo.tm_year + 1900;
  int endMonth = nowInfo.tm_mon + 1;
  int endDay   = nowInfo.tm_mday;

  // 1. Calculate the day difference, adjusting for month boundaries
  if (endDay < startDay) {
    // Determine the previous month to borrow days from
    int prevMonth = endMonth - 1;
    int prevYear = endYear;

    if (prevMonth < 1) {
      prevMonth = 12;
      prevYear--;
    }

    // Number of days in each month
    static const int daysInMonth[] = {
      0, 31, 28, 31, 30, 31,
      30, 31, 31, 30, 31, 30, 31
    };

    int daysInPrevMonth = daysInMonth[prevMonth];

    // Check for a leap year in February
    if (prevMonth == 2) {
      bool isLeap =
        (prevYear % 4 == 0 && prevYear % 100 != 0) ||
        (prevYear % 400 == 0);

      if (isLeap) {
        daysInPrevMonth = 29;
      }
    }

    endDay += daysInPrevMonth;
    endMonth--;
  }

  diff.days = endDay - startDay;

  // 2. Calculate the month difference, adjusting for year boundaries
  if (endMonth < startMonth) {
    endMonth += 12;
    endYear--;
  }

  diff.months = endMonth - startMonth;

  // 3. Calculate the year difference
  diff.years = endYear - startYear;
  diff.isValid = true;

  return diff;
}
```

## Practical Example

The following function calls `getDateDifference()` and prints the formatted result:

```cpp
void printDateDifference(int year, int month, int day) {
  DateDiff diff = getDateDifference(year, month, day);

  if (diff.isValid) {
    Serial.printf(
      "Difference from %04d-%02d-%02d to today: ",
      year,
      month,
      day
    );

    Serial.printf(
      "%d year(s), %d month(s), and %d day(s)\n",
      diff.years,
      diff.months,
      diff.days
    );
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Connect to Wi-Fi and synchronize NTP
  WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }

  configTzTime("SGT-8", "pool.ntp.org", "time.nist.gov");

  // Wait until the time is synchronized
  time_t now = time(nullptr);

  while (now < 1650000000) {
    delay(250);
    time(&now);
  }

  // Example test dates
  printDateDifference(2024, 2, 28);
  // Example output:
  // 2 year(s), 5 month(s), and 18 day(s)

  printDateDifference(2025, 12, 1);
}

void loop() {
}
```

## How the Borrowing Logic Works

### Days

If the current day of the month is smaller than the starting day—for example, today is the 15th but the target date was the 28th—the function borrows the exact number of days in the preceding month. It also accounts for leap years and decrements the ending month by one.

### Months

If the current month is smaller than the target month, the function borrows 12 months from the ending year and decrements the ending year by one.

### Years

The year difference is calculated by subtracting the starting year from the adjusted ending year:

```cpp
diff.years = endYear - startYear;
```

---

# References

https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-time.c

https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/Time/SimpleTime/SimpleTime.ino

https://github.com/espressif/newlib-esp32/blob/master/newlib/libc/time/strftime.c

https://github.com/m5stack/azure_iothub_arduino_lib_esp32/blob/master/hardware/espressif/esp32/tools/sdk/include/newlib/time.h


