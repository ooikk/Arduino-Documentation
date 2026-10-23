#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>


#define SECURE_SIGN
#define RELEASE_LOG


#ifdef SECURE_SIGN

#include <mbedtls/pk.h>
#include <mbedtls/sha256.h>

#else

//#define GITHUB_RELEASE_URL
//#define DEBUG_TRACE

#endif

// ============================================================
// 1. USER CONFIGURATION
// ============================================================

const char* WIFI_SSID = "Nightingale_IoT";
const char* WIFI_PASSWORD = "1122334455";
// Change this to the version currently running on the device.
static const char FIRMWARE_VERSION[] = "1.0.4";

#ifdef SECURE_SIGN
// IMPORTANT: replace these URLs with your repository paths.
// Redirects are intentionally disabled.
static const char MANIFEST_URL[] =
  "https://raw.githubusercontent.com/ooikk/Arduino-Documentation/main/02_MISC/02_OTA/02_Security/version.txt";

static const char SIGNATURE_URL[] =
  "https://raw.githubusercontent.com/ooikk/Arduino-Documentation/main/02_MISC/02_OTA/02_Security/version.sig";

static const char FIRMWARE_URL[] =
  "https://raw.githubusercontent.com/ooikk/Arduino-Documentation/main/02_MISC/02_OTA/02_Security/firmware.bin";

// Adjust to your partition layout. Update.begin() also checks
// whether the selected OTA partition has sufficient space.
static constexpr size_t MAX_MANIFEST_SIZE = 512;
static constexpr size_t MAX_FIRMWARE_SIZE = 4 * 1024 * 1024;
static constexpr size_t RSA_SIGNATURE_SIZE = 256;

// Check every 6 hours while running.
//const unsigned long OTA_CHECK_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;

const unsigned long OTA_CHECK_INTERVAL_MS = 2UL * 60UL * 1000UL;  // 2 minutes

// Paste the complete PEM public key exported from ota-public.pem.
// This is public verification material, NOT the private key.
static const char OTA_PUBLIC_KEY[] PROGMEM = R"KEY(
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAyuIpMJAhnNY/gGG/s/ZN
vVaPNwEv0sjB3MZkUBNyVRy6U0G/tM7pNuO3mEX+TdLeR4TouVjfwYfakIWcYE8k
z6wl+h29PIKl4Prky4t3xPdpCkFh4hCbhtxBIIlJGPkYTSn3O5wGdTEvdoEFpLYi
c2bTUmybiINZr2KcjuAi7dL32DQ3oJq/M/CeiCVm5Y6jRFyxw2MHRDvIDVyZubSk
94Bupr6bER9USq9ZUWGv77NIZ0Y0rEuigvpwshJp5avdURJxjznTPSWBd3zLH2tV
MzhLCt6tP40EncAigWjsd3b8rx94CJacKSglITwwewNR4MwT44wHDFR0x2Le9Z71
XwIDAQAB
-----END PUBLIC KEY-----
)KEY";

#else  // #ifdef SECURE_SIGN

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

#endif  // #ifdef SECURE_SIGN


// ============================================================
// 2. TLS ROOT CERTIFICATE
// DigiCert Global Root G2
// Obtain/verify the current PEM from DigiCert's official site.
// DigiCert Global Root G2 PEM
// https://cacerts.digicert.com/DigiCertGlobalRootG2.crt.pem
// ISRG Root X1
// https://letsencrypt.org/certs/isrgrootx1.pem
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

#ifdef SECURE_SIGN


// Download a small text response over validated HTTPS.
// Requires a known Content-Length and enforces a size limit.
bool fetchText(const char* url, String& output, size_t maxBytes) {
  WiFiClientSecure client;
  client.setCACert(ROOT_CA);
  client.setHandshakeTimeout(15);

  HTTPClient http;
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);

  if (!http.begin(client, url)) {
    Serial.println("HTTPS begin failed");
    return false;
  }

  int status = http.GET();
  int length = http.getSize();

  if (status != HTTP_CODE_OK || length <= 0 || (size_t)length > maxBytes) {
    Serial.printf("Text download rejected: HTTP=%d size=%d\n",
                  status, length);
    http.end();
    return false;
  }

  output = http.getString();
  http.end();

  if ((size_t)output.length() != (size_t)length) {
    Serial.println("Text response length mismatch");
    output = "";
    return false;
  }

  return true;
}

// Fetch a raw RSA-2048 signature (256 binary bytes).
bool fetchSignature(const char* url, uint8_t* signature,
                    size_t expectedLength) {
  WiFiClientSecure client;
  client.setCACert(ROOT_CA);
  client.setHandshakeTimeout(15);

  HTTPClient http;
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);

  if (!http.begin(client, url)) {
    Serial.println("Signature HTTPS begin failed");
    return false;
  }

  int status = http.GET();
  int length = http.getSize();

  if (status != HTTP_CODE_OK || length != (int)expectedLength) {
    Serial.printf("Signature response rejected: HTTP=%d size=%d\n",
                  status, length);
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  stream->setTimeout(15000);

  size_t received =
    stream->readBytes((char*)signature, expectedLength);

  http.end();

  if (received != expectedLength) {
    Serial.printf("Short signature: %u bytes\n",
                  (unsigned)received);
    return false;
  }

  return true;
}

// Verify RSA-2048 / SHA-256 / PKCS#1 v1.5 signature.
// Signature covers the EXACT bytes in version.txt.
bool verifyManifestSignature(const String& manifest,
                             const uint8_t* signature,
                             size_t signatureLength) {
  mbedtls_pk_context publicKey;
  mbedtls_pk_init(&publicKey);

  int rc = mbedtls_pk_parse_public_key(
    &publicKey,
    (const unsigned char*)OTA_PUBLIC_KEY,
    strlen(OTA_PUBLIC_KEY) + 1);

  if (rc != 0) {
    Serial.printf("Public key parse failed: -0x%04X\n",
                  (unsigned)(-rc));
    mbedtls_pk_free(&publicKey);
    return false;
  }

  // Calculate SHA-256 digest (32 bytes).
  uint8_t digest[32];

  rc = mbedtls_sha256(
    (const unsigned char*)manifest.c_str(),
    manifest.length(),
    digest,
    0  // SHA-256, not SHA-224
  );

  if (rc != 0) {
    Serial.printf("SHA-256 calculation failed: -0x%04X\n",
                  (unsigned)(-rc));
    mbedtls_pk_free(&publicKey);
    return false;
  }

  // Optional check the signature length
  if (signatureLength != 256) {
    Serial.println("Invalid signature length");
    mbedtls_pk_free(&publicKey);
    return false;
  }


  // Verify RSA-2048 / SHA-256 / PKCS#1 v1.5 signature.
  rc = mbedtls_pk_verify(
    &publicKey,
    MBEDTLS_MD_SHA256,
    digest,
    sizeof(digest),  // 32 bytes
    signature,
    signatureLength);

  mbedtls_pk_free(&publicKey);

  if (rc != 0) {
    Serial.printf("Manifest signature INVALID: -0x%04X\n",
                  (unsigned)(-rc));
    return false;
  }

  Serial.println("Manifest signature valid");
  return true;
}


// Strictly parse a version like 1.2.3.
bool parseVersion(const String& text,
                  unsigned long& major,
                  unsigned long& minor,
                  unsigned long& patch) {
  unsigned long a, b, c;
  char extra;

  if (sscanf(text.c_str(), "%lu.%lu.%lu%c",
             &a, &b, &c, &extra)
      != 3) {
    return false;
  }

  major = a;
  minor = b;
  patch = c;
  return true;
}

bool isNewerVersion(const String& candidate,
                    const char* current) {
  unsigned long a, b, c, x, y, z;

  if (!parseVersion(candidate, a, b, c) || !parseVersion(String(current), x, y, z)) {
    return false;
  }

  if (a != x) return a > x;
  if (b != y) return b > y;
  return c > z;
}

bool isHexSHA256(const String& hash) {
  if (hash.length() != 64) return false;

  for (size_t i = 0; i < hash.length(); i++) {
    char c = hash[i];
    bool valid =
      (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');

    if (!valid) return false;
  }

  return true;
}

// Expected manifest format:
// version\n
// 64-character SHA-256 hex digest\n
// firmware size in decimal bytes\n
//
// This parser intentionally requires LF line endings and a
// final newline, matching the exact file signed by OpenSSL.
#ifdef RELEASE_LOG
bool parseManifest(const String& manifest,
                   String& version,
                   String& hash,
                   size_t& firmwareSize,
                   String& releaseNotes) {
#else
bool parseManifest(const String& manifest,
                   String& version,
                   String& hash,
                   size_t& firmwareSize) {
#endif


  //  if (manifest.indexOf('\r') >= 0) return false;

  int p1 = manifest.indexOf('\n');
  int p2 = manifest.indexOf('\n', p1 + 1);

#ifdef RELEASE_LOG
  int p3 = manifest.indexOf('\n', p2 + 1);
#endif

  /**
  if (p1 <= 0 || p2 <= p1 || p3 <= p2 || p3 != (int)manifest.length() - 1) {
    return false;
  }
*/

#ifdef RELEASE_LOG
  if (p1 < 0 || p2 < 0 || p3 < 0) {
#else
  if (p1 < 0 || p2 < 0) {
#endif
    Serial.println("Malformed manifest: missing field separator");
    return false;
  }


  version = manifest.substring(0, p1);
  hash = manifest.substring(p1 + 1, p2);

#ifdef RELEASE_LOG
  String sizeText = manifest.substring(p2 + 1, p3);
  releaseNotes = manifest.substring(p3 + 1);
  releaseNotes.trim();
#else
  String sizeText = manifest.substring(p2 + 1);
#endif

  // remove any none print characters such as '\r'
  version.trim();
  hash.trim();
  sizeText.trim();

  if (!isHexSHA256(hash)) return false;

  if (sizeText.length() == 0 || sizeText.length() > 10)
    return false;

  for (size_t i = 0; i < sizeText.length(); i++) {
    if (sizeText[i] < '0' || sizeText[i] > '9')
      return false;
  }

  char* endPtr = nullptr;
  unsigned long parsedSize =
    strtoul(sizeText.c_str(), &endPtr, 10);

  if (endPtr == sizeText.c_str() || *endPtr != '\0')
    return false;

  if (parsedSize == 0 || parsedSize > MAX_FIRMWARE_SIZE) {
    return false;
  }

  firmwareSize = (size_t)parsedSize;
  return true;
}

bool downloadAndInstall(const String& expectedHash, size_t expectedSize) {


  // 5. Download firmware with TLS validation and no redirects.
  WiFiClientSecure client;
  client.setCACert(ROOT_CA);
  client.setHandshakeTimeout(15);

  HTTPClient http;
  http.setTimeout(30000);
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);

  if (!http.begin(client, FIRMWARE_URL)) {
    Serial.println("Firmware HTTPS begin failed");
    return false;
  }

  int status = http.GET();
  int contentLength = http.getSize();

  if (status != HTTP_CODE_OK || contentLength < 0 || (size_t)contentLength != expectedSize) {
    Serial.printf("Firmware rejected: HTTP=%d size=%d\n",
                  status, contentLength);
    http.end();
    return false;
  }

  // 6. Prepare the inactive OTA partition.
  if (!Update.begin(expectedSize, U_FLASH)) {
    Serial.printf("Update.begin failed: %s\n",
                  Update.errorString());
    http.end();
    return false;
  }

  // The Update library verifies SHA-256 at Update.end().
  // Must be set after begin() and before writing any data.
  if (!Update.setSHA256(expectedHash.c_str())) {
    Serial.println("Could not configure firmware SHA-256");
    Update.abort();
    http.end();
    return false;
  }

  Serial.println("Streaming firmware to OTA partition...");
  WiFiClient* stream = http.getStreamPtr();

  size_t written = Update.writeStream(*stream);
  http.end();

  if (written != expectedSize) {
    Serial.printf("Incomplete firmware: %u of %u bytes\n",
                  (unsigned)written,
                  (unsigned)expectedSize);
    Update.abort();
    return false;
  }

  // 7. Finalize only if all data arrived and SHA-256 matches.
  if (!Update.end()) {
    Serial.printf("Update.end failed: %s\n",
                  Update.errorString());
    Update.abort();
    return false;
  }

  Serial.println("Firmware hash verified; OTA finalized.");
  Serial.println("Restarting into the new firmware...");
  delay(500);
  ESP.restart();

  return true;  // Normally not reached.
}

// ============================================================
// CHECK FOR A NEWER RELEASE
// ============================================================

void checkForFirmwareUpdate() {
  if (otaInProgress) return;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("OTA check skipped: Wi-Fi disconnected");
    return;
  }

  otaInProgress = true;

  Serial.println("\nStarting authenticated OTA");

  // 1. Download the manifest.
  Serial.println("Fetching Manifesto..");

  String manifest;
  if (!fetchText(MANIFEST_URL, manifest, MAX_MANIFEST_SIZE)) {
    Serial.println("Manifest download failed");
    otaInProgress = false;
    return;
  }

  // 2. Download its raw RSA-2048 signature.
  Serial.println("Fetching RSA-2048 signature..");

  uint8_t signature[RSA_SIGNATURE_SIZE];

  if (!fetchSignature(SIGNATURE_URL, signature,
                      sizeof(signature))) {
    Serial.println("Signature download failed..");
    otaInProgress = false;
    return;
  }

  // 3. Authenticate the manifest BEFORE trusting its fields.
  Serial.println("\nAuthenticate the manifest");

  if (!verifyManifestSignature(
        manifest, signature, sizeof(signature))) {
    Serial.println("OTA rejected: invalid signature");
    otaInProgress = false;
    return;
  }

  // 4. Parse and validate signed metadata.
  Serial.println("Validate signed metadata..");

#ifdef RELEASE_LOG
  String releaseNotes;
#endif

  String version, expectedHash;
  size_t expectedSize = 0;

#ifdef RELEASE_LOG
  if (!parseManifest(manifest, version,
                     expectedHash, expectedSize, releaseNotes)) {
#else
  if (!parseManifest(manifest, version,
                     expectedHash, expectedSize)) {
#endif
    Serial.println("OTA rejected: malformed manifest");
    otaInProgress = false;
    return;
  }

  Serial.printf("Release version: %s\n", version.c_str());
  Serial.printf("Expected size: %u bytes\n",
                (unsigned)expectedSize);

  // Basic version policy: accept only a strictly newer version.
  // This is not hardware-backed anti-rollback.
  if (!isNewerVersion(version, FIRMWARE_VERSION)) {
    Serial.println("OTA rejected: version is not newer");
    otaInProgress = false;
    return;
  }

  Serial.printf("Download new version: %s\n", version.c_str());
  Serial.printf("Release Note:\n%s\n", releaseNotes.c_str());

  downloadAndInstall(expectedHash, expectedSize);

  // Normally a successful update reboots before reaching here.
  otaInProgress = false;
}


#else  // #ifdef SECURE_SIGN

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

#ifdef RELEASE_LOG

bool fetchManifest(String& remoteVersion, String& expectedSha256, String& releaseLog) {
#else

bool fetchManifest(String& remoteVersion, String& expectedSha256) {
#endif

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

#ifdef RELEASE_LOG
  int newline1 = body.indexOf('\n', newline + 1);  // find end of second line
  if (newline1 < 0) {
    Serial.println("Manifest format error: missing release log");
    return false;
  }
  releaseLog = body.substring(newline1 + 1);
  releaseLog.trim();

  expectedSha256 = body.substring(newline + 1, newline1);

#else

  expectedSha256 = body.substring(newline + 1);
#endif

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
  Serial.printf("Firmware HTTP result: %d\n", code);

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
  String releaseLog;

#ifdef RELEASE_LOG
  if (!fetchManifest(remoteVersion, expectedSha256, releaseLog)) {
#else
  if (!fetchManifest(remoteVersion, expectedSha256)) {
#endif
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

#ifdef RELEASE_LOG
  Serial.printf("Release log:\n%s\n", releaseLog.c_str());
#endif

  downloadAndInstall(expectedSha256, );

  // Normally a successful update reboots before reaching here.
  otaInProgress = false;
}

#endif  // #ifdef SECURE_SIGN

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
#ifdef SECURE_SIGN
    Serial.printf("****Github Sign OTA: Github release****\n");
#else
    Serial.printf("****Github Auto OTA: Github release****\n");
#endif
    Serial.printf("Firmware %s, Wi-Fi status %d\n", FIRMWARE_VERSION, (int)WiFi.status());
    Serial.printf("Temperature %.1f deg\n", (float)random(200, 400) / 10.0f);
  }

  // Your normal application tasks can run here.
  delay(10);
}