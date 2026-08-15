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

---
# References

https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-time.c

https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/Time/SimpleTime/SimpleTime.ino

https://github.com/espressif/newlib-esp32/blob/master/newlib/libc/time/strftime.c

https://github.com/m5stack/azure_iothub_arduino_lib_esp32/blob/master/hardware/espressif/esp32/tools/sdk/include/newlib/time.h

