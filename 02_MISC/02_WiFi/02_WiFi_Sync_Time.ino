#include <time.h>
#include <WiFi.h>

// ----------------------------
// User configuration
// ----------------------------
const char* WIFI_SSID = "Nightingale_IoT";
const char* WIFI_PASSWORD = "1122334455";

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

// ----------------------------
// Wi-Fi connection
// ----------------------------
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");

  uint32_t startMs = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startMs < 15000) {
    delay(250);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi connected. IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi connection failed.");
  }
}

void setup(){

  Serial.begin(115200);
  delay(500);

  connectWiFi();

  // Sync NTP aligned to Singapore Time
  syncTimeSingapore();

  // Print the current time once during bootup
  printLocalTime();
}

void loop(){

}