#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

// ============================================================
// 1. USER CONFIGURATION
// ============================================================

const char* WIFI_SSID = "Nightingale_IoT";
const char* WIFI_PASSWORD = "1122334455";

//#define GITHUB_RELEASE_URL

//#define DEBUG_TRACE

// Version of THIS compiled firmware.
// Change this when building a new release.
#define FIRMWARE_VERSION "1.6.0"

const char* MANIFEST_URL = "https://raw.githubusercontent.com/ooikk/Arduino-Documentation/main/02_MISC/02_OTA/version.txt";
// "https://raw.githubusercontent.com/"
// "YOUR_GITHUB_USERNAME/YOUR_REPO/main/version.txt";

#ifdef GITHUB_RELEASE_URL
const char* FIRMWARE_URL = "https://github.com/ooikk/Arduino-Documentation/releases/latest/download/firmware.bin";
#else
const char* FIRMWARE_URL = "https://raw.githubusercontent.com/ooikk/Arduino-Documentation/main/02_MISC/02_OTA/firmware.bin";
#endif

// "https://github.com/ooikk/"
// "YOUR_REPO/releases/latest/download/firmware.bin";

// Check every 6 hours while running.
//const unsigned long OTA_CHECK_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;

const unsigned long OTA_CHECK_INTERVAL_MS = 2UL * 60UL * 1000UL;  // 2 minutes

// Avoid downloading unexpectedly large files.
const size_t MAX_FIRMWARE_SIZE = 8UL * 1024UL * 1024UL;

// ============================================================
// 2. TLS ROOT CERTIFICATE
// DigiCert Global Root G2
// Obtain/verify the current PEM from DigiCert's official site.
// ============================================================

static const char ROOT_CA[] PROGMEM = R"PEM(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)PEM";


// ============================================================
// 3. STATE
// ============================================================

unsigned long lastOtaCheckMs = 0;
bool otaCheckHasRun = false;
bool otaInProgress = false;

// ============================================================
// 4. VERSION COMPARISON
// Supports versions such as 1.0.0, 1.2.3, 2.0.0
// ============================================================

bool parseVersion(const String& text,
                  unsigned long& major,
                  unsigned long& minor,
                  unsigned long& patch) {
  unsigned int a, b, c;
  char extra;

  if (sscanf(text.c_str(), "%u.%u.%u%c", &a, &b, &c, &extra) != 3) {
    return false;
  }

  major = a;
  minor = b;
  patch = c;
  return true;
}

int compareVersions(const String& a, const String& b) {
  unsigned long a1, a2, a3;
  unsigned long b1, b2, b3;

  if (!parseVersion(a, a1, a2, a3) || !parseVersion(b, b1, b2, b3)) {
    return 0;  // Invalid versions are rejected separately.
  }

  if (a1 != b1) return a1 > b1 ? 1 : -1;
  if (a2 != b2) return a2 > b2 ? 1 : -1;
  if (a3 != b3) return a3 > b3 ? 1 : -1;

  return 0;
}

// ============================================================
// 5. READ AND VALIDATE THE MANIFEST
//
// version.txt format:
// 1.0.1
// 64-character SHA-256 hash
// ============================================================


bool fetchManifest(String& remoteVersion, String& expectedSha256) {
  WiFiClientSecure tls;
  tls.setCACert(ROOT_CA);
  tls.setTimeout(15000);

  HTTPClient http;
  http.setConnectTimeout(15000);
  http.setTimeout(20000);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  Serial.println("Checking GitHub manifest...");
  Serial.printf("Manifest URL: %s\n", MANIFEST_URL);

  if (!http.begin(tls, MANIFEST_URL)) {
    Serial.println("Manifest: HTTP begin failed");
    return false;
  }

  int code = http.GET();

  Serial.printf("Manifest HTTP result: %d\n", code);

#ifdef DEBUG_TRACE
  if (code < 0) {
    Serial.printf("HTTP error: %s\n",
                  http.errorToString(code).c_str());

    char tlsError[160] = { 0 };
    int tlsErrorCode = tls.lastError(tlsError, sizeof(tlsError));

    Serial.printf("TLS error code: %d\n", tlsErrorCode);
    Serial.printf("TLS error detail: %s\n", tlsError);

    http.end();
    return false;
  }
#endif
  
  if (code != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  // Read the body BEFORE closing the connection.
  String body = http.getString();
  http.end();

  body.trim();

  int newline = body.indexOf('\n');
  if (newline < 0) {
    Serial.println("Manifest format error: missing hash line");
    return false;
  }

  remoteVersion = body.substring(0, newline);
  remoteVersion.trim();

  expectedSha256 = body.substring(newline + 1);
  expectedSha256.trim();
  expectedSha256.toLowerCase();

  unsigned long a, b, c;
  if (!parseVersion(remoteVersion, a, b, c)) {
    Serial.println("Manifest version is invalid");
    return false;
  }

  if (expectedSha256.length() != 64) {
    Serial.println("Manifest SHA-256 must be 64 hex characters");
    return false;
  }

  for (size_t i = 0; i < expectedSha256.length(); i++) {
    char ch = expectedSha256[i];
    if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'))) {
      Serial.println("Manifest contains invalid SHA-256 characters");
      return false;
    }
  }

  Serial.printf("Installed version: %s\n", FIRMWARE_VERSION);
  Serial.printf("Published version: %s\n", remoteVersion.c_str());

  return true;
}


// ============================================================
// 6. DOWNLOAD AND INSTALL FIRMWARE
// ============================================================

bool downloadAndInstall(const String& expectedSha256) {
  WiFiClientSecure tls;

#ifdef GITHUB_RELEASE_URL
// Bypass certificate verification for GitHub's complex redirect chain
  tls.setInsecure(); 
#else
  tls.setCACert(ROOT_CA);
#endif
  tls.setTimeout(20000);

  HTTPClient http;
  http.setConnectTimeout(20000);
  http.setTimeout(30000);


#ifdef GITHUB_RELEASE_URL
  // Enable following redirects so GitHub release assets download properly from the CDN
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
#else
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
#endif


  Serial.println("Connecting to GitHub firmware asset...");

  if (!http.begin(tls, FIRMWARE_URL)) {
    Serial.println("Firmware: HTTP begin failed");
    return false;
  }

  int code = http.GET();

#ifdef DEBUG_TRACE
  if (code < 0) {
    Serial.printf("Firmware HTTP error: %s\n",
                  http.errorToString(code).c_str());

    char tlsError[160] = { 0 };
    int tlsErrorCode = tls.lastError(tlsError, sizeof(tlsError));

    Serial.printf("Firmware TLS error code: %d\n", tlsErrorCode);
    Serial.printf("Firmware TLS detail: %s\n", tlsError);

    http.end();
    return false;
  }
#endif
  

  if (code != HTTP_CODE_OK) {
    Serial.printf("Firmware HTTP error: %d\n", code);
    http.end();
    return false;
  }

  int contentLength = http.getSize();

  if (contentLength <= 0 || (size_t)contentLength > MAX_FIRMWARE_SIZE) {
    Serial.printf("Invalid firmware size: %d\n", contentLength);
    http.end();
    return false;
  }

  const esp_partition_t* target =
    esp_ota_get_next_update_partition(nullptr);

  if (target == nullptr) {
    Serial.println("No OTA target partition; update cancelled");
    http.end();
    return false;
  }

  Serial.printf("Target partition: %s\n", target->label);
  Serial.printf("Firmware size: %d bytes\n", contentLength);

  if ((size_t)contentLength > target->size) {
    Serial.println("Firmware does not fit OTA partition");
    http.end();
    return false;
  }

  if (!Update.begin((size_t)contentLength, U_FLASH)) {
    Serial.printf("Update.begin failed: %s\n",
                  Update.errorString());
    http.end();
    return false;
  }

  // Must be set after begin() and before writing firmware bytes.
  if (!Update.setSHA256(expectedSha256.c_str())) {
    Serial.println("Could not configure SHA-256 verification");
    Update.abort();
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  Serial.printf("Firmware URL: %s\n", FIRMWARE_URL);
  Serial.println("Downloading firmware...");

  size_t written = Update.writeStream(*stream);

  Serial.printf("Bytes written: %u / %u\n",
                (unsigned)written,
                (unsigned)contentLength);

  http.end();

  if (written != (size_t)contentLength) {
    Serial.println("Incomplete download; aborting update");
    Update.abort();
    return false;
  }

  // Update.end() validates the image and the configured SHA-256.
  if (!Update.end()) {
    Serial.printf("Update.end failed: %s\n",
                  Update.errorString());
    return false;
  }

  if (!Update.isFinished()) {
    Serial.println("Update did not finish correctly");
    return false;
  }

  Serial.println("Firmware verified and installed.");
  Serial.println("Rebooting into the new firmware...");
  delay(1000);
  ESP.restart();

  return true;  // Normally not reached.
}

// ============================================================
// 7. CHECK FOR A NEWER RELEASE
// ============================================================

void checkForFirmwareUpdate() {
  if (otaInProgress) return;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("OTA check skipped: Wi-Fi disconnected");
    return;
  }

  otaInProgress = true;

  String remoteVersion;
  String expectedSha256;

  if (!fetchManifest(remoteVersion, expectedSha256)) {
    Serial.println("OTA check failed; existing firmware retained");
    otaInProgress = false;
    return;
  }

  int comparison =
    compareVersions(remoteVersion, String(FIRMWARE_VERSION));

  if (comparison < 0) {
    Serial.println("Server version is older; ignoring it");
    otaInProgress = false;
    return;
  }

  if (comparison == 0) {
    Serial.println("Firmware is already up to date");
    otaInProgress = false;
    return;
  }

  Serial.println("New firmware version available");

  downloadAndInstall(expectedSha256);

  // Normally a successful update reboots before reaching here.
  otaInProgress = false;
}

// ============================================================
// 8. WIFI
// ============================================================

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < 30000UL) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected. IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi unavailable; OTA will retry later");
  }
}


// ============================================================
// XX. SETUP AND LOOP
// ============================================================
#ifdef DEBUG_TRACE

void testGitHubTLS1() {
  WiFiClientSecure tls;
  tls.setCACert(ROOT_CA);
  tls.setTimeout(15000);

  Serial.println("Testing HTTPS to raw.githubusercontent.com...");

  if (tls.connect("raw.githubusercontent.com", 443)) {
    Serial.println("TLS connection succeeded");
    tls.stop();
  } else {
    char errorBuffer[160] = { 0 };
    int err = tls.lastError(errorBuffer, sizeof(errorBuffer));
    Serial.printf("TLS failed: %d, %s\n", err, errorBuffer);
  }
}

void testReleaseAssetTLS() {
  WiFiClientSecure testClient;
  testClient.setCACert(ROOT_CA);
  testClient.setTimeout(15000);

  Serial.println("Testing TLS to release-assets.githubusercontent.com...");

  if (testClient.connect("release-assets.githubusercontent.com", 443)) {
    Serial.println("Release asset host TLS connection succeeded");
    testClient.stop();
  } else {
    Serial.println("Release asset host TLS connection FAILED");

    char tlsError[160] = { 0 };
    int errorCode = testClient.lastError(tlsError, sizeof(tlsError));
    Serial.printf("TLS error code: %d\n", errorCode);
    Serial.printf("TLS detail: %s\n", tlsError);
  }
}

void testGitHubTLS() {
  WiFiClientSecure testClient;
  testClient.setCACert(ROOT_CA);
  testClient.setTimeout(15000);

  Serial.println("Testing TLS to github.com...");

  if (testClient.connect("github.com", 443)) {
    Serial.println("github.com TLS connection succeeded");
    testClient.stop();
  } else {
    Serial.println("github.com TLS connection FAILED");

    char tlsError[160] = { 0 };
    int errorCode = testClient.lastError(tlsError, sizeof(tlsError));

    Serial.printf("TLS error code: %d\n", errorCode);
    Serial.printf("TLS detail: %s\n", tlsError);
  }
}

#endif

// ============================================================
// 9. SETUP AND LOOP
// ============================================================



void setup() {
  Serial.begin(115200);
  delay(1000);

#ifdef DEBUG_TRACE
  Serial.setDebugOutput(true);
#endif

  Serial.println();
  Serial.println("ESP32-S3 Automatic Pull OTA");
  Serial.printf("Running firmware version: %s\n",
                FIRMWARE_VERSION);

  const esp_partition_t* running =
    esp_ota_get_running_partition();

  const esp_partition_t* next =
    esp_ota_get_next_update_partition(nullptr);

  if (running) {
    Serial.printf("Running partition: %s, size=%u\n",
                  running->label,
                  (unsigned)running->size);
  }

  if (next) {
    Serial.printf("Next OTA partition: %s, size=%u\n",
                  next->label,
                  (unsigned)next->size);
  } else {
    Serial.println("WARNING: no OTA target partition found");
  }

  connectWiFi();

#ifdef DEBUG_TRACE
  testGitHubTLS();
  testReleaseAssetTLS();
#endif

  // Check once at startup if Wi-Fi connected.
  if (WiFi.status() == WL_CONNECTED && next != nullptr) {
    checkForFirmwareUpdate();
  }

  lastOtaCheckMs = millis();
  otaCheckHasRun = true;
}

uint32_t lastHeartbeat = 0;

void loop() {
  // Reconnect Wi-Fi if it drops.
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastReconnectAttempt = 0;

    if (millis() - lastReconnectAttempt >= 15000UL) {
      lastReconnectAttempt = millis();
      Serial.println("Attempting Wi-Fi reconnect...");
      WiFi.reconnect();
    }
  }

  // Check every six hours after the startup check.
  if (otaCheckHasRun && millis() - lastOtaCheckMs >= OTA_CHECK_INTERVAL_MS) {
    lastOtaCheckMs = millis();

    if (WiFi.status() == WL_CONNECTED) {
      checkForFirmwareUpdate();
    } else {
      Serial.println("Scheduled OTA check skipped: no Wi-Fi");
    }
  }

  if (
    millis() - lastHeartbeat >= 10000) {
    lastHeartbeat = millis();

    Serial.printf("****Github Auto OTA: Github release****\n");
    Serial.printf("Firmware %s, Wi-Fi status %d\n", FIRMWARE_VERSION, (int)WiFi.status());
    Serial.printf("Temperature %.1f deg\n", (float)random(200, 400) / 10.0f);
  }

  // Your normal application tasks can run here.
  delay(10);
}