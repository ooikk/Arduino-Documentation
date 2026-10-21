#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

const char* WIFI_SSID = "Nightingale_IoT";
const char* WIFI_PASS = "1122334455";
const char* TEST_URL  = "https://example.com/";
//const char* TEST_URL  = "https://expired.badssl.com/";

// Replace with the real, verified root CA PEM.
// Keep this buffer alive for the entire connection.
// SSL.com TLS ECC Root CA 2022.crt
static const char rootCA[] PROGMEM = R"PEM(
-----BEGIN CERTIFICATE-----
MIICOjCCAcCgAwIBAgIQFAP1q/s3ixdAW+JDsqXRxDAKBggqhkjOPQQDAzBOMQsw
CQYDVQQGEwJVUzEYMBYGA1UECgwPU1NMIENvcnBvcmF0aW9uMSUwIwYDVQQDDBxT
U0wuY29tIFRMUyBFQ0MgUm9vdCBDQSAyMDIyMB4XDTIyMDgyNTE2MzM0OFoXDTQ2
MDgxOTE2MzM0N1owTjELMAkGA1UEBhMCVVMxGDAWBgNVBAoMD1NTTCBDb3Jwb3Jh
dGlvbjElMCMGA1UEAwwcU1NMLmNvbSBUTFMgRUNDIFJvb3QgQ0EgMjAyMjB2MBAG
ByqGSM49AgEGBSuBBAAiA2IABEUpNXP6wrgjzhR9qLFNoFs27iosU8NgCTWyJGYm
acCzldZdkkAZDsalE3D07xJRKF3nzL35PIXBz5SQySvOkkJYWWf9lCcQZIxPBLFN
SeR7T5v15wj4A4j3p8OSSxlUgaNjMGEwDwYDVR0TAQH/BAUwAwEB/zAfBgNVHSME
GDAWgBSJjy+j6CugFFR781a4Jl9nOAuc0DAdBgNVHQ4EFgQUiY8vo+groBRUe/NW
uCZfZzgLnNAwDgYDVR0PAQH/BAQDAgGGMAoGCCqGSM49BAMDA2gAMGUCMFXjIlbp
15IkWE8elDIPDAI2wv2sdDJO4fscgIijzPvX6yv/N33w7deedWo1dlJF4AIxAMeN
b0Igj762TVntd00pxCAgRWSGOlDGxK0tk/UYfXLtqc/ErFc2KAhl3zx5Zn6g6g==
-----END CERTIFICATE-----
)PEM";

WiFiClientSecure client;

bool syncClock() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Synchronizing system time...");
  unsigned long start = millis();

  while (time(nullptr) < 1700000000 &&
         millis() - start < 20000) {
    Serial.print(".");
    delay(500);
  }

  bool validTime = time(nullptr) >= 1700000000;
  Serial.println(validTime ? "\nClock synchronized."
                           : "\nClock synchronization failed.");
  return validTime;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.setDebugOutput(true);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to Wi-Fi...");
  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < 20000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWi-Fi connection failed.");
    return;
  }

  Serial.println("\nWi-Fi connected.");

  if (!syncClock()) {
    Serial.println("FAIL CLOSED: refusing TLS without valid time.");
    return;
  }

  // Authenticate the server using the trusted CA.
  client.setCACert(rootCA);
  client.setHandshakeTimeout(15);

  HTTPClient https;
  https.setTimeout(15000);

  Serial.printf("Validated HTTPS GET: %s\n", TEST_URL);

  if (!https.begin(client, TEST_URL)) {
    Serial.println("HTTPClient begin failed.");
    return;
  }

  int code = https.GET();

  if (code > 0) {
    Serial.printf("HTTP status: %d\n", code);
    Serial.println("TLS connection succeeded with CA validation.");
  } else {
    Serial.printf("HTTPS failed: %s (%d)\n",
                  HTTPClient::errorToString(code).c_str(),
                  code);
    Serial.println("Inspect verbose TLS logs for the cause.");
  }

  https.end();
  client.stop();
}

void loop() {}