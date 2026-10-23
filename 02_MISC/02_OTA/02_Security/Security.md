# Module 1 — ESP32-S3 Security Engineering


**Target:** ESP32-S3 DevKitC-1 · N16R8 · Arduino IDE 2.x · Espressif Arduino-ESP32 Core 3.x

We'll approach this as a security engineering course, not a collection of Arduino examples. The objective is to build a device that remains trustworthy when an attacker can access its network, copy its flash contents, replace firmware files on a server, or physically handle the device.

One important distinction throughout this course: a security feature being available in the ESP32-S3 silicon does not mean it is enabled, configured correctly, or protected by your current Arduino firmware.

## 1.1 — The ESP32-S3 security architecture

<img width="75%" height="auto" alt="image" src="https://github.com/user-attachments/assets/390242f2-9da5-42b0-aba1-03cbc2167c67" />


[voltaat.com](https://www.voltaat.com/products/esp32-s3-devkitc-1-n16r8?utm_source=chatgpt.com)

Your N16R8 configuration combines three different security domains:

### 1. Boot and firmware trust

**Secure Boot V2 · RSA signature verification · eFuse trust configuration**

Establishes whether the executable firmware was signed by a trusted publisher. This is the foundation for resisting unauthorized firmware replacement.

### 2. Data confidentiality

**Flash Encryption · AES-XTS · NVS Encryption**

Protects stored firmware and selected data against offline inspection. It does not automatically encrypt every application file or protect secrets while your running application uses them.

### 3. Communications and update trust

**TLS · certificate validation · signed manifests · OTA policy**

Protects communications with servers and verifies that firmware updates are authorized. TLS and firmware signatures solve related but distinct problems.

These layers must work together. For example, TLS may protect a firmware download in transit, but if an attacker can replace the firmware and its accompanying unsigned hash on the server, TLS alone does not establish that the firmware publisher authorized that image.

## 1.2 — ESP32-S3 versus the classic ESP32

The classic ESP32 and ESP32-S3 share many security concepts, but their hardware and supported features are not identical.

| Feature | Classic ESP32 | ESP32-S3 |
|---|---|---|
| **Secure Boot** | Original Secure Boot V1 and supported configurations | Secure Boot V2 |
| **Hardware cryptography** | AES, SHA and RSA-related acceleration, depending on operation | AES, SHA, RSA and additional cryptographic hardware support |
| **Flash encryption** | Supported; implementation and configuration depend on chip revision | Supported with AES-XTS-based flash encryption |
| **eFuse security configuration** | Yes | Yes, with S3-specific fields and key-purpose controls |
| **HMAC / digital-signature capabilities** | Chip-specific; do not assume S3 parity | S3 has HMAC and Digital Signature peripherals |
| **External PSRAM** | Depends on module and design | Your N16R8 uses 8 MB Octal PSRAM |

The ESP32-S3 also has a hardware Key Manager associated with cryptographic key derivation and supported key-management functions. The exact available operations depend on the hardware peripheral, eFuse configuration, and software support.

> **Engineering rule:** use the ESP32-S3 technical reference manual, datasheet, and ESP-IDF security documentation for S3-specific behavior. Do not copy classic ESP32 eFuse commands, security assumptions, or provisioning instructions without checking their applicability.

## 1.3 — Inside the ESP32-S3 security hardware

<img width="977" height="966" alt="image" src="https://github.com/user-attachments/assets/d284f612-d299-4fd6-91e3-866f013d17d9" />


[embeddedcomputing.com](https://embeddedcomputing.com/technology/ai-machine-learning/espressif-providing-support-for-the-aiot-market-with-its-dual-core-esp32-s3?utm_source=chatgpt.com)

Think of the security hardware as several independent mechanisms rather than one master security switch.

### eFuse controller — hardware-rooted configuration

Contains one-time-programmable fields and key blocks. These control security settings, key purposes, read/write protection, boot policy and other irreversible decisions.

### Secure Boot V2 — firmware authenticity

The ESP32-S3 ROM bootloader and second-stage bootloader participate in a chain of trust. Secure Boot V2 uses RSA-3072 with RSA-PSS signatures; the trusted public-key digest is anchored in eFuse.

— ESP-IDF Programming Guide latest documentation
- [Secure Boot v2](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/security/secure-boot-v2.html)


### AES / XTS — flash and external-memory encryption

Hardware transparently encrypts and decrypts supported memory accesses. Flash Encryption protects off-chip flash contents; S3 also supports external SPI RAM encryption through the memory cache.

— ESP-IDF Programming Guide v6.1 documentation
- [Flash Encryption](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/security/flash-encryption.html)
- [Chapter 5 eFuse Controller](https://documentation.espressif.com/esp32-s3_technical_reference_manual_en.pdf)


### SHA and RSA accelerators — cryptographic operations

Accelerators help perform hashing and public-key operations. Hardware acceleration improves performance; it does not establish trust unless the correct keys and verification policies are used.

### HMAC and Digital Signature peripheral

These provide hardware-assisted cryptographic operations for supported key configurations. They can be useful for device authentication and signing operations without exposing protected key material to ordinary application code. The exact scheme depends on the selected peripheral mode and eFuse key purpose.

### Key Manager

Supports hardware-managed key derivation and related key operations on the S3. It is not a general-purpose vault into which Arduino applications can freely import arbitrary keys; its supported functions and configuration must be checked against the S3 documentation.

**Reference:** [ESP32-S3 Technical Reference Manual](https://documentation.espressif.com/esp32-s3_technical_reference_manual_en.pdf)

### What does this mean for your firmware?

For example, consider a device that downloads firmware over HTTPS.

- `mbedtls_sha256()` can calculate the image hash.
- `WiFiClientSecure::setCACert()` configures TLS server-certificate validation.
- A public-key signature check can establish whether an authorized publisher signed the update.
- Secure Boot V2 checks the authenticity of firmware at boot.
- Flash Encryption makes offline extraction of stored firmware and supported encrypted data more difficult.

These checks protect different stages of the lifecycle. Passing one check does not automatically satisfy the others.

## 1.4 — N16R8: Flash, PSRAM and partition planning

Your board has 16 MB of flash and 8 MB of Octal PSRAM. Those are different resources with different security implications.

| Resource | Capacity | Typical Use |
|---|---:|---|
| **16 MB Flash** | 16,777,216 bytes | Bootloader · partition table · application slots · NVS · filesystem · other data |
| **8 MB Octal PSRAM** | 8,388,608 bytes | Runtime buffers · large objects · image data · other supported external-RAM allocations |

### Planning dual-slot OTA

For reliable OTA, the partition table must provide space for two application slots and the other partitions your project needs.

| Partition | Purpose |
|---|---|
| `nvs` | Configuration and preferences |
| `otadata` | OTA slot-selection metadata |
| `app0` | One application image |
| `app1` | Alternate application image |
| `spiffs` or another data partition | Optional filesystem |

The names and exact layout depend on your partition CSV. For a 16 MB device, do not assume that selecting a generic “OTA” scheme creates two sufficiently large application slots. Check the actual partition table, binary size, filesystem requirements and bootloader configuration.

Flash Encryption also has partition-specific behavior: app partitions and certain system partitions are encrypted by default, while other data partitions may require the `encrypted` flag. NVS Encryption is a separate consideration for protecting stored configuration.

— ESP-IDF Programming Guide v6.1 documentation

### An important PSRAM nuance

PSRAM is volatile; Flash Encryption is not a substitute for sound runtime secret handling. Even where hardware transparently encrypts supported external-RAM accesses, your application must still consider when credentials are copied into RAM, whether buffers are cleared, and whether other code can access them.

Also, the ESP32-S3's flash-encryption documentation uses the terminology **XTS-AES-128** for an XTS configuration using a 256-bit combined key. Do not confuse the name of the XTS configuration with the total key material stored in eFuse.

— ESP-IDF Programming Guide v6.1 documentation

## 1.5 — What Arduino IDE does and does not do

| Task | Arduino IDE / Arduino core | Additional tooling or configuration |
|---|---|---|
| Compile and upload ordinary firmware | Yes | Usually none |
| Use TLS and mbedTLS APIs | Yes | Correct certificate and library configuration |
| Select a partition scheme | Often via Tools menu | Custom CSV and build integration when needed |
| Calculate SHA-256 or verify application-level signatures | Yes, with supported libraries | Key management and signature-generation workflow |
| Configure Secure Boot V2 correctly for production | Not a simple IDE checkbox | ESP-IDF build/configuration workflow and `espsecure.py` / `espefuse.py` as appropriate |
| Provision Flash Encryption keys and eFuses | Not safely handled by ordinary Upload alone | `espefuse.py` and a planned provisioning procedure |
| Configure encrypted NVS | Not automatically guaranteed by using Preferences | NVS encryption configuration and compatible initialization |
| Verify production security state | Serial logs alone are insufficient | Inspect eFuses, partition layout, signed images and flash behavior |

The Arduino framework can run on a security-configured ESP32-S3. The difficulty is integrating the correct bootloader, partition table, image-signing process, eFuse provisioning and update workflow. An ordinary Arduino upload should never be treated as proof that these protections are enabled.

## Module 1 knowledge check

Before we move into integrity, authenticity and TLS, consider these three questions.

1. An attacker reads the contents of your ESP32-S3 flash. Which feature primarily protects the confidentiality of stored firmware?
   - SHA-256
   - Flash Encryption
   - TLS certificate validation

2. Which feature is responsible for verifying firmware authenticity during boot?
   - Secure Boot V2
   - Preferences
   - PSRAM encryption alone

3. Why is an Arduino IDE upload of a working sketch not proof that Secure Boot is enabled?
   - Arduino sketches cannot use hardware cryptography
   - Secure Boot requires the correct bootloader, signed images and hardware trust configuration
   - The N16R8 does not support eFuse

**Module 1 answers**
- Q1: Flash Encryption: It protects the confidentiality of supported data stored in flash against offline inspection.
- Q2: Secure Boot V2: It verifies firmware authenticity during boot using a trusted signature-verification chain.
- Q3: Arduino upload: The complete boot and signing chain must be configured. A successful upload does not prove that Secure Boot, flash encryption, or eFuse protections are configured correctly. Secure Boot requires the correct bootloader, signed images and hardware trust configuration

# Module 2 — Integrity vs authenticity vs confidentiality

## 2.1 The three security properties

These terms are related, but they solve different problems.

### Integrity

**Has the data been changed?**

- **Example:** Calculate the firmware's SHA-256 hash and compare it with a trusted expected hash.
- **ESP32-S3:** SHA hardware support or the mbedTLS SHA-256 implementation.

### Authenticity

**Who created or authorized the data?**

- **Example:** Verify a firmware signature using a trusted publisher's public key before accepting an update.
- **ESP32-S3:** Secure Boot V2 for boot-time firmware authentication; mbedTLS public-key verification for application-level update authorization.

### Confidentiality

**Can an unauthorized party read the data?**

- **Example:** Protect stored Wi-Fi credentials from offline flash extraction.
- **ESP32-S3:** Flash Encryption, appropriately configured NVS Encryption, and TLS for data in transit.

These properties are independent. Encrypted firmware can still be unauthorized. Authentic firmware can contain a security vulnerability. A file can have a correct hash while being publicly readable.

## 2.2 Why SHA-256 alone cannot authenticate firmware

Imagine your OTA server hosts two files:

```json
// manifest.json
{
  "version": "2.0.0",
  "firmware": "firmware.bin",
  "sha256": "EXPECTED_FIRMWARE_HASH"
}
```

```text
// firmware.bin
```

Your ESP32 downloads both files and checks whether the calculated hash of the binary matches the manifest.

That verifies consistency between the two downloaded files. But what happens if an attacker can replace both files?

```text
Original publisher
Firmware A → SHA-256(A) → manifest A

Attacker replaces both files
Firmware B → SHA-256(B) → manifest B

Device accepts the wrong firmware
The hash comparison passes because the attacker supplied both the replacement firmware and its matching hash.
```

> **Critical principle:** SHA-256 is a cryptographic hash, not a digital signature. It does not identify the publisher or prove authorization.

There is one important qualification: a hash can support authentication when the expected hash comes from an independently trusted source. The weakness is trusting an unauthenticated hash delivered alongside the file it is supposed to validate.

## 2.3 — The fix: asymmetric digital signatures

With digital signatures, the publisher holds a private signing key, while the ESP32 holds the corresponding trusted public key.

### Publisher — private key

1. Build the firmware.
2. Calculate its SHA-256 hash.
3. Sign the hash using the publisher's private key.
4. Publish the firmware and signature.

### ESP32-S3 — trusted public key

1. Download the firmware and signature.
2. Calculate the firmware's SHA-256 hash locally.
3. Verify the signature using the trusted public key.
4. Reject the update if verification fails.
5. Only then proceed with the update process.

For our later OTA lab, we will use an asymmetric signature scheme supported by mbedTLS. The publisher's private key must remain off the ESP32, preferably in a protected signing environment. The public key embedded in the device is not secret, but its integrity is critical.

### Does a signature provide non-repudiation?

Not automatically. A valid signature establishes that the data matches a signature generated by the corresponding private key, subject to the security of the algorithm and implementation. Strong non-repudiation also depends on key ownership, access controls, identity assurance and audit evidence.

For an embedded OTA system, our immediate objectives are integrity and publisher authenticity, rather than legal non-repudiation.

## 2.4 — Threat modelling with STRIDE

Before writing security code, identify what an attacker might do and which control addresses it.

| STRIDE threat | ESP32-S3 example | Relevant mitigation |
|---|---|---|
| **S — Spoofing** | Fake OTA server impersonates the publisher | TLS certificate validation; signed firmware |
| **T — Tampering** | Firmware binary or configuration is modified | Signature verification; Secure Boot; suitable data protection |
| **R — Repudiation** | A party denies authorizing an update | Protected signing keys and auditable signing records |
| **I — Information disclosure** | Attacker extracts Wi-Fi credentials from flash | Flash Encryption, NVS Encryption, careful secret handling |
| **D — Denial of service** | Attacker repeatedly disrupts downloads or exhausts resources | Timeouts, bounded buffers, retry limits, robust recovery |
| **E — Elevation of privilege** | Malicious firmware gains control of the device | Secure Boot, least privilege, validated inputs, secure update policy |

No single mechanism eliminates every threat. For example, Secure Boot can prevent execution of unauthorized firmware, but it does not prevent an attacker from jamming Wi-Fi or disconnecting the power supply.

## 2.5 — Apply the model to your OTA pipeline

Your current system already uses HTTPS, certificate validation, SHA-256 and `Update.h`. We will build on that rather than replace working components unnecessarily.

### Transport trust

`WiFiClientSecure::setCACert()` validates the server's certificate chain against a configured trust anchor, subject to hostname and certificate-validation behavior in the library.

### Publisher trust

An independently trusted public key verifies the signed manifest or firmware. A compromised web server should not be able to authorize its own malicious update.

### Installation integrity

Validate the image and metadata before committing to installation; handle interrupted downloads, boot validation and recovery deliberately.

### Boot-time trust

Secure Boot V2 independently verifies firmware at boot. Application-level signature verification and boot-time verification are complementary safeguards.

One subtle but important implementation point: calling `Update.write()` is not the same as making the new firmware trusted. The update workflow should validate the publisher's signature before writing the image, check download length and hash, and only finalize the update after all applicable checks pass. Secure Boot then provides another verification layer at boot.

## Module 2 quiz

1. An attacker replaces both the firmware binary and the SHA-256 hash in an unsigned manifest. The hashes match. What can you conclude?
   - The firmware is authentic because SHA-256 matches
   - The binary matches the supplied hash, but publisher authenticity is unproven
   - TLS would always detect the replacement

2. Which key should the ESP32 use to verify a publisher's digital signature?
   - The publisher's private signing key
   - A public key obtained from the same untrusted manifest without validation
   - A trusted publisher public key provisioned or authenticated independently

3. Your device verifies the publisher's signature correctly, but an attacker can still interrupt every OTA download. Which STRIDE category best describes this attack?
   - Denial of service
   - Information disclosure
   - Repudiation

### Results

- **Q1 = B:** Matching an attacker-supplied hash establishes consistency, not publisher authenticity.
- **Q2 = C:** The ESP32 must trust the publisher's public key independently of the untrusted update package.
- **Q3 = A:** Interrupting downloads is a denial-of-service attack.


# Module 3 — TLS security on ESP32-S3

**Arduino-ESP32 Core 3.x · Arduino IDE 2.x · WiFiClientSecure · HTTPClient**

## 3.1 What TLS actually protects

When your ESP32-S3 connects to an HTTPS server, TLS can provide three important protections:

- **Confidentiality:** encrypts traffic between the device and server.
- **Integrity:** detects unauthorized modification of protected traffic.
- **Server authentication:** helps the device establish that it is communicating with a server presenting a certificate valid under its trust policy.

However, HTTPS does not automatically prove that a downloaded firmware image was authorized by your firmware publisher. That is why we will add signed-manifest verification in Module 7.

## 3.2 Compare the three connection modes

### Mode A — `client.setInsecure()`

**Not suitable for production trust**

Encrypts the connection but disables normal server-certificate authentication. An attacker who can intercept the connection may impersonate the server and establish a separate encrypted connection to the device.

### Mode B — `client.setCACert(rootCA)`

**Recommended baseline**

Configures a trusted CA certificate. The TLS implementation validates the server certificate chain against the configured trust anchor and performs applicable validity and hostname checks. This is the appropriate starting point for HTTPS OTA.

### Mode C — Mutual TLS (mTLS)

**Device and server authentication**

The device validates the server, while the server also requests and validates a client certificate. The device proves possession of the corresponding private key during the TLS handshake.

One practical distinction: a client certificate does not replace the server's CA trust configuration. In mTLS, the client normally needs both the server trust anchor and its own client certificate/private key.

### What certificate validation checks

A valid HTTPS connection requires more than finding a certificate with a familiar name.

| Check | What it establishes | Typical failure |
|---|---|---|
| **Certificate chain** | The server certificate chains to a trusted CA | Unknown or untrusted issuer |
| **Validity period** | Certificates are within their Not Before and Not After dates | Expired or not-yet-valid certificate |
| **Hostname** | Certificate identity matches the requested hostname | Connecting to `example.com` but certificate identifies another host |
| **Trust anchor** | The chain terminates at a trusted root | Wrong CA configured |
| **TLS handshake** | Protocol negotiation and cryptographic checks succeed | Incompatible settings, peer rejection or network failure |

ESP-IDF's TLS implementation supports CA-based verification and hostname validation. SNI also helps a server select the correct certificate when hosting multiple domains.

> **Time matters.** The ESP32-S3 does not have a battery-backed real-time clock that automatically preserves accurate time across power loss. Synchronize its clock before validating certificate dates, and do not silently bypass certificate validation if NTP fails.

## 3.3 Where should the root CA live?

For a small Arduino project, a static PEM string is the simplest starting point. It can be stored in flash rather than copied into a temporary RAM buffer.

```cpp
#include <WiFiClientSecure.h>

// Replace this with the actual, verified root CA PEM.
// The complete certificate must remain available for the
// lifetime of the WiFiClientSecure connection.
static const char rootCA[] PROGMEM = R"PEM(
-----BEGIN CERTIFICATE-----
PASTE_VERIFIED_ROOT_CA_PEM_HERE
-----END CERTIFICATE-----
)PEM";

WiFiClientSecure client;

void configureTLS() {
  client.setCACert(rootCA);
}
```

The certificate text above is a placeholder, not a usable certificate.

For a production device:

- **Embedded root CA:** simple and protected against ordinary remote modification, but changing the trust anchor requires a firmware update.
- **LittleFS:** useful when you need to update trust material, but a writable filesystem is not inherently trustworthy. Authenticate updates and protect against rollback.
- **Certificate bundle:** useful when connecting to many public services; it increases flash usage but reduces the need to manage individual roots.
- **Private client key:** treat it as a secret. A public root CA is not secret, but a client private key must be protected against extraction and unauthorized use.

Also, `PROGMEM` is not a security boundary. It does not by itself encrypt the certificate or prevent an attacker from reading flash.

## 3.4 — Mutual TLS in Arduino-ESP32 Core 3.x

One API detail matters for your target: the official Arduino-ESP32 `WiFiClientSecure` interface uses `setCertificate()` and `setPrivateKey()` for the client identity, rather than relying on a `setClientCert()` method.

- [NetworkClientSecure](https://github.com/espressif/arduino-esp32/blob/master/libraries/NetworkClientSecure/README.md)
- [NetworkClientSecure.h](https://github.com/espressif/arduino-esp32/blob/master/libraries/NetworkClientSecure/src/NetworkClientSecure.h)


```cpp
#include <WiFiClientSecure.h>

WiFiClientSecure client;

extern const char serverRootCA[];
extern const char deviceCertificate[];
extern const char devicePrivateKey[];

void configureMutualTLS() {
  // Authenticate the server
  client.setCACert(serverRootCA);

  // Authenticate this device to the server
  client.setCertificate(deviceCertificate);
  client.setPrivateKey(devicePrivateKey);
}
```

This configures the client side of mTLS. It does not make an arbitrary server enforce client authentication: the server must request and validate the device certificate, and normally associate it with an authorized device identity.

For production, don't embed a long-lived device private key in an ordinary firmware image if physical extraction is in your threat model. We'll examine better credential protection in Module 4.

# Lab 3 — Valid vs insecure TLS

**Hardware:** ESP32-S3 N16R8 · **Serial Monitor:** 115200 baud

We will run two separate sketches against [https://example.com/](https://example.com/).

- **Lab A:** use `setInsecure()` to demonstrate that the request can succeed without authenticating the server.
- **Lab B:** use `setCACert()` to validate the server's certificate chain.
- **Negative tests:** configure the wrong CA, then test an expired certificate with an appropriate trust anchor.

## Preparation

1. In Arduino IDE, select your ESP32-S3 N16R8 board and its working USB serial port.
2. Replace the Wi-Fi placeholders in each sketch with your test network credentials.
3. In Lab B, obtain the correct root CA from the server's certificate chain using a trusted source. Verify that the chain terminates at that root. Do not paste the server's leaf certificate and call it a root CA.
4. In **Tools → Core Debug Level**, select **Verbose** if that option is available for your selected board package.
5. Open **Serial Monitor** at **115200 baud**.

The official Arduino-ESP32 examples also synchronize the clock before TLS validation, because certificate validity dates depend on correct system time.

**Documents about location CA**

[Locating and Exporting the Root CA Certificate](https://github.com/ooikk/Arduino-Documentation/blob/main/02_MISC/02_WiFi/MQTT.md#locating-and-exporting-the-root-ca-certificate)

[Obtaining the ThingsBoard Root CA Certificate](https://github.com/ooikk/Arduino-Documentation/blob/main/02_MISC/02_WiFi/Thingsboard.md#obtaining-the-thingsboard-root-ca-certificate)


## Lab A — Insecure TLS

This is a deliberately insecure diagnostic sketch. Use it only on your test device, and do not reuse this TLS configuration in your OTA code.

```cpp
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
const char* TEST_URL  = "https://example.com";

WiFiClientSecure client;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.setDebugOutput(true);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to Wi-Fi");
  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < 20000) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWi-Fi connection failed.");
    return;
  }

  Serial.printf("\nWi-Fi connected: %s\n",
                WiFi.localIP().toString().c_str());

  // LAB A ONLY: bypass server authentication.
  client.setInsecure();

  HTTPClient https;
  https.setTimeout(15000);

  Serial.printf("GET %s\n", TEST_URL);

  if (!https.begin(client, TEST_URL)) {
    Serial.println("HTTPClient begin failed.");
    return;
  }

  int code = https.GET();

  if (code > 0) {
    Serial.printf("HTTP status: %d\n", code);
    Serial.println("Request completed without CA validation.");
  } else {
    Serial.printf("Request failed: %s (%d)\n",
                  HTTPClient::errorToString(code).c_str(),
                  code);
  }

  https.end();
  client.stop();
}

void loop() {}
```

### Expected result

The request will typically succeed if the network and server are reachable. A `200` or another valid HTTP response shows that the server returned a response; it does not demonstrate that the server's identity was authenticated.

## Lab B — Validated TLS

Before compiling, replace the sample PEM with the complete, verified root CA certificate appropriate for `example.com`. The placeholder below deliberately cannot authenticate a real server.

```cpp
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
const char* TEST_URL  = "https://example.com/";

// Replace with the real, verified root CA PEM.
// Keep this buffer alive for the entire connection.
static const char rootCA[] PROGMEM = R"PEM(
-----BEGIN CERTIFICATE-----
REPLACE_WITH_COMPLETE_VERIFIED_ROOT_CA_PEM
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
```

The clock check here is intentionally basic: it checks that the time is plausible, not that NTP itself is authenticated. For a production device, consider how the device establishes trustworthy time and recovers when time synchronization is unavailable.

## 3.5 — Negative tests: prove that validation is working

A successful connection is only half the lab. We also need to demonstrate that invalid certificates are rejected.

### Test 1 — Correct root CA

**Expected:** pass

Use the actual trusted root CA for the server's certificate chain and a synchronized clock. The TLS handshake should succeed if the certificate, hostname and server configuration are valid.

### Test 2 — Wrong root CA

**Expected:** fail

Replace `rootCA` with a valid but unrelated root certificate. The handshake should fail because the server's chain does not terminate at that trust anchor.

### Test 3 — Expired server certificate

**Expected:** fail

Change the URL to [https://expired.badssl.com/](https://expired.badssl.com/) and configure the appropriate trusted root CA for its chain. The expired leaf certificate should be rejected. Confirm the current certificate dates and chain before running the test.

## Reading the Serial Monitor

With **Core Debug Level** set to **Verbose**, look for TLS messages indicating certificate verification errors or handshake failure. Exact messages and error codes depend on the Arduino-ESP32 core build and the failure.

Common diagnostic categories include:

- **Certificate verification failed:** wrong CA, expired certificate, not-yet-valid certificate or another chain-validation problem.
- **Hostname mismatch:** the certificate does not identify the requested host.
- **Connection or timeout failure:** potentially a network problem rather than a certificate failure.
- **HTTP status code:** the HTTP exchange progressed far enough to receive an HTTP response; it is not, by itself, proof of secure configuration.

For additional diagnostics, the official `NetworkClientSecure` implementation exposes `lastError()` on supported Core 3.x versions.

## Common pitfalls to avoid

- Don't pin a leaf certificate as though it were a root CA. Server certificates rotate and expire; choose the trust strategy deliberately.
- Don't treat `setInsecure()` as a workaround for a certificate error. Diagnose the trust chain, hostname, time and certificate validity instead.
- Don't assume HTTPS redirects are harmless. Check the destination host and trust policy if redirects are followed.
- Don't confuse a valid certificate with an authorized firmware publisher. Module 7 will add signed-manifest verification.
- Don't assume a verbose log proves a failed handshake was rejected for the correct reason. Verify the actual negative test and confirm that no HTTP response was received from the failed TLS connection.

# Module 4 — Secure Credential Storage on ESP32-S3

In this module, we’ll move from protecting data while it travels over the network (TLS) to protecting data stored inside the ESP32-S3.

Your ESP32-S3 N16R8 has 16 MB of flash and 8 MB of PSRAM. For security, the critical distinction is that PSRAM is working memory, while flash is persistent storage. Wi-Fi credentials, API tokens, device configuration and other persistent values are commonly stored in flash-backed NVS or filesystems.

## 4.1 What are we protecting?

### 1. Device credentials

<img width="750" height="750" alt="image" src="https://github.com/user-attachments/assets/72e1f41c-df7d-4aae-bb5d-d88ec14c1367" />

Wi-Fi passwords, cloud API tokens, MQTT credentials and device identity keys. If extracted, these may let an attacker impersonate your device or access your services.

### 2. Persistent storage

<img width="1024" height="993" alt="image" src="https://github.com/user-attachments/assets/585c11a9-ae60-4c3c-9e4a-251bc0f70408" />

NVS (Non-Volatile Storage), firmware partitions and filesystems such as LittleFS. Data may remain after power is removed.

### 3. Runtime memory

<img width="2048" height="1365" alt="image" src="https://github.com/user-attachments/assets/02b9ee9d-8f28-4a3f-ae50-a23da0b53c99" />


Passwords and keys can also exist in RAM while your program uses them. Encrypting flash does not automatically protect secrets from compromised firmware or memory disclosure.

## 4.2 The storage options

| Method | What it provides | Main limitation |
|---|---|---|
| **Hard-coded secret in firmware** | Convenient for testing | Can be extracted from firmware; difficult to rotate safely |
| **Ordinary NVS / Preferences** | Persistent key-value storage | Does not inherently encrypt stored values |
| **NVS encryption** | Encrypts protected NVS entries | Requires correct key provisioning and configuration |
| **Flash Encryption** | Encrypts flash contents using hardware-supported mechanisms | Does not replace access control, Secure Boot or safe key handling |
| **Secure element / external key service** | Can isolate certain secrets and operations | Extra hardware, integration and provisioning complexity |

> **Key idea:** Preferences is a storage API, not an encryption feature. Changing a hard-coded password to `prefs.putString()` does not, by itself, make the password confidential.

## 4.3 NVS encryption versus Flash Encryption

This is the most important technical distinction in Module 4.

### NVS encryption

Encrypts NVS data using an NVS-specific encryption scheme. The application can continue using the usual NVS read/write APIs after the encryption feature has been configured.

### Flash Encryption

Encrypts supported regions of the ESP32-S3's external flash. When the CPU reads mapped flash through the normal cache mechanism, the hardware transparently decrypts it. NVS data still needs its own encryption scheme.

— ESP-IDF Programming Guide v5.5.6 documentation
- [Flash Encryption](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/security/flash-encryption.html)

One nuance for your ESP32-S3: Espressif supports different NVS key-protection schemes. The traditional scheme protects NVS encryption keys using Flash Encryption; supported chips can also use an HMAC-based scheme. Therefore, don't assume that enabling Flash Encryption automatically means NVS encryption is correctly configured for every ESP-IDF version and build.

- [NVS Encryption](https://github.com/espressif/esp-idf/blob/master/docs/en/api-reference/storage/nvs_encryption.rst)
- [Kconfig](https://github.com/espressif/esp-idf/blob/master/components/nvs_flash/Kconfig)
- [ESP-IDF User Guide for ESP32-P4, SDK latest](https://documentation.espressif.com/projects/esp-idf/en/latest/esp32s3/migration-guides/release-5.x/5.2/storage.html?title=ESP-IDF+User+Guide+for+ESP32-P4,+SDK+latest)

### What happens if someone reads the flash chip?

| Configuration | Likely result of raw flash extraction |
|---|---|
| **Plain NVS, no Flash Encryption** | Stored NVS values may be recoverable |
| **Flash Encryption only** | Application code and selected flash regions are encrypted, but ordinary NVS values are not automatically protected by NVS encryption |
| **Flash Encryption + correctly configured NVS encryption** | Firmware and protected NVS data are encrypted at rest |

These are simplified outcomes; the actual result depends on the partition table, encryption configuration and how the attacker obtains the data.

## 4.4 Lab 4A — Store a credential using Preferences

First, we'll examine what the Arduino API does. This is a demonstration of storage, not secure credential storage.

The example stores a dummy token so you can test it without exposing a real password or API key.

```cpp
#include <Preferences.h>

Preferences prefs;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Open a namespace for read/write access.
  if (!prefs.begin("security", false)) {
    Serial.println("Failed to open Preferences");
    return;
  }

  // DEMO ONLY: this is not encrypted by Preferences.
  prefs.putString("api_token", "DEMO-TOKEN-12345");

  String token = prefs.getString("api_token", "");

  Serial.print("Stored token: ");
  Serial.println(token);

  prefs.end();
}

void loop() {
}
```

### What this lab demonstrates

- `prefs.begin("security", false)` opens the `security` namespace for read/write access.
- `putString()` writes the value to NVS.
- `getString()` retrieves it.
- The value persists across a restart, unlike a normal local variable.

> **Security observation:** the namespace name `security` and the key name `api_token` are labels, not access controls. The API doesn't ask for a password before reading the value, and ordinary Preferences use doesn't imply encryption.

**Do not substitute a real cloud token into this example.**

## 4.5 Lab 4B — Design a safer storage architecture

For a production IoT device, consider the following design:

```text
ESP32-S3 application
Uses credentials only when needed

Credential access layer
Centralizes reading, validation, rotation and error handling

Protected persistent storage
NVS encryption configured for the selected ESP-IDF/core build

Additional protection:
Flash Encryption + Secure Boot, with carefully managed provisioning and recovery.
```

### Practical rules

- **Separate configuration from secrets.** Brightness, thresholds and device names usually don't need confidentiality. Wi-Fi passwords and private keys do.
- **Don't print secrets.** Avoid `Serial.println(token)` in production, even for debugging.
- **Avoid unnecessary copies.** A secret retrieved into a `String` or buffer can remain in RAM; flash encryption doesn't protect it while the application is using it.
- **Plan credential rotation.** Support changing or revoking cloud credentials without needing to rebuild the entire firmware.
- **Protect the boot chain.** Storage encryption alone doesn't stop an attacker from running malicious firmware that reads secrets. Secure Boot helps prevent unauthorized firmware from booting.
- **Treat provisioning as part of security.** A secure storage mechanism is only as strong as its key provisioning, recovery and device-access controls.

Espressif cautions that Flash Encryption changes the firmware flashing and update workflow. We will study that separately before enabling any irreversible eFuse settings.

— ESP-IDF Programming Guide v5.5.6 documentation

## 4.6 Knowledge check

1. You store an API token using `Preferences.putString()`. What can you conclude?
   - The token is encrypted automatically
   - The token is persistent, but encryption is not guaranteed
   - Only the key name is stored

   **Correct.** Preferences is an NVS access API. Encryption depends on the device's NVS security configuration.

2. What is the key difference between Flash Encryption and NVS encryption?
   - They are exactly the same feature
   - Flash Encryption protects supported flash regions; NVS needs an appropriate NVS encryption scheme
   - NVS encryption protects only data in PSRAM

   **Correct.** Flash Encryption does not automatically encrypt ordinary NVS values in the way NVS encryption does.

3. If an attacker can run malicious firmware on the device, is encrypted NVS alone sufficient?
   - Yes, because the attacker can never read decrypted values
   - No; running firmware may access secrets through normal application APIs
   - Yes, provided the namespace is called `security`

   **Correct.** Encryption at rest does not by itself defend against compromised firmware that can access decrypted data.

# Module 5 — ESP32-S3 Secure Boot V2

In Module 4, we looked at protecting secrets stored in flash. Now we move to a different security problem:

> **How can your ESP32-S3 determine whether the firmware it is about to execute was authorized by you?**

This is the purpose of Secure Boot. For your ESP32-S3 N16R8, Secure Boot V2 establishes a hardware-rooted chain of trust, helping prevent unauthorized or modified firmware from running.

## 5.1 The threat: someone replaces your firmware

Imagine you deploy an ESP32-S3 device that controls a motor, monitors sensors and connects to your cloud server.

An attacker gets physical access to the device and attempts to install modified firmware that steals credentials or changes motor behavior.

<img width="1043" height="552" alt="image" src="https://github.com/user-attachments/assets/99af6f40-4082-47d0-ba7e-7fbe4b32a59f" />


| Scenario | Result |
|---|---|
| **Firmware image** | Your compiled application in flash |
| **Tampering attempt** | An unauthorized image is written to flash |
| **Secure Boot enabled** | Reject an image whose signature does not verify against the trusted key |
| **No Secure Boot** | The chip lacks this signature-based boot enforcement |

> **A crucial distinction:** Secure Boot is about authenticity and integrity of the boot chain, not keeping firmware confidential. Flash Encryption addresses confidentiality of supported flash contents.

## 5.2 How Secure Boot V2 works

The ESP32-S3 Secure Boot V2 mechanism uses RSA-3072 signatures with SHA-256 hashing and RSA-PSS signature verification. The chip uses a trusted public-key digest stored in eFuse to establish which signing key is authorized.

1. **Developer signs firmware** — The release process uses a private signing key kept secret.
2. **Signed image is installed** — USB flashing, OTA or another supported update mechanism.
3. **Boot ROM establishes trust** — The hardware-rooted trust anchor is checked against the signature's public-key digest.
4. **Image signature is verified** — A valid signature from an authorized key permits the boot chain to proceed.
5. **Authorized firmware may boot** — Invalid or unauthorized signed images are rejected.

The boot ROM is immutable code built into the chip. It establishes the first trust decision; subsequent boot stages must preserve the chain of trust.

> The private signing key should never be embedded in your ESP32 firmware. The device needs the trusted verification material, not the publisher's private key.

The verification sequence above follows Espressif's [Secure Boot V2 documentation for ESP32-S3](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/security/secure-boot-v2.html).

— ESP-IDF Programming Guide v6.1 documentation
- [Secure Boot v2](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/security/secure-boot-v2.html)

## 5.3 What exactly is signed?

The signature covers the firmware image, including the relevant image padding. A signature block contains the RSA public key, image digest and signature. The device checks both that the public key is authorized by the eFuse trust anchor and that the signature matches the image.

Consider three scenarios:

| Attack or change | Expected outcome |
|---|---|
| **Attacker modifies one byte of a signed firmware image** | Signature verification fails |
| **Attacker creates a different image and signs it with their own key** | Verification fails because that key is not trusted by the device |
| **You release a new image and sign it with your authorized private key** | Verification can succeed, provided the image and boot configuration are otherwise valid |

A signature is not simply a password attached to a file. It is a cryptographic proof that the image matches a signing operation performed with the corresponding private key.

## 5.4 Secure Boot is not the same as anti-rollback

Suppose firmware version 1.0 has a security vulnerability. You release version 1.1 to fix it, and both versions are signed with your authorized key.

Secure Boot can accept both versions because both have valid signatures. It does not, by itself, guarantee that version 1.0 is too old to run.

| Mechanism | Question it answers |
|---|---|
| **Secure Boot** | Is this firmware authorized and intact? |
| **Anti-rollback** | Is this firmware version permitted, or is it older than the minimum allowed security version? |
| **Flash Encryption** | Can an attacker read or manipulate protected flash contents easily by extracting raw flash? |

For a production device, these mechanisms complement one another. Secure Boot also does not prevent every denial-of-service attack or guarantee that authorized firmware is free of bugs.

— ESP-IDF Programming Guide v6.1 documentation

## 5.5 What this means for your Arduino IDE setup

Your ESP32-S3 currently uses Arduino IDE 2.3.10 and Arduino-ESP32 core 3.3.12. Your existing OTA uploads work, but successful OTA upload does not prove that Secure Boot is enabled.

The standard Arduino workflow does not automatically configure the complete Secure Boot V2 provisioning process. Enabling it involves a signed bootloader and application, key management, and eFuse configuration.

> **Important for your learning board:** do not burn Secure Boot eFuses yet. eFuse changes can be irreversible, and enabling security features can change how USB flashing and recovery work. Plan the signing, provisioning and recovery process before making changes.

— ESP-IDF Programming Guide v6.1 documentation
- [Secure Boot v2](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/security/secure-boot-v2.html)

For now, we can learn and test the signing concepts without changing your board's security configuration.

## 5.6 Knowledge check

1. Where does the ESP32-S3 keep the trust anchor used by Secure Boot V2?
   - In a public text file on LittleFS
   - As a digest of an authorized RSA public key in eFuse
   - In the Wi-Fi router

   **Correct.** The eFuse key digest anchors trust in the signing public key.

2. An attacker modifies one byte of a signed application image. What should happen?
   - The signature verification should fail
   - The ESP32 automatically repairs the byte
   - The image is accepted if its filename is unchanged

   **Correct.** Changing signed image content invalidates the signature.

3. Both firmware 1.0 and 1.1 have valid signatures from your authorized key. Which feature prevents an obsolete vulnerable version from being accepted?
   - TLS
   - NVS encryption
   - Anti-rollback enforcement

   **Correct.** A valid signature proves authorization, not that the version is sufficiently recent.

4. What should you do before enabling Secure Boot on your learning board?
   - Burn the eFuses first and work out recovery later
   - Plan signing, key storage, provisioning and recovery before changing eFuses
   - Assume selecting ESP32-S3 in Arduino IDE enables it

   **Correct.** Secure Boot requires a deliberate provisioning workflow; eFuse changes can be irreversible.

# Module 6 — ESP32-S3 Flash Encryption

In Module 5, we learned how Secure Boot V2 checks whether firmware is authorized to run. Now we’ll protect the confidentiality of data stored in your ESP32-S3 N16R8's flash.

The central question is:

> **If someone removes your ESP32-S3's flash chip or reads its contents using a programming tool, can they recover your firmware, configuration or stored data?**

Flash Encryption is designed to make that much harder by encrypting supported flash contents using a hardware-managed key.

## 6.1 Secure Boot versus Flash Encryption

### Secure Boot V2 — authenticity

Checks whether firmware was signed by an authorized publisher and has not been modified in a way that invalidates its signature.

### Flash Encryption — confidentiality at rest

Encrypts supported regions of external flash so that raw flash extraction does not ordinarily reveal the plaintext contents.

Neither feature replaces the other. An encrypted firmware image could still be unauthorized if boot authentication were not enforced. Conversely, a correctly signed image can remain readable if flash encryption is not enabled.

## 6.2 How encryption works on your ESP32-S3

Your N16R8 board has 16 MB of external flash and 8 MB of PSRAM. Flash Encryption applies to supported external flash regions, not to PSRAM as a general-purpose memory-protection mechanism.

```text
Application writes data
Firmware, encrypted-partition data or other supported flash contents

Flash Encryption hardware
Encrypts writes and decrypts reads through supported flash-access paths

External flash
Encrypted representation stored on the chip

Authorized runtime reads can access plaintext through the normal supported hardware path.
```

The ESP32-S3 uses AES-XTS-based Flash Encryption. The encryption key is protected through eFuse-backed hardware mechanisms; it is not meant to be stored as an ordinary application string. The exact key-generation and provisioning procedure depends on the selected security configuration.

— ESP-IDF Programming Guide v5.5.6 documentation
- [Flash Encryption](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32s3/security/flash-encryption.html)

## 6.3 Which flash partitions are encrypted?

Flash Encryption does not mean that every byte in every partition is automatically encrypted in the same way. The bootloader's configuration and partition table matter.

For ESP32-S3, the documented default encrypted regions include the second-stage bootloader, partition table, NVS key partition, OTA selection data (`otadata`) and application partitions. Other data partitions can be encrypted when marked with the `encrypted` flag in the partition table.

— ESP-IDF Programming Guide v5.5.6 documentation

| Partition or data | Typical treatment with Flash Encryption enabled |
|---|---|
| **Application firmware** | Encrypted |
| **Second-stage bootloader** | Encrypted |
| **Partition table** | Encrypted |
| **NVS encryption key partition** | Encrypted |
| **Ordinary NVS data partition** | Requires attention to the NVS encryption configuration |
| **LittleFS / custom data partition** | Depends on partition configuration and access method |
| **PSRAM contents** | Not protected by external-flash encryption |

This is why you should not assume that storing a password in a LittleFS file makes it confidential. The partition's configuration and how the data is written matter.

## 6.4 Development mode versus Release mode

Espressif provides two main Flash Encryption modes. This distinction is especially important for your Arduino OTA workflow.

### Development mode

**For controlled development**

- Allows more flexibility for flashing and testing.
- The ROM download path may encrypt plaintext images on the device.
- Provides weaker protection against physical reflashing attacks than Release mode.
- Is not the recommended configuration for production devices.

### Release mode

**For production provisioning**

- Restricts flash-encryption operations through the ROM download interface.
- Requires a planned process for future firmware updates and recovery.
- Commonly relies on OTA or correctly pre-encrypted images for later updates, depending on the locked-down configuration.
- Should be deployed only after testing the complete provisioning and recovery process.

> The word **development** can be misleading: it does not mean the device is fully protected from an attacker with physical access. Release mode adds restrictions, but the overall security depends on the other eFuse settings and boot protections too.

— ESP-IDF Programming Guide v5.3 documentation
- [Flash Encryption](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32s3/security/flash-encryption.html)

## 6.5 What happens to your OTA updates?

You previously got OTA working with `ArduinoOTA` on your ESP32-S3. Flash Encryption changes the requirements for firmware deployment, but it does not mean that OTA becomes impossible.

The important sequence is:

1. Your build system creates a firmware image.
2. The update mechanism transfers the image to the device.
3. The device writes the image to the appropriate OTA partition through a supported flash-writing path.
4. With Flash Encryption correctly configured, supported writes to encrypted partitions are stored encrypted.
5. On reboot, the bootloader reads the image through the hardware decryption path. If Secure Boot is also enabled, the boot chain verifies the firmware's signature.

Espressif documents automatic encryption of OTA writes through supported partition-writing APIs. However, do not assume that every Arduino upload tool, custom flashing script or third-party OTA library will handle every production configuration correctly. The exact workflow must be validated against the core, bootloader, partition table and eFuse configuration.

— ESP-IDF Programming Guide v5.5.6 documentation

A useful security stack for your eventual OTA project is:

- **TLS:** protects the connection to the update server.
- **Signed firmware / Secure Boot:** establishes that the firmware is authorized.
- **Flash Encryption:** protects supported flash contents at rest.
- **Anti-rollback:** prevents booting firmware below an enforced security version, when configured.

These mechanisms solve different problems.

## 6.6 Safe lab — Inspect before you enable

For this lesson, we will inspect your board only. **Do not burn eFuses or enable Flash Encryption as an experiment on your current working board.**

If Espressif's `espefuse` utility is installed and available in your terminal, connect your board through its USB serial port (for example, `COM7`) and run:

```bash
espefuse --chip esp32s3 --port COM7 summary
```

This is intended to read and display eFuse information. If your installed version uses a different command syntax, check `espefuse --help`.

Look for information relating to:

- `SPI_BOOT_CRYPT_CNT` — the counter associated with Flash Encryption state.
- Flash-encryption key blocks and their protection status.
- Secure Boot-related eFuse fields.
- Download-mode and debug-access restrictions.

The precise field names and output depend on the utility version and chip configuration. Do not share secret key material; a summary of status fields is enough.

> **Stop here if you are unsure.** Reading the summary is the exercise. Do not run `burn-efuse`, `write-protect-efuse`, or any command that changes security eFuses.

Sample efuse output:
```text
C:\Users\ooikk\Documents\HTML\FundPriceCrawller>espefuse --chip esp32s3 --port COM7 summary
espefuse v5.5.0
Connecting....

=== Run "summary" command ===
EFUSE_NAME (Block) Description  = [Meaningful Value] [Readable/Writeable] (Hex Value)
----------------------------------------------------------------------------------------
Calibration fuses:
K_RTC_LDO (BLOCK1)                                 BLOCK1 K_RTC_LDO                                   = 10 R/W (0b0001010)
K_DIG_LDO (BLOCK1)                                 BLOCK1 K_DIG_LDO                                   = 17 R/W (0b0010001)
V_RTC_DBIAS20 (BLOCK1)                             BLOCK1 voltage of rtc dbias20                      = 22 R/W (0x16)
V_DIG_DBIAS20 (BLOCK1)                             BLOCK1 voltage of digital dbias20                  = 29 R/W (0x1d)
DIG_DBIAS_HVT (BLOCK1)                             BLOCK1 digital dbias when hvt                      = 21 R/W (0b10101)
ADC2_CAL_VOL_ATTEN3 (BLOCK1)                       ADC2 calibration voltage at atten3                 = 31 R/W (0b011111)
TEMP_CALIB (BLOCK2)                                Temperature calibration data                       = -14.8 R/W (0b110010100)
OCODE (BLOCK2)                                     ADC OCode                                          = 88 R/W (0x58)
ADC1_INIT_CODE_ATTEN0 (BLOCK2)                     ADC1 init code at atten0                           = 122 R/W (0x7a)
ADC1_INIT_CODE_ATTEN1 (BLOCK2)                     ADC1 init code at atten1                           = 27 R/W (0b011011)
ADC1_INIT_CODE_ATTEN2 (BLOCK2)                     ADC1 init code at atten2                           = 27 R/W (0b011011)
ADC1_INIT_CODE_ATTEN3 (BLOCK2)                     ADC1 init code at atten3                           = 26 R/W (0b011010)
ADC2_INIT_CODE_ATTEN0 (BLOCK2)                     ADC2 init code at atten0                           = 145 R/W (0x91)
ADC2_INIT_CODE_ATTEN1 (BLOCK2)                     ADC2 init code at atten1                           = 37 R/W (0b100101)
ADC2_INIT_CODE_ATTEN2 (BLOCK2)                     ADC2 init code at atten2                           = 14 R/W (0b001110)
ADC2_INIT_CODE_ATTEN3 (BLOCK2)                     ADC2 init code at atten3                           = 29 R/W (0b011101)
ADC1_CAL_VOL_ATTEN0 (BLOCK2)                       ADC1 calibration voltage at atten0                 = 56 R/W (0x38)
ADC1_CAL_VOL_ATTEN1 (BLOCK2)                       ADC1 calibration voltage at atten1                 = 68 R/W (0x44)
ADC1_CAL_VOL_ATTEN2 (BLOCK2)                       ADC1 calibration voltage at atten2                 = 55 R/W (0x37)
ADC1_CAL_VOL_ATTEN3 (BLOCK2)                       ADC1 calibration voltage at atten3                 = 59 R/W (0x3b)
ADC2_CAL_VOL_ATTEN0 (BLOCK2)                       ADC2 calibration voltage at atten0                 = 118 R/W (0x76)
ADC2_CAL_VOL_ATTEN1 (BLOCK2)                       ADC2 calibration voltage at atten1                 = 61 R/W (0b0111101)
ADC2_CAL_VOL_ATTEN2 (BLOCK2)                       ADC2 calibration voltage at atten2                 = 66 R/W (0b1000010)

Config fuses:
WR_DIS (BLOCK0)                                    Disable programming of individual eFuses           = 0 R/W (0x00000000)
RD_DIS (BLOCK0)                                    Disable reading from BlOCK4-10                     = 0 R/W (0b0000000)
DIS_ICACHE (BLOCK0)                                Set this bit to disable Icache                     = False R/W (0b0)
DIS_DCACHE (BLOCK0)                                Set this bit to disable Dcache                     = False R/W (0b0)
DIS_TWAI (BLOCK0)                                  Set this bit to disable CAN function               = False R/W (0b0)
DIS_APP_CPU (BLOCK0)                               Disable app cpu                                    = False R/W (0b0)
DIS_DIRECT_BOOT (BLOCK0)                           Disable direct boot mode                           = False R/W (0b0)
UART_PRINT_CONTROL (BLOCK0)                        Set the default UART boot message output mode      = Enable R/W (0b00)
PIN_POWER_SELECTION (BLOCK0)                       Set default power supply for GPIO33-GPIO37; set wh = VDD_SPI R/W (0b1)
                                                   en SPI flash is initialized
PSRAM_CAP (BLOCK1)                                 PSRAM capacity                                     = 8M R/W (0b01)
PSRAM_TEMP (BLOCK1)                                PSRAM temperature                                  = 85C R/W (0b10)
PSRAM_VENDOR (BLOCK1)                              PSRAM vendor                                       = AP_3v3 R/W (0b01)
PSRAM_CAP_3 (BLOCK1)                               PSRAM capacity bit 3                               = False R/W (0b0)
BLOCK_USR_DATA (BLOCK3)                            User data
   = 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 R/W
BLOCK_SYS_DATA2 (BLOCK10)                          System data part 2 (reserved)
   = 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 R/W

Flash fuses:
FLASH_TPUW (BLOCK0)                                Configures flash waiting time after power-up; in u = 0 R/W (0x0)
                                                   nit of ms. If the value is less than 15; the waiti
                                                   ng time is the configurable value.  Otherwise; the
                                                    waiting time is twice the configurable value
FLASH_ECC_MODE (BLOCK0)                            Flash ECC mode in ROM                              = 16to18 byte R/W (0b0)
FLASH_TYPE (BLOCK0)                                SPI flash type                                     = 4 data lines R/W (0b0)
FLASH_PAGE_SIZE (BLOCK0)                           Set Flash page size                                = 0 R/W (0b00)
FLASH_ECC_EN (BLOCK0)                              Set 1 to enable ECC for flash boot                 = False R/W (0b0)
FORCE_SEND_RESUME (BLOCK0)                         Set this bit to force ROM code to send a resume co = False R/W (0b0)
                                                   mmand during SPI boot
FLASH_CAP (BLOCK1)                                 Flash capacity                                     = None R/W (0b000)
FLASH_TEMP (BLOCK1)                                Flash temperature                                  = None R/W (0b00)
FLASH_VENDOR (BLOCK1)                              Flash vendor                                       = None R/W (0b000)

Identity fuses:
DISABLE_WAFER_VERSION_MAJOR (BLOCK0)               Disables check of wafer version major              = False R/W (0b0)
DISABLE_BLK_VERSION_MAJOR (BLOCK0)                 Disables check of blk version major                = False R/W (0b0)
WAFER_VERSION_MINOR_LO (BLOCK1)                    WAFER_VERSION_MINOR least significant bits         = 2 R/W (0b010)
PKG_VERSION (BLOCK1)                               Package version                                    = 0 R/W (0b000)
BLK_VERSION_MINOR (BLOCK1)                         BLK_VERSION_MINOR                                  = 3 R/W (0b011)
WAFER_VERSION_MINOR_HI (BLOCK1)                    WAFER_VERSION_MINOR most significant bit           = False R/W (0b0)
WAFER_VERSION_MAJOR (BLOCK1)                       WAFER_VERSION_MAJOR                                = 0 R/W (0b00)
OPTIONAL_UNIQUE_ID (BLOCK2)                        Optional unique 128-bit ID
   = d4 31 6a 5c 9d 5e f3 14 55 f2 c1 a3 8c 7b 85 4c R/W
BLK_VERSION_MAJOR (BLOCK2)                         BLK_VERSION_MAJOR of BLOCK2                        = ADC calib V1 R/W (0b01)
WAFER_VERSION_MINOR (BLOCK0)                       calc WAFER VERSION MINOR = WAFER_VERSION_MINOR_HI  = 2 R/W (0x2)
                                                   << 3 + WAFER_VERSION_MINOR_LO (read only)
PSRAM_CAPACITY (BLOCK0)                            calc as = PSRAM_CAP_3 << 2 + PSRAM_CAP (read only) = 1 R/W (0b001)

Jtag fuses:
SOFT_DIS_JTAG (BLOCK0)                             Set these bits to disable JTAG in the soft way (od = 0 R/W (0b000)
                                                   d number 1 means disable ). JTAG can be enabled in
                                                    HMAC module
DIS_PAD_JTAG (BLOCK0)                              Set this bit to disable JTAG in the hard way. JTAG = False R/W (0b0)
                                                    is disabled permanently
STRAP_JTAG_SEL (BLOCK0)                            Set this bit to enable selection between usb_to_jt = False R/W (0b0)
                                                   ag and pad_to_jtag through strapping gpio3 when bo
                                                   th reg_dis_usb_jtag and reg_dis_pad_jtag are equal
                                                    to 0

Mac fuses:
MAC (BLOCK1)                                       MAC address
   = ac:a7:04:e0:4e:64 (OK) R/W
CUSTOM_MAC (BLOCK3)                                Custom MAC
   = 00:00:00:00:00:00 (OK) R/W

Security fuses:
DIS_DOWNLOAD_ICACHE (BLOCK0)                       Set this bit to disable Icache in download mode (b = False R/W (0b0)
                                                   oot_mode[3:0] is 0; 1; 2; 3; 6; 7)
DIS_DOWNLOAD_DCACHE (BLOCK0)                       Set this bit to disable Dcache in download mode (  = False R/W (0b0)
                                                   boot_mode[3:0] is 0; 1; 2; 3; 6; 7)
DIS_FORCE_DOWNLOAD (BLOCK0)                        Set this bit to disable the function that forces c = False R/W (0b0)
                                                   hip into download mode
DIS_DOWNLOAD_MANUAL_ENCRYPT (BLOCK0)               Set this bit to disable flash encryption when in d = False R/W (0b0)
                                                   ownload boot modes
SPI_BOOT_CRYPT_CNT (BLOCK0)                        Enables flash encryption when 1 or 3 bits are set  = Disable R/W (0b000)
                                                   and disabled otherwise
SECURE_BOOT_KEY_REVOKE0 (BLOCK0)                   Revoke 1st secure boot key                         = False R/W (0b0)
SECURE_BOOT_KEY_REVOKE1 (BLOCK0)                   Revoke 2nd secure boot key                         = False R/W (0b0)
SECURE_BOOT_KEY_REVOKE2 (BLOCK0)                   Revoke 3rd secure boot key                         = False R/W (0b0)
KEY_PURPOSE_0 (BLOCK0)                             Purpose of Key0                                    = USER R/W (0x0)
KEY_PURPOSE_1 (BLOCK0)                             Purpose of Key1                                    = USER R/W (0x0)
KEY_PURPOSE_2 (BLOCK0)                             Purpose of Key2                                    = USER R/W (0x0)
KEY_PURPOSE_3 (BLOCK0)                             Purpose of Key3                                    = USER R/W (0x0)
KEY_PURPOSE_4 (BLOCK0)                             Purpose of Key4                                    = USER R/W (0x0)
KEY_PURPOSE_5 (BLOCK0)                             Purpose of Key5                                    = USER R/W (0x0)
SECURE_BOOT_EN (BLOCK0)                            Set this bit to enable secure boot                 = False R/W (0b0)
SECURE_BOOT_AGGRESSIVE_REVOKE (BLOCK0)             Set this bit to enable revoking aggressive secure  = False R/W (0b0)
                                                   boot
DIS_DOWNLOAD_MODE (BLOCK0)                         Set this bit to disable download mode (boot_mode[3 = False R/W (0b0)
                                                   :0] = 0; 1; 2; 3; 6; 7)
ENABLE_SECURITY_DOWNLOAD (BLOCK0)                  Set this bit to enable secure UART download mode   = False R/W (0b0)
SECURE_VERSION (BLOCK0)                            Secure version (used by ESP-IDF anti-rollback feat = 0 R/W (0x0000)
                                                   ure)
BLOCK_KEY0 (BLOCK4)
  Purpose: USER
               Key0 or user data
   = 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 R/W
BLOCK_KEY1 (BLOCK5)
  Purpose: USER
               Key1 or user data
   = 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 R/W
BLOCK_KEY2 (BLOCK6)
  Purpose: USER
               Key2 or user data
   = 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 R/W
BLOCK_KEY3 (BLOCK7)
  Purpose: USER
               Key3 or user data
   = 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 R/W
BLOCK_KEY4 (BLOCK8)
  Purpose: USER
               Key4 or user data
   = 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 R/W
BLOCK_KEY5 (BLOCK9)
  Purpose: USER
               Key5 or user data
   = 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 R/W

Spi Pad fuses:
SPI_PAD_CONFIG_CLK (BLOCK1)                        SPI_PAD_configure CLK                              = 0 R/W (0b000000)
SPI_PAD_CONFIG_Q (BLOCK1)                          SPI_PAD_configure Q(D1)                            = 0 R/W (0b000000)
SPI_PAD_CONFIG_D (BLOCK1)                          SPI_PAD_configure D(D0)                            = 0 R/W (0b000000)
SPI_PAD_CONFIG_CS (BLOCK1)                         SPI_PAD_configure CS                               = 0 R/W (0b000000)
SPI_PAD_CONFIG_HD (BLOCK1)                         SPI_PAD_configure HD(D3)                           = 0 R/W (0b000000)
SPI_PAD_CONFIG_WP (BLOCK1)                         SPI_PAD_configure WP(D2)                           = 0 R/W (0b000000)
SPI_PAD_CONFIG_DQS (BLOCK1)                        SPI_PAD_configure DQS                              = 0 R/W (0b000000)
SPI_PAD_CONFIG_D4 (BLOCK1)                         SPI_PAD_configure D4                               = 0 R/W (0b000000)
SPI_PAD_CONFIG_D5 (BLOCK1)                         SPI_PAD_configure D5                               = 0 R/W (0b000000)
SPI_PAD_CONFIG_D6 (BLOCK1)                         SPI_PAD_configure D6                               = 0 R/W (0b000000)
SPI_PAD_CONFIG_D7 (BLOCK1)                         SPI_PAD_configure D7                               = 0 R/W (0b000000)

Usb fuses:
DIS_USB_OTG (BLOCK0)                               Set this bit to disable USB function               = False R/W (0b0)
USB_EXCHG_PINS (BLOCK0)                            Set this bit to exchange USB D+ and D- pins        = False R/W (0b0)
USB_EXT_PHY_ENABLE (BLOCK0)                        Set this bit to enable external PHY                = False R/W (0b0)
DIS_USB_JTAG (BLOCK0)                              Set this bit to disable function of usb switch to  = False R/W (0b0)
                                                   jtag in module of usb device
DIS_USB_SERIAL_JTAG (BLOCK0)                       Set this bit to disable usb device                 = False R/W (0b0)
USB_PHY_SEL (BLOCK0)                               This bit is used to switch internal PHY and extern
   = internal PHY is assigned to USB Device while external PHY is assigned to USB OTG R/W (0b0)
                                                   al PHY for USB OTG and USB Device
DIS_USB_SERIAL_JTAG_ROM_PRINT (BLOCK0)             USB printing                                       = Enable R/W (0b0)
DIS_USB_SERIAL_JTAG_DOWNLOAD_MODE (BLOCK0)         Set this bit to disable UART download mode through = False R/W (0b0)
                                                    USB
DIS_USB_OTG_DOWNLOAD_MODE (BLOCK0)                 Set this bit to disable download through USB-OTG   = False R/W (0b0)

Vdd fuses:
VDD_SPI_XPD (BLOCK0)                               SPI regulator power up signal                      = True R/W (0b1)
VDD_SPI_TIEH (BLOCK0)                              If VDD_SPI_FORCE is 1; determines VDD_SPI voltage
   = VDD_SPI connects to VDD3P3_RTC_IO R/W (0b1)
VDD_SPI_FORCE (BLOCK0)                             Set this bit and force to use the configuration of = True R/W (0b1)
                                                    eFuse to configure VDD_SPI

Wdt fuses:
WDT_DELAY_SEL (BLOCK0)                             RTC watchdog timeout threshold; in unit of slow cl = 40000 R/W (0b00)
                                                   ock cycle

Flash voltage (VDD_SPI) set to 3.3V by efuse.
```

## 6.7 Knowledge check

1. What is the primary purpose of Flash Encryption?
   - Prove the firmware was signed by the authorized publisher
   - Protect supported flash contents from being recovered through raw flash extraction
   - Encrypt all network traffic automatically

   **Correct.** Flash Encryption protects supported external flash contents at rest. TLS protects network traffic, and Secure Boot verifies firmware authenticity.

2. What is the main security weakness of Development mode?
   - It cannot run encrypted firmware
   - It always disables OTA
   - The ROM download interface retains encryption capabilities that can weaken protection against physical reflashing attacks

   **Correct.** Development mode retains capabilities useful for testing that should be restricted for production.

3. Does Flash Encryption automatically encrypt every LittleFS data file?
   - Yes, every byte of every partition is always encrypted
   - No; the partition configuration and supported write path matter
   - Only if the file is named `secret.txt`

   **Correct.** Data partitions must be configured appropriately; do not assume filesystem data is automatically protected.

4. Why combine Secure Boot with Flash Encryption?
   - They provide complementary protections: authorized firmware verification and confidentiality of flash contents
   - Flash Encryption makes Secure Boot unnecessary
   - Secure Boot encrypts all user data

   **Correct.** One protects the boot trust chain; the other protects supported flash contents at rest.

# Module 7 — Secure OTA: Hashes versus Digital Signatures

You already have HTTPS firmware downloading and OTA installation working on your ESP32-S3 N16R8. In this module, we'll strengthen that design so your device can verify who authorized a firmware release, not merely whether the downloaded file matches a hash.

We'll keep this practical and build on your existing Arduino-ESP32 Core 3.3.12 setup. We won't enable Secure Boot or burn eFuses during this lesson.

## 7.1 The security problem with a hash-only manifest

Imagine your ESP32 downloads two files from a cloud repository:

- `manifest.json` — firmware version, download URL and SHA-256 hash.
- `firmware.bin` — the actual firmware image.

Your device computes the SHA-256 hash of the downloaded firmware and compares it with the hash in the manifest.

<img width="1021" height="560" alt="image" src="https://github.com/user-attachments/assets/112575e3-16d8-428a-aac1-68424ee5a771" />


```text
Cloud repository
manifest.json + firmware.bin

ESP32-S3 downloads both files
Computes SHA-256 of firmware.bin

Compare calculated hash with manifest hash
Do the two values match?

Match → File matches the supplied hash
Mismatch → Reject the downloaded file
```

The weakness is subtle: what if an attacker replaces both the firmware and the manifest?

The modified firmware has a different hash, but the attacker also changes the manifest to contain that new hash. The comparison still passes.

HTTPS helps protect the connection to the server, but it doesn't fully solve every supply-chain problem. For example, a compromised server, compromised account, malicious publishing pipeline or incorrectly trusted endpoint could distribute both files together.

## 7.2 Three levels of OTA verification

| Mechanism | What it verifies | What it does not establish by itself |
|---|---|---|
| **SHA-256 hash** | File matches a known digest | Who authorized the file |
| **HTTPS/TLS** | Secure connection to an authenticated server, subject to correct validation | Independent authorization of a firmware release |
| **Digital signature** | Release was signed by the holder of an authorized private key, and signed content matches | That the signed firmware is bug-free or safe |

A robust OTA system often uses all three.

The important difference is the trust anchor: a hash is useful for integrity when its expected value is trusted; a digital signature lets the device verify the publisher using a public key provisioned independently of the update files.

## 7.3 The solution: digitally sign the manifest

We'll use a detached signature: keep the manifest and signature in separate files.

### Publisher — private key

The release system signs the exact bytes of `manifest.json`. The private key stays on your development/release machine or a protected signing service; it is never uploaded to the ESP32.

### Update repository

Publishes `manifest.json`, `manifest.sig` and `firmware.bin`. The signature is separate from the manifest, avoiding ambiguity about which JSON fields were signed.

### ESP32 — trusted public key

Verifies the signature against the manifest bytes using a public key embedded in, or otherwise securely provisioned to, the device. Only after successful verification does it trust the manifest's fields.

The manifest could contain:

```json
{
  "product": "esp32s3-n16r8",
  "version": "1.2.0",
  "firmware_url": "https://example.com/firmware.bin",
  "size": 524288,
  "sha256": "REPLACE_WITH_64_HEX_CHARACTERS"
}
```

These values are illustrative. The URL, version and size must match your actual release.

The signature covers the exact bytes of the JSON file, including whitespace and line endings. If you reformat the JSON after signing it, signature verification should fail. That is intentional: the device verifies the precise document the publisher signed.

## 7.4 The correct verification order

For your ESP32-S3 OTA updater, use this sequence:

| Step | Action |
|---:|---|
| 1 | **Download manifest and detached signature** — Use TLS with proper CA and hostname validation. |
| 2 | **Verify manifest signature** — Reject the update immediately if verification fails. |
| 3 | **Parse and validate manifest** — Check product identity, version policy, size limits and HTTPS URL. |
| 4 | **Download firmware** — Enforce the expected size and handle download errors. |
| 5 | **Verify firmware SHA-256** — Compare the actual firmware digest with the signed manifest value. |
| 6 | **Install and reboot** — Only install after all checks pass; validate the new boot and recovery strategy. |

Why verify the manifest signature before downloading firmware? Because the signed manifest determines which firmware URL, size and hash are authorized. Downloading the firmware first can waste bandwidth, although it must never be installed before all verification succeeds.

A signed manifest still needs policy checks. A validly signed release might have the wrong product identifier, an unsupported version or an unsafe size. The device should reject those too.

## 7.5 Lab 7A — Demonstrate why a hash alone is insufficient

This first experiment runs on your Windows PC, so it won't alter your ESP32 or eFuses. It uses OpenSSL to demonstrate the trust difference.

Refer to: [Install OpenSSL](https://github.com/ooikk/Arduino-Documentation/blob/main/02_MISC/02_OTA/02_Security/Security.md#note-install-openssl)

>Note: Build a firmware.bin and take note of the SHA256 and file size (use dir to display file size) for use in this experiment. These information are required for creating `manifest.json` file later. Run `openssl` in the same folder where firmware.bin is located.

### Step 1 — Create a test signing key

In a terminal with OpenSSL available:

```bash
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out test-private.pem

openssl pkey -in test-private.pem -pubout -out test-public.pem
```
Example of the output key: `test-private.pem`

```text
-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQD2LtLduyPQJ4b4
8OCjKFmN93O63D32Bn2sY+xw8cj7CIdB4sBTMCiBFrdMLnAMw11euJljKGlN3qiD
ZgPywNT1eUC7MHpHydTle9htnIEmU2ExXIIK3Neo9as5niPiiYf3bruoZ90YAL19
NI2lMPYY/wVEArCL9ldNZCBl1YdhslG0RE3PkpVAZ0vBkBxFkaq1XDBzKOWTJo0D
kMZucM16KbT/shzPH8WP1nPPhF8+M8YrjnVJ2+nQf77BRoA4SHt/GmqKMmo3cSYt
9DTiF8riZ7qTrB40CO9cB4YzfNarVPaRVMletaHt9MmyqK02HnUJJRBaUbRWEWEf
or1nKBJPAgMBAAECggEAGi0CXhx+2KwgmCmXM7dKRmO673G+44DvLeb2Bfr/bAm3
xVP52nPdaqm4AFFTuAJcftd79Q91RZgDgkJ/mvkmlkSNPcS6y7/0c24b+2j6USaT
yAQruOygavusIr/jgDvatClsVKID39XRpWrIFiTC61zPpNqbBlbBaPODf7BuPlSF
IpHHzA1zCD84OJKcEtug6dETki88JWyLLk7AAwY/5bcUmIcZGC9QLpbtobdislcU
HWFel4lb0s+U0a2r5c8z1HK8ZE/Rp8nR7BlLaMsUR9qXSyPyWClD+HWk5djN/JiQ
TSbnNsYec5sYu67yObNxEDaqGzLjBTheBt/dj6IBCQKBgQD+qpNF2DnB5BD5mrYp
Noq/1MOphRTjzJuykTLDa8pLy1E3ozjx4DvqxcRM+MMpxFRSLiJO8QWMj5CzYYiW
jp6lXT/MkCgSpprMSI5VAn5qKpeBVqg6qIEXOSVUZo5MHMgx1qb5N0T4xdLob3D0
KivWY400gPYqz3qtx7S9AbLnXQKBgQD3eN/7B0g0ceSWqa4ljkXIseo3oNzgSIsy
zmKhA9EArItUSmwBSWvLfE44yso06RxC+jI9dVwc284BpCddg/qsJ2rUlE5hny8r
OuQDSQfIrPsBWIXZR270sQX54UudsdWinEPS6LRZAWDloVOlk5IeXyo40yMNGo23
Gzf2ft8hmwKBgBKy25VpRvRhlo19OprUEmP7b1JnHx2KzHPUDXIj2/qstw3qwieg
W0Xlt4AfU9UhQT+3e3VoZQvpDqrdInZLioKB0klm3EpiSUNuoGTFhhqtfepVPozw
Cmp2G7OSxCf6hQiDxkbONBZAEDhY1mZXIljffmK4hVqnWU6hNzgZg9VRAoGBAPCO
5VsFdyDT3Zq/EnObWyZ+G+k7S7oRio42pRVTumTnVQD43whZMJm7GeaG6gyI1Voe
D5xCaun6mMRrp4B1/MRABM7j41VuAdIKCv9OjGx8CfsC7AmMMMVXFzIR2QlPhxXs
xNlOL5Gdj21oZwE68/zmEQrv4tCoET3/DWVlas0lAoGAY74afiJEYqf4eibkNvGl
FO1iVwWTboJLy/WCPa38XjvZtUUCgEv6vwvNCSQ1LKWGyJ3JrYhMl80cArBdHWkv
c3k9U055uoHv+gj9/SCcu57t/MzkecVRLENXJXODEWTHmiUM4IP5uaTU9a5zMVsk
Izo4JlWFiIWBQZgphPdbwgc=
-----END PRIVATE KEY-----
```

Example of the output key: `test-public.pem`

```text
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA6B0VjJa/Y3DtK2Qbtszj
TQxFNDq9hkiUwU4iNh8ikUtpExot4auZnzvqORVVI+0FBCrrL2w7gfPGVyS+EETS
/3P3QsD5pKs1RvOhubQpgWY4I4wPLdeA5LlD3EL6ojaZA8KVpKJR1ZPKQ4DYC5qG
14+gcl4cAs4lidNwRpfg7GiN+zk0c4coieFUQ7VIKAmP49rbdufWwYbZ0rWav/Tw
29QGKX3OPFaF4eQ9RqmKjMskZQc7LuWg5rw3yuWqg9wVnVhZP4TK3Uy5Z6Pwzkla
L1f6h6P+8Fn7m9PfyuRnlSRtbAne68aYH1uJT2r2hc89jW5e0/E+UMBVtTsMSTaC
nQIDAQAB
-----END PUBLIC KEY-----
```


These are disposable lab keys. Do not use them for production releases. Keep `test-private.pem` private; the public key is what the device needs.

### Step 2 — Create and hash a manifest

Save the JSON manifest from Section 7.3 as `manifest.json`, then run:

```bash
openssl dgst -sha256 manifest.json
```

Record the resulting digest.

Now change the manifest's version or firmware URL and hash it again. The digest changes, as expected.

### Step 3 — Sign the manifest

Sign the original manifest:

```bash
openssl dgst -sha256 -sign test-private.pem -out manifest.sig manifest.json
```

Verify the signature:

```bash
openssl dgst -sha256 -verify test-public.pem -signature manifest.sig manifest.json
```

Expected output:

```text
Verified OK
```

Now modify `manifest.json` without generating a new signature, then run the verification command again. It should report that verification failed.

Restore the original manifest before continuing.

### Step 4 — Simulate a compromised repository

Imagine an attacker replaces the manifest with a different version and recalculates its SHA-256 hash. A hash-only client that trusts the replacement manifest can still accept the matching hash.

But the attacker cannot create a valid signature for the replacement manifest without the authorized private key. Signature verification fails.

> **Lab result:** a hash can detect an accidental or unauthorized change only when the expected hash itself is trusted. A digital signature provides a way to authenticate the publisher's signed content.

## 7.6 How this connects to your Arduino OTA implementation

Your existing updater already uses `WiFiClientSecure`, `HTTPClient` and `Update.h`. You don't need to replace the entire updater. You need to add a verification stage before trusting the manifest.

For an Arduino-ESP32 implementation, the main functions would be separated like this:

```cpp
bool verifyManifestSignature(
    const uint8_t* manifest,
    size_t manifestLength,
    const uint8_t* signature,
    size_t signatureLength);

bool parseAndValidateManifest(
    const uint8_t* manifest,
    size_t manifestLength);

bool verifyFirmwareSHA256(
    const uint8_t* expectedHash,
    const uint8_t* actualHash,
    size_t hashLength);
```

The signature-verification function uses mbedTLS public-key verification with the trusted public key and the signature algorithm used by your signing tool. For example, the OpenSSL commands above produce an RSA/SHA-256 signature using PKCS#1 v1.5 padding. The device verifier must use compatible settings; that is different from Secure Boot V2's RSA-PSS signature scheme.

The exact Arduino implementation should be built and tested against the mbedTLS API included in your installed ESP32 core. Keep these rules:

- Verify the signature over the exact downloaded manifest bytes, before trusting parsed values.
- Reject malformed signatures, manifests that exceed a fixed size limit, invalid URLs and oversized firmware.
- Compare SHA-256 over the actual firmware bytes, not the manifest or a filename.
- Don't call `Update.begin()` and write firmware until all required checks have passed.
- Check the final update result and design a safe rollback/recovery process.

For a streaming firmware download, you can compute SHA-256 incrementally rather than storing the entire firmware image in RAM. You must ensure that verification finishes before the device commits to running the new image. A suitable dual-OTA partition layout and boot-selection strategy are important here.

## 7.7 Production considerations

Before treating this as a production-ready OTA system, there are three additional issues to solve.

- **Anti-rollback:** a valid signature alone does not prevent reinstalling an older, vulnerable release. Enforce a version policy and, for stronger hardware-backed enforcement, evaluate ESP32-S3 anti-rollback support.
- **Signing-key protection:** if an attacker steals your release private key, they may be able to sign malicious firmware that passes your manifest check. Protect backups, restrict signing access and plan key rotation.
- **Firmware authenticity at boot:** signed-manifest verification is an application-level check. It does not replace Secure Boot V2, which independently verifies firmware at boot.

## 7.8 Knowledge check

1. An attacker replaces both `firmware.bin` and `manifest.json`, including the SHA-256 hash. What can happen with a hash-only updater?
   - It must reject the firmware
   - It can accept the replacement if the new firmware matches the untrusted hash
   - The hash automatically identifies the publisher

   **Correct.** The attacker controls both the data and the expected hash, so the comparison alone doesn't authenticate the release.

2. Which key should the ESP32 use to verify a signed manifest?
   - The publisher's private signing key
   - A public key trusted independently of the downloaded update files
   - A key supplied only inside the same unsigned manifest

   **Correct.** The device verifies with a trusted public key; the private signing key must remain protected.

3. Why sign the manifest before trusting its URL and firmware hash?
   - The signature authenticates the manifest's contents, including the URL and expected digest
   - It makes TLS unnecessary
   - It encrypts the firmware automatically

   **Correct.** The signed manifest authenticates the release metadata. TLS is still useful, and firmware encryption is a separate concern.

4. If an old firmware image has a valid signature, what additional protection may be needed?
   - A longer manifest filename
   - Anti-rollback enforcement
   - Disabling SHA-256

   **Correct.** A valid signature doesn't by itself prove that the release is recent enough.

# Module 8 — Design a Secure ESP32-S3 IoT Device

This final module brings together everything you've learned and applies it to your ESP32-S3 N16R8 development board.

We'll use a realistic project: an ESP32-S3 device that reads sensors, controls a DC motor, connects to Wi-Fi, reports telemetry to a cloud service and installs firmware updates over the air.

The objective is not simply to enable every security feature. It is to identify each threat, select the right protection, and deploy it without losing the ability to update or recover the device.

## 8.1 The complete security architecture

<img width="787" height="649" alt="image" src="https://github.com/user-attachments/assets/e7b26a86-d14f-4216-a105-5cd0f838813a" />



```text
Cloud service and firmware repository
TLS endpoint · Signed manifest · Firmware releases

Authenticated communications
TLS certificate and hostname validation · Optional mutual TLS

ESP32-S3 N16R8
Secure Boot V2
Authorized firmware

Flash Encryption
Flash confidentiality

NVS Encryption
Protected stored secrets

Secure OTA
Signed metadata and hash checks

Application and peripherals
FreeRTOS tasks · Motor control · Sensors · GPIO
```

These protections operate at different layers. TLS protects network communication; signed manifests authenticate release metadata; Secure Boot verifies the firmware boot chain; Flash Encryption protects supported flash contents at rest; and NVS Encryption protects appropriately configured stored values.

For production, also consider debug/download access restrictions, anti-rollback, secure manufacturing, key rotation, fault handling and physical tampering.

## 8.2 Threat model: what can go wrong?

A threat model helps us prioritize the protections instead of enabling features blindly.

| Threat | Example | Primary mitigation |
|---|---|---|
| **Network interception** | Attacker intercepts telemetry or update traffic | TLS with certificate and hostname validation |
| **Malicious firmware update** | Repository serves an unauthorized release | Signed manifest and firmware verification; Secure Boot V2 |
| **Flash extraction** | Attacker reads stored firmware or data | Flash Encryption; NVS Encryption where appropriate |
| **Credential theft** | Wi-Fi or cloud token is exposed | Protected storage, restricted logging, credential rotation |
| **Firmware downgrade** | Device reinstalls a vulnerable signed release | Enforced anti-rollback policy |
| **Unauthorized physical access** | Attacker uses debug or download interfaces | Appropriate eFuse configuration and interface restrictions |
| **Compromised signing key** | Attacker signs malicious releases | Protected signing infrastructure, restricted access, key rotation and revocation strategy |
| **Failed update** | Power is lost during installation or new firmware crashes | Dual OTA slots, validation and recovery/rollback design |

No single mechanism eliminates every threat. For example, Secure Boot cannot make a vulnerable but correctly signed application safe, and TLS cannot protect secrets from malicious firmware already running on the device.

## 8.3 A realistic deployment plan

For your existing Arduino OTA project, I recommend the following staged approach.

### Stage 1 — Harden the current software

- Keep TLS certificate and hostname validation enabled.
- Ensure system time is set before certificate date checks.
- Remove production use of `setInsecure()`.
- Never print Wi-Fi passwords, tokens or private keys.
- Add explicit update size limits and clear error handling.

### Stage 2 — Authenticate software releases

- Create a release manifest with product ID, version, size and SHA-256.
- Sign the exact manifest bytes with a protected publisher private key.
- Verify the signature on the ESP32 using a trusted public key.
- Verify the firmware's SHA-256 before accepting the update.

### Stage 3 — Validate update and recovery behavior

- Confirm that the partition table has suitable OTA slots.
- Test interrupted downloads, wrong hashes and invalid signatures.
- Test what happens if the new application fails during boot.
- Keep a documented recovery route while developing.

### Stage 4 — Protect persistent secrets

- Identify which NVS values are actually confidential.
- Configure and test NVS Encryption using a supported ESP-IDF-based build and provisioning workflow.
- Confirm how the encryption keys are protected and recovered.

### Stage 5 — Provision hardware security

- Integrate Secure Boot V2 and Flash Encryption.
- Test signing, first boot, OTA updates and recovery on dedicated test hardware.
- Verify eFuse settings and debug/download restrictions.
- Only then define a production provisioning procedure.

> **Important:** Stage 5 is not just a Tools-menu setting. With your Arduino-ESP32 Core 3.3.12 setup, first verify the exact supported build and signing workflow, bootloader configuration, partition table and provisioning procedure. Do not experiment with irreversible eFuse changes on your working development board.

## 8.4 Design decision: where should the trust anchor live?

For signed OTA, the device needs a trusted public key that an attacker cannot simply replace alongside the manifest.

There are several approaches:

### Option A — Public key embedded in the application

Simple to prototype. However, replacing the application could also replace its embedded verification key if the boot chain does not prevent unauthorized firmware.

### Option B — Public key protected by the secure boot chain

Stronger when the firmware containing the key is itself authenticated by Secure Boot. Key updates and rotation still require a planned trust strategy.

### Option C — Hardware-rooted trust and controlled key management

For higher-assurance designs, use the ESP32-S3 security features and a carefully designed signing-key lifecycle. Keep release private keys off the device and protect them in the release environment.

The public key is not secret. Its authenticity and resistance to unauthorized replacement are what matter.

## 8.5 Final project — Secure OTA acceptance test

Use this as your capstone checklist before considering your ESP32-S3 update mechanism secure.

### Acceptance checklist

**0 of 10**

- [ ] HTTPS works with the correct CA and hostname, and rejects an untrusted certificate.
- [ ] The manifest signature is checked before its fields are trusted.
- [ ] A modified manifest is rejected even if the attacker recalculates its SHA-256 field.
- [ ] A firmware image with a wrong hash is never installed.
- [ ] Wrong product IDs, invalid URLs, excessive image sizes and disallowed versions are rejected.
- [ ] Interrupted downloads do not corrupt the currently working firmware.
- [ ] The device has a tested recovery path after a failed update.
- [ ] Production logs contain no passwords, private keys or access tokens.
- [ ] NVS Encryption, Secure Boot and Flash Encryption are configured only after testing their exact provisioning workflows.
- [ ] Key rotation, anti-rollback and production recovery requirements are documented.

**Copy checklist**

> This checklist is a design and testing aid, not a certification. Passing it does not prove that the whole product is free from vulnerabilities.

## 8.6 Final assessment

1. Your updater uses HTTPS and validates the server certificate. Does that alone establish that a firmware release was authorized by your publisher?
   - Yes, always
   - No; verify a trusted publisher signature as well
   - Only when the filename ends in `.bin`

   **Correct.** TLS authenticates the server connection. Signed release metadata independently authenticates the publisher's authorized content.

2. What does Secure Boot add to a signed-manifest OTA system?
   - It independently checks firmware authenticity in the boot chain
   - It replaces TLS certificate validation
   - It automatically encrypts every NVS value

   **Correct.** Secure Boot verifies the boot chain, providing protection even if an update path has a flaw.

3. Why should Flash Encryption and Secure Boot be tested together before production provisioning?
   - They use exactly the same key and do the same thing
   - Their configuration affects flashing, booting, updates and recovery, and eFuse changes may be irreversible
   - They are enabled automatically by selecting the ESP32-S3 board

   **Correct.** Production security depends on the complete build and provisioning workflow, not just an individual feature.

4. What should happen if the firmware's SHA-256 does not match the value in a verified signed manifest?
   - Install it and log a warning
   - Install it if TLS succeeded
   - Reject the firmware and do not install it

   **Correct.** A mismatch means the downloaded image does not match the signed release metadata.

5. Which is the strongest overall design among these choices?
   - HTTPS only
   - Flash Encryption only
   - Validated TLS, signed update metadata, firmware hash verification, Secure Boot, protected secrets and tested recovery

   **Correct.** A layered design addresses different threats; no individual control covers all of them.

## Where you go from here

You've now covered all eight modules: security architecture, security properties, TLS, credential storage, Secure Boot V2, Flash Encryption, signed OTA, and system-level design.

My recommendation is to make the next step a hands-on hardening of your existing ESP32-S3 OTA updater, before attempting eFuse provisioning. Start by reviewing your current `downloadAndInstall()` flow and adding signed-manifest verification in a way that preserves your working TLS and OTA behavior.

That gives you a practical path from a functioning update demo to a much stronger security design—while keeping your current development board recoverable.

---

# Hands-on lab: Harden your ESP32-S3 OTA updater

We'll build on your working ESP32-S3 N16R8 setup: Arduino IDE 2.3.10, Arduino-ESP32 Core 3.3.12, `WiFiClientSecure`, `HTTPClient`, mbedTLS SHA-256 and `Update.h`.

Your existing updater already downloads firmware over HTTPS and checks its SHA-256. We'll add the missing trust layer: a digitally signed manifest that authenticates the firmware release before the updater trusts its URL, version or hash.

## 1. What we're going to implement

| Step | Action |
|---:|---|
| 1 | **Download manifest and signature** — HTTPS with your trusted root CA |
| 2 | **Verify publisher signature** — Reject an invalid or modified manifest |
| 3 | **Validate release metadata** — Check product, version, URL and firmware size |
| 4 | **Download firmware to inactive OTA slot** — Calculate SHA-256 as data arrives |
| 5 | **Verify the complete firmware hash** — Abort the update if the hash or size is wrong |
| 6 | **Finalize update** — Check `Update.end()`, then reboot only on success |

We'll use a plain-text manifest rather than JSON for this first implementation. This keeps the code smaller and makes it easier to see exactly which bytes are signed. We can switch to JSON later without changing the security principle.

## 2. Release file format

Publish three files in the same GitHub repository directory:

| File | Purpose |
|---|---|
| `version.txt` | Signed release metadata |
| `version.sig` | RSA-2048 signature of the exact `version.txt` bytes |
| `firmware.bin` | Compiled ESP32-S3 firmware |

Example `version.txt` contents — the file should end with a newline:

```text
1.2.0
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
524288
```

The three lines are:

1. Firmware version (`major.minor.patch`).
2. SHA-256 of the exact `firmware.bin` file, as 64 hexadecimal characters.
3. Firmware size in bytes.

The values above are examples; replace the hash and size with those of your actual compiled firmware.

The manifest signature authenticates all three lines. The firmware hash then binds the downloaded binary to that signed metadata.

## 3. Generate the signing key and signature on your PC

Use OpenSSL on your development PC. These commands create a lab key, not a production signing key.

```bash
# Generate a 2048-bit RSA private key.
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out ota-private.pem

# Export the public key for the ESP32.
openssl pkey -in ota-private.pem -pubout -out ota-public.pem

# Calculate the firmware hash.
openssl dgst -sha256 firmware.bin

# Obtain the firmware size in bytes.
```

Use the actual hash and file size to prepare `version.txt`. Then sign the exact file:

```bash
openssl dgst -sha256 -sign ota-private.pem -out version.sig version.txt
```

This command uses RSA with SHA-256 and PKCS#1 v1.5 padding. The ESP32 verifier below uses the matching algorithm. Keep `ota-private.pem` off the ESP32 and out of your public repository. Publish only the firmware, manifest, signature and public files you intend to distribute.

## 4. Arduino implementation

This sample includes the signature check, metadata validation, version check, download size checks and firmware hash verification. It assumes that Wi-Fi is already connected and the device clock has been synchronized, as in your existing updater.

Before compiling:

- Replace `OWNER`, `REPO` and `BRANCH` in the three URLs.
- Reuse your existing `ROOT_CA` declaration containing the trusted CA certificates.
- Replace the public-key placeholder with the complete contents of `ota-public.pem`.
- Set `CURRENT_VERSION` to the version running on your board.
- Publish `version.txt`, `version.sig` and `firmware.bin` at the specified raw GitHub URLs.

The code uses the Arduino-ESP32 `Update.setSHA256()` feature to verify the firmware payload before finalizing the update. The Arduino Update API documents that the digest is checked when the update is completed.

- [ESP32 Arduino Update Library](https://github.com/espressif/arduino-esp32/blob/master/libraries/Update/README.md)
- [Update.h](https://github.com/espressif/arduino-esp32/blob/master/libraries/Update/src/Update.h)


```cpp
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>

#include <mbedtls/pk.h>
#include <mbedtls/sha256.h>

// Reuse the ROOT_CA declaration from your working OTA sketch.
// Example expected declaration:
// extern const char ROOT_CA[];

// IMPORTANT: replace these URLs with your repository paths.
// Redirects are intentionally disabled.
static const char MANIFEST_URL[] =
  "https://raw.githubusercontent.com/OWNER/REPO/BRANCH/version.txt";

static const char SIGNATURE_URL[] =
  "https://raw.githubusercontent.com/OWNER/REPO/BRANCH/version.sig";

static const char FIRMWARE_URL[] =
  "https://raw.githubusercontent.com/OWNER/REPO/BRANCH/firmware.bin";

// Change this to the version currently running on the device.
static const char FIRMWARE_VERSION[] = "1.0.0";

// Adjust to your partition layout. Update.begin() also checks
// whether the selected OTA partition has sufficient space.
static constexpr size_t MAX_MANIFEST_SIZE = 512;
static constexpr size_t MAX_FIRMWARE_SIZE = 4 * 1024 * 1024;
static constexpr size_t RSA_SIGNATURE_SIZE = 256;

// Paste the complete PEM public key exported from ota-public.pem.
// This is public verification material, NOT the private key.
static const char OTA_PUBLIC_KEY[] PROGMEM = R"KEY(
-----BEGIN PUBLIC KEY-----
REPLACE_WITH_YOUR_REAL_PUBLIC_KEY
-----END PUBLIC KEY-----
)KEY";

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

  if (status != HTTP_CODE_OK ||
      length <= 0 ||
      (size_t)length > maxBytes) {
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

  if (status != HTTP_CODE_OK ||
      length != (int)expectedLength) {
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
    strlen(OTA_PUBLIC_KEY) + 1
  );

  if (rc != 0) {
    Serial.printf("Public key parse failed: -0x%04X\n",
                  (unsigned)(-rc));
    mbedtls_pk_free(&publicKey);
    return false;
  }

  uint8_t digest[32];

  mbedtls_sha256(
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
    sizeof(digest),
    signature,
    signatureLength
  );

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
             &a, &b, &c, &extra) != 3) {
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

  if (!parseVersion(candidate, a, b, c) ||
      !parseVersion(String(current), x, y, z)) {
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
      (c >= '0' && c <= '9') ||
      (c >= 'a' && c <= 'f') ||
      (c >= 'A' && c <= 'F');

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
bool parseManifest(const String& manifest,
                   String& version,
                   String& hash,
                   size_t& firmwareSize) {
 // if (manifest.indexOf('\r') >= 0) return false;

  int p1 = manifest.indexOf('\n');
  int p2 = manifest.indexOf('\n', p1 + 1);

  if (p1 < 0 || p2 < 0) {
    Serial.println("Malformed manifest: missing field separator");
    return false;
  }

  version = manifest.substring(0, p1);
  hash = manifest.substring(p1 + 1, p2);
  String sizeText = manifest.substring(p2 + 1);

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

  if (parsedSize == 0 ||
      parsedSize > MAX_FIRMWARE_SIZE) {
    return false;
  }

  firmwareSize = (size_t)parsedSize;
  return true;
}

bool downloadAndInstall() {
  Serial.println("\nStarting authenticated OTA");

  // 1. Download the manifest.
  String manifest;
  if (!fetchText(MANIFEST_URL, manifest, MAX_MANIFEST_SIZE)) {
    Serial.println("Manifest download failed");
    return false;
  }

  // 2. Download its raw RSA-2048 signature.
  uint8_t signature[RSA_SIGNATURE_SIZE];

  if (!fetchSignature(SIGNATURE_URL, signature,
                      sizeof(signature))) {
    Serial.println("Signature download failed");
    return false;
  }

  // 3. Authenticate the manifest BEFORE trusting its fields.
  if (!verifyManifestSignature(
        manifest, signature, sizeof(signature))) {
    Serial.println("OTA rejected: invalid signature");
    return false;
  }

  // 4. Parse and validate signed metadata.
  String version, expectedHash;
  size_t expectedSize = 0;

  if (!parseManifest(manifest, version,
                     expectedHash, expectedSize)) {
    Serial.println("OTA rejected: malformed manifest");
    return false;
  }

  Serial.printf("Release version: %s\n", version.c_str());
  Serial.printf("Expected size: %u bytes\n",
                (unsigned)expectedSize);

  // Basic version policy: accept only a strictly newer version.
  // This is not hardware-backed anti-rollback.
  if (!isNewerVersion(version, FIRMWARE_VERSION)) {
    Serial.println("OTA rejected: version is not newer");
    return false;
  }

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

  if (status != HTTP_CODE_OK ||
      contentLength < 0 ||
      (size_t)contentLength != expectedSize) {
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
```

Call `downloadAndInstall()` from your existing OTA trigger after Wi-Fi and time synchronization are ready. Keep your existing Wi-Fi connection code and `ROOT_CA` declaration; don't create a second, conflicting CA definition.

## 5. Create a signing key

In a terminal with OpenSSL available, create ota-private.pem:

```powershell
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out ota-private.pem
```

create ota-public.pem:

```powershell
openssl pkey -in ota-private.pem -pubout -out ota-public.pem
```

Replace your public key in the code:

```cpp
static const char OTA_PUBLIC_KEY[] PROGMEM = R"KEY(
-----BEGIN PUBLIC KEY-----
REPLACE_WITH_YOUR_REAL_PUBLIC_KEY
-----END PUBLIC KEY-----
)KEY";
```

## 6. Prepare the release files correctly

On Windows PowerShell, after compiling your firmware and saving the binary as `firmware.bin`, get the size:

```powershell
(Get-Item .\firmware.bin).Length
```

Get the SHA-256 digest:

```powershell
(Get-FileHash .\firmware.bin -Algorithm SHA256).Hash.ToLower()
```

Put those values in `version.txt`, then sign it again whenever any byte in the manifest changes:

```powershell
openssl dgst -sha256 -sign ota-private.pem -out version.sig version.txt
```

Verify the signature against the local manifest:

```powershell
openssl dgst -sha256 -verify ota-public.pem -signature version.sig version.txt
```

Expected output:

```powershell
Verified OK
```


Upload all three files to the exact directory referenced by your URLs. Ensure that `version.txt` is saved with LF line endings and a final newline, as required by the sample parser.

## 7. Test the security checks before trusting the updater

Run these tests on your development board. Start with a known-good firmware image and keep your USB recovery method available.

| Test | Change | Expected result |
|---:|---|---|
| 1 | Correct manifest, signature and firmware | Update succeeds |
| 2 | Modify the manifest without regenerating the signature | Rejected at signature verification |
| 3 | Replace the signature with a random 256-byte file | Rejected at signature verification |
| 4 | Publish different firmware but keep the signed manifest unchanged | Rejected at the SHA-256 check |
| 5 | Change the signed firmware size to an incorrect value and resign the manifest | Rejected when the download size doesn't match |
| 6 | Publish a version equal to or lower than `CURRENT_VERSION` | Rejected by the sample's version policy |
| 7 | Use an untrusted TLS certificate | Rejected before the response can be trusted |

For the hash-mismatch test, restore the correct release files after testing. Do not deliberately interrupt power during your first trials.

## 8. Three important limitations

- **The manifest signature is not Secure Boot.** An attacker who can install arbitrary firmware through an unprotected boot chain may replace the verification code and public key. Secure Boot V2 provides the separate hardware-rooted boot verification layer.
- **The version comparison is not hardware anti-rollback.** It helps with normal update policy, but a previously authorized older application might bypass this software check. Production anti-rollback requires a stronger enforced version policy.
- **The example assumes a suitable OTA partition layout.** `Update.begin()` can fail if the inactive partition is too small. Your 16 MB flash capacity alone doesn't guarantee that the current partition table provides enough space.

One further option worth evaluating: Arduino-ESP32 Core 3.3.12's Update library documents built-in RSA/ECDSA firmware signature verification behind the `UPDATE_SIGN` build option. That approach can authenticate the firmware image itself, rather than only authenticating a manifest and its hash. It requires the matching firmware-signing tool and signed-binary format.

- [Welcome to ESP32 Arduino Core’s documentation](https://documentation.espressif.com/arduino-esp32/en/latest/index.html)

> **My recommendation:** first compile this manifest-verification version with your existing CA certificate and test the seven cases above. If the compiler reports an API mismatch, share the exact error; we can adapt it to your installed Core 3.3.12 rather than changing your working OTA setup unnecessarily.

## 9. Append a release log / release notes

You can append a release log / release notes section to the end of `version.txt` and display it on the ESP32-S3 Serial Monitor only after confirming that a newer firmware version is available.

There is one important change: your current `parseManifest()` expects exactly three lines. You must update it to accept the additional release notes, while keeping the digital signature verification over the entire original `version.txt` file.

### 1. Suggested `version.txt` format

For example, your manifest could become:

```text
1.2.0
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
988288
Release notes:
- Added signed manifest verification.
- Improved OTA download error handling.
- Fixed a Wi-Fi reconnection issue.
```

The fields are:

| Line(s) | Content | Validation |
|---|---|---|
| 1 | Firmware version | Valid version format |
| 2 | SHA-256 firmware hash | Exactly 64 hexadecimal characters |
| 3 | Firmware size | Valid decimal integer within the permitted range |
| 4 onward | Release notes | Optional human-readable text |

The hash above is illustrative. Use the actual SHA-256 digest of your firmware binary.

**Important:** After adding or changing release notes, you must regenerate `version.sig`, because the signature covers the complete manifest, including the release notes.

### 2. Update `parseManifest()`

I recommend adding a `releaseNotes` output parameter:

```cpp
bool parseManifest(
    const String& manifest,
    String& version,
    String& expectedHash,
    size_t& expectedSize,
    String& releaseNotes
);
```

The parser should:

- Extract and validate the first three lines as before.
- Extract everything after the third newline into `releaseNotes`.
- Permit an empty release-notes section if you want it to be optional.
- Reject malformed required fields as before.

The key extraction logic is:

```cpp
int firstNewline = manifest.indexOf('\n');
int secondNewline = manifest.indexOf('\n', firstNewline + 1);
int thirdNewline = manifest.indexOf('\n', secondNewline + 1);

if (firstNewline < 0 ||
    secondNewline < 0 ||
    thirdNewline < 0) {
    return false;
}

// Extract the first three fields using your existing validation.
version = manifest.substring(0, firstNewline);

expectedHash = manifest.substring(
    firstNewline + 1, secondNewline
);

// Parse and validate the size field from the third line.
String sizeText = manifest.substring(
    secondNewline + 1, thirdNewline
);

// Keep your existing strict size validation here.

// Everything after the third newline is the release notes.
releaseNotes = manifest.substring(thirdNewline + 1);
```

This is the core extraction logic, not a complete replacement for your existing parser. In particular, check each newline position before using it to calculate a later search index. Keep your existing strict hash, size and line-ending validation.

### 3. Display release notes only when an update is available

Add the `releaseNotes` variable to your existing flow:

```cpp
String version, expectedHash, releaseNotes;
size_t expectedSize = 0;

// Verify the signature first.
if (!verifyManifestSignature(
        manifest, signature, sizeof(signature))) {
    Serial.println("OTA rejected: invalid signature");
    otaInProgress = false;
    return;
}

// Parse the signed metadata.
if (!parseManifest(manifest, version,
                   expectedHash, expectedSize,
                   releaseNotes)) {
    Serial.println("OTA rejected: malformed manifest");
    otaInProgress = false;
    return;
}

// Check whether the downloaded version is newer.
if (!isNewerVersion(version, currentVersion)) {
    Serial.println("No newer firmware available");
    otaInProgress = false;
    return;
}

// Only display release notes for a newer release.
Serial.println();
Serial.println("================================");
Serial.println(" New firmware update available");
Serial.printf(" Current version: %s\n", currentVersion.c_str());
Serial.printf(" New version:     %s\n", version.c_str());
Serial.println("--------------------------------");
Serial.println(" Release notes:");

if (releaseNotes.length() > 0) {
    Serial.println(releaseNotes);
} else {
    Serial.println("(No release notes provided)");
}

Serial.println("================================");
```

Here, `isNewerVersion()` represents your existing firmware-version comparison logic. Keep your actual function name and logic if they differ.

This placement ensures that the release notes are displayed only after:

- The manifest signature is valid.
- The metadata has passed validation.
- The firmware version is confirmed to be newer.

You can then continue to your existing firmware download and installation steps.

### 4. Two security details to preserve

- **Sign the release notes:** The signature must cover every byte in `version.txt`, including the release notes and newline characters. Sign the final file after all edits.
- **Limit manifest size:** Release notes increase the manifest size. Keep a reasonable maximum, such as your existing `MAX_MANIFEST_SIZE`, and reject oversized responses before processing them. Consider also limiting the release-notes length and filtering control characters if the notes are printed to the Serial Monitor.

**My recommendation:** keep the existing three required fields unchanged and append release notes after them. This minimizes changes to your working OTA implementation while allowing you to provide useful firmware update information.

---

# Protecting an OTA update is not the same as protecting your intellectual property (IP)

Your current ESP32-S3 OTA security work can help ensure that devices receive authentic firmware. However, it does not automatically prevent someone who obtains `firmware.bin` from copying your product, studying your implementation, or building a competing device.

Let's examine your two scenarios and the practical protections available on your ESP32-S3 N16R8.

## 1. Scenario A — Someone flashes your firmware.bin onto their own ESP32-S3

Suppose you sell a product containing an ESP32-S3, a display, sensors, and your proprietary application. Someone obtains the same firmware binary and buys compatible hardware.

| Protection mechanism | Can it stop this? | Explanation |
|---|---|---|
| HTTPS / TLS | No | Protects the download connection, not the downloaded file after release. |
| SHA-256 checksum | No | Detects a mismatch against a trusted digest; it does not prevent copying. |
| Signed OTA manifest | No, by itself | Authenticates the release metadata, but does not restrict who can run the firmware. |
| Secure Boot V2 | No, by itself | Enforces firmware-signing rules at boot. If the copied firmware is validly signed, another compatible device may accept it. |
| Flash Encryption | Sometimes helps | Makes flash contents harder to extract from a device, but does not prevent someone from using a firmware binary they already possess. |
| Device-bound authorization | Yes, if designed properly | Firmware can require proof that it is running on an authorized device before enabling protected features. |

The key point is that a valid signature answers **“Was this firmware authorized by the publisher?”** It does not answer **“Is this device licensed to run it?”**

## 2. Scenario B — Someone reverse-engineers firmware.bin

This is also possible. A compiled ESP32 application is machine code, not the original Arduino C++ source, but it can still reveal substantial implementation details.

| What an attacker does | What they may learn |
|---|---|
| Extract readable strings | API endpoints, debug messages, configuration names, sometimes credentials |
| Disassemble the binary | Machine instructions, function calls, control flow |
| Use a decompiler such as Ghidra | Approximate C-like pseudocode and application logic |
| Inspect constants and lookup tables | Algorithms, thresholds, calibration values, protocol details |
| Examine hardware interactions | Sensor interfaces, GPIO assignments, timing, communication protocols |

They generally will not recover your exact original source code, comments, variable names, or project structure unless additional debugging information is available. However, a sufficiently skilled engineer may reconstruct important algorithms and reproduce much of the product's behaviour.

### What can reduce this risk?

- Strip debugging information and avoid shipping debug builds. This removes helpful clues, although it does not prevent disassembly.
- Obfuscate selected logic. This can increase reverse-engineering effort, but obfuscation is not a security boundary.
- Keep your most valuable algorithms or decision-making on a server, if the product can tolerate the network dependency.
- Enable Flash Encryption in production mode to protect firmware stored in the device's external flash against straightforward physical readout. Espressif recommends combining it with Secure Boot; it does not protect a plaintext `.bin` that you have already distributed.

## 3. A stronger design: make the firmware both authentic and licensed

For a commercial ESP32-S3 product, I would separate the controls into three objectives:

### Objective A — Prevent unauthorized firmware

Enable Secure Boot V2 and sign your production firmware. Only software authorized by your signing key should be allowed to boot on a properly configured device.

### Objective B — Make firmware extraction harder

Use Flash Encryption in release mode, with appropriate production eFuse and debug-interface configuration. Keep signing keys and device provisioning secrets protected. This protects firmware stored on the device, not a binary already obtained by an attacker.

### Objective C — Restrict use to authorized products

Add device licensing or device-bound authorization. A copied firmware image should not automatically grant access to your paid product's protected functionality.

These are complementary controls, not interchangeable ones. Espressif's guidance recommends combining Secure Boot and Flash Encryption for production devices.

### How device-bound authorization could work

For example, imagine you manufacture 1,000 ESP32-S3 devices.

1. **Provision each device**  
   Assign a unique device identity and, where appropriate, a unique cryptographic key.

2. **Boot verified firmware**  
   Secure Boot checks that the firmware was signed by an authorized publisher.

3. **Verify the device's license**  
   The application authenticates with your licensing service or validates a device-specific authorization.

4. **Enable licensed features**  
   Unauthorized devices cannot automatically obtain the credentials or authorization needed for protected functions.

A few important details:

- Do not put one shared licensing secret into every firmware image. If extracted, that single secret could compromise the whole product line.
- For online licensing, authenticate devices using unique credentials or cryptographic keys and have your server issue authorization.
- For offline products, use device-specific signed licenses or cryptographic provisioning. These can make cloning harder, but offline enforcement is more difficult to defeat if an attacker can modify or emulate the application.
- Protect the license-checking code with Secure Boot and appropriate flash/debug protections. Otherwise, an attacker may patch the check to always return “licensed.”

#### Device-bound licensing design

If your ESP32-S3 product must operate completely offline, I recommend a **device-bound licensing design** rather than modifying the SHA-256 hash. You can make a copied `firmware.bin` much less useful on unauthorized hardware, although no purely software-based offline scheme can guarantee protection against a determined reverse engineer.

##### 1. Recommended design for your ESP32-S3 N16R8

- **Production firmware:** Secure Boot V2 + signed firmware + license verification
- **Device-specific license:** Contains the authorized device identity and a digital signature from your licensing private key
- **Offline verification at startup:** The ESP32 verifies the license signature and checks that it matches this device
- **Enable or deny protected features**

##### 2. How the licensing process works

For example, suppose you manufacture three products.

| Device | Device identity | License |
|---|---|---|
| Product A | `DEVICE-001` | Valid for A |
| Product B | `DEVICE-002` | Valid for B |
| Product C | `DEVICE-003` | Valid for C |

Your licensing tool generates a license for each product and signs it with a private key kept on your PC, not in the ESP32.

The firmware contains only the corresponding public key. At startup, it verifies the license signature and checks the device identity. Copying Product A's license to Product B should fail the identity check.

The license could contain fields such as:

```text
device_id = DEVICE-001
product_id = MY_PRODUCT
features = FULL
license_version = 1
```

The private key signs these fields. The ESP32 verifies that signature offline.

##### 3. Where should the device identity be stored?

You have three practical choices.

###### Option A — NVS / ordinary flash

Easy to implement, but the identity may be copied or modified by an attacker who can rewrite flash. Suitable for prototypes, not strong anti-cloning protection.

###### Option B — ESP32-S3 eFuse-backed identity

You can use an appropriately provisioned, device-specific eFuse value as part of the identity. eFuses are one-time-programmable, so plan and test provisioning carefully. A public identifier alone does not prove device authenticity; the design also needs a protected secret or another unforgeable property.

###### Option C — External secure element

A secure element can keep a unique private key inside dedicated hardware and perform cryptographic operations without exposing that key to the ESP32. This is generally the stronger choice when resistance to cloning and physical extraction is important, at additional hardware and integration cost.


##### 4. What happens if someone copies your firmware?

| Attacker's action | Expected result with the proposed design |
|---|---|
| Copies `firmware.bin` to another ESP32-S3 | Firmware may boot if correctly signed, but its license check should fail. |
| Copies the original device's license file | The device identity mismatch should cause rejection. |
| Changes the license fields | Digital signature verification should fail. |
| Reverse-engineers the binary | They may discover the verification logic; Secure Boot and Flash Encryption raise the difficulty but do not make reverse engineering impossible. |
| Patches the license check | A determined attacker may succeed unless the design includes stronger hardware-backed enforcement and protects the boot chain. |

**Important:** Secure Boot authenticates firmware; it does not enforce licensing by itself. Flash Encryption protects flash contents, not a firmware binary already obtained by an attacker.

##### 5. My recommendation

For your offline product, start with device-specific signed licenses, then decide whether the value of your IP justifies an external secure element. Do not use one shared secret embedded in every firmware image, and do not rely on a modified SHA-256 hash as the licensing mechanism.

Before choosing the implementation, one key question:




## 4. What I would do for your current project

Since you are developing with Arduino IDE and the ESP32-S3 N16R8, I would take this approach in stages:

| Stage | Action | Primary benefit |
|---|---|---|
| 1 | Keep TLS, signed manifest and SHA-256 verification | Authentic OTA releases |
| 2 | Sign firmware and plan for Secure Boot V2 | Prevent unauthorized firmware from booting on properly provisioned devices |
| 3 | Test Flash Encryption on a separate development board | Reduce physical firmware extraction |
| 4 | Add unique device identities and a licensing mechanism | Deter unauthorized use and mass cloning |
| 5 | Move especially valuable algorithms to a server, where practical | Reduce the amount of valuable logic exposed in the binary |

**Important:** do not enable release-mode Flash Encryption or burn security eFuses on your current working development board just to experiment. These operations can permanently restrict flashing, debugging and recovery. Validate the full production and recovery process on a separate board first.

## 5. The fundamental limitation

If an attacker obtains your plaintext `firmware.bin`, you cannot guarantee that they will be unable to study it. Nor can Secure Boot alone prevent someone from using a validly signed firmware image on another compatible device.

The realistic objective is to make copying harder, less useful, and more costly through layered protections, device-specific authorization, and appropriate legal IP protection. Copyright, patents, trade secrets and licensing agreements address different aspects of unauthorized copying; the right combination depends on the product and jurisdiction.

For your next design decision, the most important question is: **must your product operate completely offline, or can it connect to the internet periodically to verify its license?** That determines how robustly you can restrict unauthorized devices without compromising the user experience.

---
# Note: Install OpenSSL

For your ESP32-S3 OTA signing exercise, you need OpenSSL to generate an RSA private/public key pair and sign your manifest. Here are two ways to fix it.

## Option 1 — Install OpenSSL on Windows (recommended)

1. Visit the official OpenSSL project website: [OpenSSL.org](https://www.openssl.org/).
- Built Releases: https://slproweb.com/products/Win32OpenSSL.html
3. Open the Windows installation guidance or download page and choose a reputable Windows build. OpenSSL's project site provides links to third-party Windows distributions.
4. Run the installer. Note the installation folder, which may be similar to:

   ```text
   C:\Program Files\OpenSSL-Win64\bin
   ```

5. Add that `bin` folder to your Windows `PATH`:

   - Open the Windows Start menu and search for **Edit the system environment variables**.
   - Select **Environment Variables**.
   - Under your user variables, select **Path** → **Edit** → **New**.
   - Add the actual folder containing `openssl.exe`.
   - Click **OK** to close the dialogs.
   - Close and reopen Command Prompt.

6. Verify the installation in a new Command Prompt:

   ```cmd
   openssl version
   ```

   You should see a version string, for example `OpenSSL 3.x.x` or newer.

OpenSSL's Windows installer has a `PATH` adjustment option, so if you use that installer, enable it during installation.

## Option 2 — Use OpenSSL already included with Git for Windows

If you have Git installed, try this in Command Prompt:

```cmd
"C:\Program Files\Git\usr\bin\openssl.exe" version
```

If that works, you can run your key-generation command using the full executable path:

```cmd
"C:\Program Files\Git\usr\bin\openssl.exe" genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out test-private.pem
```

If Windows says the file cannot be found, use Option 1.

## Next: generate your OTA signing keys

Once OpenSSL works, run these commands from your intended working directory:

```cmd
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out test-private.pem
```

```cmd
openssl pkey -in test-private.pem -pubout -out test-public.pem
```

This creates:

- `test-private.pem` — keep this private; use it to sign release manifests.
- `test-public.pem` — embed its public key in your ESP32 firmware for signature verification.

For your initial test, use a dedicated test key pair. Never commit the private key to GitHub, even if the repository is private.

---

# Secure OTA Migration: GitHub → Cloudflare R2

I reviewed your attached OTA sketch. The good news is that **you do not need to rewrite your OTA updater**. Your existing code already has the important building blocks: validated HTTPS connections, RSA-2048/SHA-256 manifest verification, semantic version comparison, firmware-size checks, streaming to the inactive OTA partition, and firmware SHA-256 verification. Pasted text Pasted text Pasted text

The migration should primarily change **how the ESP32 obtains the update metadata and firmware URL**. We will retain your existing signature-verification and installation functions wherever possible.

One important detail from your sketch: `FIRMWARE_VERSION` is currently hard-coded as `1.0.3`, your RSA public key is embedded in the firmware, and your signed manifest contains the version, SHA-256 hash, image size and optional release notes. Pasted text Pasted text

## 1. Recommended architecture

I recommend **Cloudflare Worker authentication + a short-lived R2 presigned GET URL + your existing RSA-signed manifest**.

### Architecture diagram

```text
              DEVELOPER / RELEASE PROCESS
              ───────────────────────────
              1. Build firmware.bin
              2. Calculate firmware SHA-256
              3. Create version.txt
              4. Sign version.txt with RSA private key
                 (private key remains offline)
                          │
                          ▼
              ┌─────────────────────────┐
              │ Cloudflare R2           │
              │                         │
              │ firmware.bin            │
              │ version.txt             │
              │ version.sig             │
              └────────────┬────────────┘
                           │
                           │ Private R2 access
                           ▼
┌──────────────────┐  HTTPS  ┌──────────────────────────┐
│ ESP32-S3 N16R8   │ ──────► │ Cloudflare Worker        │
│                  │         │                          │
│ Device ID/token  │         │ 1. Authenticate device   │
│ Root CA trust    │         │ 2. Read signed manifest  │
│ RSA public key   │         │ 3. Generate presigned GET│
└────────┬─────────┘         └────────────┬─────────────┘
         │                                │
         │◄──── JSON response ────────────┘
         │      • version.txt contents
         │      • RSA signature (Base64)
         │      • presigned firmware URL
         │
         ▼
  ESP32 verifies RSA signature
  over the EXACT manifest bytes
         │
         ▼
  Parse version, SHA-256, size
  Reject invalid or non-newer release
         │
         ▼
  HTTPS GET to presigned R2 URL
         │
         ▼
  Existing Update.writeStream()
  and Update.setSHA256()
         │
         ▼
  Verify complete image and finalize
         │
         ▼
  Reboot into new firmware
```

### Why this is better than putting R2 credentials on the ESP32

Cloudflare R2 supports AWS Signature Version 4 presigned URLs. A presigned URL grants access to a specific object and operation for a limited time. It contains the authorization signature, so the ESP32 does not need the R2 access key or secret key. [Cloudflare Docs](https://developers.cloudflare.com/r2/api/s3/presigned-urls/)

The Worker authenticates the device before issuing the URL. The Worker holds the R2 credentials in its server-side secret store; the ESP32 receives only the short-lived download URL.

There are two distinct authentication decisions:

- **Worker authentication:** Is this device authorized to request an update?
- **Firmware authentication:** Was this release manifest signed by the publisher whose public key is embedded in the firmware?

Both are necessary. A presigned URL controls access to the file, but it does not prove that the firmware is an authentic release from you.

## 2. Threat model: what each security mechanism protects

| Mechanism | What it protects | What it does not guarantee |
|---|---|---|
| HTTPS with CA validation | Confidentiality and integrity in transit; authenticates the HTTPS server | That the server is publishing firmware you intended to release |
| Worker device authentication | Prevents unauthenticated clients from obtaining new presigned URLs | That a stolen device credential cannot be reused |
| R2 presigned GET URL | Authorizes a specific object download until expiry | That the downloaded binary is authentic or safe |
| RSA signature on `version.txt` | Authenticates the publisher's signed metadata | That the firmware itself has no vulnerabilities |
| SHA-256 hash in the signed manifest | Binds the firmware image to authenticated metadata | Publisher authenticity if the hash is not authenticated |
| Version comparison | Prevents ordinary accidental downgrades | Hardware-enforced anti-rollback |

Cloudflare explicitly warns that presigned URLs are bearer tokens: anyone who obtains the URL can use it for the authorized operation until it expires. [Cloudflare Docs](https://developers.cloudflare.com/r2/api/s3/presigned-urls/)

### Why keep your RSA manifest signature?

Suppose an attacker obtains access to your R2 account or compromises the Worker. The attacker might replace `firmware.bin`, or manipulate the metadata response.

If the ESP32 trusts only a SHA-256 hash supplied in the Worker JSON, the attacker could replace both the firmware and its hash.

Your current design prevents this: the ESP32 verifies the manifest using the embedded RSA public key before trusting its version, hash and size. The attacker cannot generate a valid signature for modified metadata without the corresponding private key.

**Keep your RSA private key offline. Never place it in the Worker or ESP32.** The Worker needs the R2 credentials to generate download URLs, but it does not need the RSA signing key.

### Important distinction about firmware verification

Your existing implementation calls `Update.setSHA256(expectedHash.c_str())` before streaming the firmware. It writes to the inactive OTA partition, then finalizes the update only if the expected number of bytes arrived and the hash check succeeds. Pasted text

That is a reasonable authenticated-OTA design: the *expected* hash is authenticated before the download, and the image is checked before the update is finalized.

It is not the same as downloading the entire firmware and verifying a separate signature over the binary before writing any bytes. We can retain your existing approach without buffering a roughly 1 MB firmware image in PSRAM.

---

## 3. Cloudflare R2 setup: step by step

### Step 1 — Create a private R2 bucket

1. Sign in to the [Cloudflare](https://www.cloudflare.com/) dashboard.
2. Open **Storage & databases → R2 Object Storage**.
3. Select **Create bucket**.
4. Choose a name, for example `esp32-firmware`.
5. Choose an appropriate location and create the bucket.
6. Keep it **private**. Do not enable public access merely to make downloads work.

Cloudflare's bucket creation documentation is available here: [Create an R2 bucket](https://developers.cloudflare.com/r2/buckets/create-buckets/).

### Step 2 — Upload the release files

Upload these objects to the bucket:

```text
esp32-firmware/
├── firmware.bin
├── version.txt
└── version.sig
```

The `version.txt` format should remain compatible with your existing parser:

```text
1.0.4
<64-character SHA-256 hash>
1039536
Release notes:
- Fix release-note display.
- Improve OTA handling.
```

Your signature must cover the **exact bytes of `version.txt`**, including newline characters. If you change the file, regenerate `version.sig`.

For the current sketch, the signature is a raw 256-byte RSA-2048 signature. The Worker below converts it to Base64 for transport inside JSON; the ESP32 decodes it back to binary before calling your existing verification function.

### Step 3 — Create an R2 S3 API token

The Worker will use the AWS SDK to generate a presigned GET URL.

1. In the Cloudflare dashboard, open **R2 Object Storage → Overview**.
2. Under **Account Details**, select **Manage** beside API Tokens.
3. Create an R2 API token.
4. Choose **Object Read only**.
5. Scope it to the `esp32-firmware` bucket if the interface offers that option.
6. Create the token.
7. Copy the **Access Key ID** and **Secret Access Key** into a secure password manager.

Do not paste these credentials into Arduino code, commit them to GitHub, or include them in the Worker source file. Cloudflare's official instructions describe the token permissions and S3 endpoint. [Cloudflare Docs](https://developers.cloudflare.com/r2/api/tokens/)

### Step 4 — Configure a five-minute presigned URL

Use an expiry of **300 seconds** initially.

Five minutes is a reasonable starting point for a device that requests the URL and immediately begins downloading a roughly 1 MB image. The expiry applies to when requests can be authorized; it is not a guarantee that a download already in progress will be forcibly terminated at the five-minute mark.

If your network is slow, test this value and increase it if necessary. The ESP32 should request a fresh URL after an expiry-related HTTP error.

The URL will look roughly like this:

```text
https://<bucket>.<account-id>.r2.cloudflarestorage.com/firmware.bin
    ?X-Amz-Algorithm=AWS4-HMAC-SHA256
    &X-Amz-Credential=...
    &X-Amz-Date=...
    &X-Amz-Expires=300
    &X-Amz-Signature=...
```

Use the URL exactly as returned by the Worker. Do not trim it, decode its query parameters, or rebuild it manually.

**Do not use an R2 custom domain for this presigned URL.** Cloudflare documents that presigned URLs work with the S3 API endpoint and not with custom domains. You can use a custom domain for your Worker API if desired, but the signed R2 download URL should use the S3 API domain. [Cloudflare Docs](https://developers.cloudflare.com/r2/api/s3/presigned-urls/)

### Step 5 — CORS configuration

For a native ESP32 using `HTTPClient`, **CORS is not required**. CORS is a browser-enforced policy; it does not provide authentication for your ESP32.

If you also plan to download firmware from a browser-based dashboard, configure CORS for that web application's exact origin and the required methods. See [Cloudflare's R2 CORS documentation](https://developers.cloudflare.com/r2/buckets/cors/?ref=arabroot.io).

---

## 4. Backend: Cloudflare Worker

We'll implement a Worker that:

1. Accepts an HTTPS POST from an ESP32.
2. Validates its device ID and bearer token.
3. Reads `version.txt` and `version.sig` from the private R2 bucket using a Worker R2 binding.
4. Validates the metadata's basic structure.
5. Generates a five-minute presigned GET URL for `firmware.bin`.
6. Returns the URL, exact manifest text, signature in Base64, and convenient metadata fields.

**Why return the signed manifest as well as the URL?** Your ESP32 must authenticate the release metadata before it trusts the hash, version and size. Returning only `sha256` and `version` in JSON would not authenticate those values.

### 4.1 Create the Worker project

Install Node.js if needed, then open a terminal and run:

```bash
npm create cloudflare@latest esp32-ota-auth
```

Choose a Worker project using TypeScript. Inside the project directory, install the required packages:

```bash
cd esp32-ota-auth
npm install @aws-sdk/client-s3 @aws-sdk/s3-request-presigner
```

The correct package for presigned GET URLs is `@aws-sdk/s3-request-presigner`. The `@aws-sdk/s3-presigned-post` package is for presigned POST form uploads, not this GET-download workflow. [AWS Documentation](https://docs.aws.amazon.com/AWSJavaScriptSDK/v3/latest/Package/-aws-sdk-s3-request-presigner/)

### 4.2 Configure the Worker

In `wrangler.jsonc`, configure the Worker and bind the private bucket. Adapt the configuration generated by your installed Wrangler version if its schema differs.

```jsonc
{
  "$schema": "./node_modules/wrangler/config-schema.json",
  "name": "esp32-ota-auth",
  "main": "src/index.ts",
  "compatibility_date": "2026-10-23",
  "compatibility_flags": ["nodejs_compat"],
  "r2_buckets": [
    {
      "binding": "OTA_BUCKET",
      "bucket_name": "esp32-firmware"
    }
  ]
}
```

The `OTA_BUCKET` binding lets the Worker read the release metadata from R2 without making the bucket public. The AWS SDK credentials are used separately to generate the presigned URL.

### 4.3 Store the secrets

Set these secrets from the terminal:

```bash
npx wrangler secret put R2_ACCOUNT_ID
npx wrangler secret put R2_ACCESS_KEY_ID
npx wrangler secret put R2_SECRET_ACCESS_KEY
npx wrangler secret put DEVICE_TOKENS_JSON
```

For the last secret, enter JSON mapping device IDs to individual bearer tokens, for example:

```json
{
  "esp32-lab-001": "REPLACE_WITH_A_LONG_RANDOM_DEVICE_TOKEN",
  "esp32-lab-002": "REPLACE_WITH_ANOTHER_LONG_RANDOM_TOKEN"
}
```

Use different random tokens for different devices. These are example values; generate your own.

Cloudflare Workers secrets are encrypted and exposed to the Worker through its environment. Do not put sensitive values in the ordinary `vars` section or commit them to source control. [Cloudflare Docs](https://developers.cloudflare.com/workers/configuration/secrets/)

For a production fleet, prefer a proper per-device credential lifecycle, with revocation, rotation and device provisioning. A bearer token embedded in ESP32 firmware can be extracted by someone with physical access to the device; it is better than exposing R2 credentials, but it is not equivalent to hardware-backed device identity.

### 4.4 Complete Worker implementation

Replace `src/index.ts` with the following example. It uses a fixed firmware object name so the device cannot ask the Worker to sign arbitrary R2 object paths.

```typescript
import {
  S3Client,
  GetObjectCommand,
} from "@aws-sdk/client-s3";

import { getSignedUrl } from "@aws-sdk/s3-request-presigner";

interface Env {
  OTA_BUCKET: R2Bucket;

  R2_ACCOUNT_ID: string;
  R2_ACCESS_KEY_ID: string;
  R2_SECRET_ACCESS_KEY: string;

  // JSON object mapping device ID -> unique bearer token.
  DEVICE_TOKENS_JSON: string;
}

const BUCKET_NAME = "esp32-firmware";
const MANIFEST_KEY = "version.txt";
const SIGNATURE_KEY = "version.sig";
const FIRMWARE_KEY = "firmware.bin";

const URL_EXPIRY_SECONDS = 300;
const MAX_MANIFEST_BYTES = 512;
const RSA_SIGNATURE_BYTES = 256;

function jsonResponse(
  data: unknown,
  status = 200,
): Response {
  return new Response(JSON.stringify(data), {
    status,
    headers: {
      "Content-Type": "application/json; charset=utf-8",
      "Cache-Control": "no-store",
      "Pragma": "no-cache",
    },
  });
}

// Compare token bytes without returning at the first mismatch.
function constantTimeEqual(a: string, b: string): boolean {
  const encoder = new TextEncoder();
  const aa = encoder.encode(a);
  const bb = encoder.encode(b);

  let difference = aa.length ^ bb.length;
  const length = Math.max(aa.length, bb.length);

  for (let i = 0; i < length; i++) {
    difference |= (aa[i] ?? 0) ^ (bb[i] ?? 0);
  }

  return difference === 0;
}

function validateManifest(manifest: string) {
  // This sample expects LF line endings and a newline after the
  // size field, as required by the existing RELEASE_LOG parser.
  if (manifest.includes("\r")) {
    throw new Error("Manifest must use LF line endings");
  }

  if (new TextEncoder().encode(manifest).length > MAX_MANIFEST_BYTES) {
    throw new Error("Manifest too large");
  }

  const p1 = manifest.indexOf("\n");
  const p2 = p1 >= 0 ? manifest.indexOf("\n", p1 + 1) : -1;
  const p3 = p2 >= 0 ? manifest.indexOf("\n", p2 + 1) : -1;

  if (p1 <= 0 || p2 <= p1 || p3 <= p2) {
    throw new Error("Malformed manifest");
  }

  const version = manifest.slice(0, p1);
  const sha256 = manifest.slice(p1 + 1, p2);
  const sizeText = manifest.slice(p2 + 1, p3);

  if (!/^\d+\.\d+\.\d+$/.test(version)) {
    throw new Error("Invalid version");
  }

  if (!/^[0-9a-fA-F]{64}$/.test(sha256)) {
    throw new Error("Invalid SHA-256");
  }

  if (!/^\d+$/.test(sizeText)) {
    throw new Error("Invalid firmware size");
  }

  const size = Number(sizeText);

  // Keep aligned with the ESP32 MAX_FIRMWARE_SIZE and partition.
  if (!Number.isSafeInteger(size) || size <= 0 || size > 4 * 1024 * 1024) {
    throw new Error("Firmware size out of range");
  }

  return {
    version,
    sha256: sha256.toLowerCase(),
    size,
  };
}

export default {
  async fetch(request: Request, env: Env): Promise<Response> {
    if (request.method !== "POST") {
      return jsonResponse({ error: "POST required" }, 405);
    }

    const url = new URL(request.url);

    if (url.pathname !== "/v1/ota") {
      return jsonResponse({ error: "Not found" }, 404);
    }

    // 1. Authenticate the device.
    const authorization = request.headers.get("Authorization") ?? "";
    const match = authorization.match(/^Bearer (.+)$/);

    if (!match) {
      return jsonResponse({ error: "Unauthorized" }, 401);
    }

    let requestBody: { deviceId?: string };

    try {
      requestBody = await request.json() as { deviceId?: string };
    } catch {
      return jsonResponse({ error: "Invalid JSON" }, 400);
    }

    const deviceId = requestBody.deviceId;

    if (
      typeof deviceId !== "string" ||
      !/^[a-zA-Z0-9_-]{1,64}$/.test(deviceId)
    ) {
      return jsonResponse({ error: "Invalid device ID" }, 400);
    }

    let deviceTokens: Record<string, string>;

    try {
      deviceTokens = JSON.parse(env.DEVICE_TOKENS_JSON);
    } catch {
      return jsonResponse({ error: "Server configuration error" }, 500);
    }

    const expectedToken = deviceTokens[deviceId];

    if (
      typeof expectedToken !== "string" ||
      !constantTimeEqual(match[1], expectedToken)
    ) {
      return jsonResponse({ error: "Unauthorized" }, 401);
    }

    try {
      // 2. Read the exact manifest bytes from private R2.
      const manifestObject = await env.OTA_BUCKET.get(MANIFEST_KEY);
      const signatureObject = await env.OTA_BUCKET.get(SIGNATURE_KEY);

      if (!manifestObject || !signatureObject) {
        return jsonResponse({ error: "Release metadata unavailable" }, 503);
      }

      const manifestBytes = await manifestObject.arrayBuffer();
      const signatureBytes = await signatureObject.arrayBuffer();

      if (
        manifestBytes.byteLength === 0 ||
        manifestBytes.byteLength > MAX_MANIFEST_BYTES
      ) {
        return jsonResponse({ error: "Manifest size invalid" }, 500);
      }

      if (signatureBytes.byteLength !== RSA_SIGNATURE_BYTES) {
        return jsonResponse({ error: "Signature size invalid" }, 500);
      }

      const manifest = new TextDecoder("utf-8", { fatal: true })
        .decode(manifestBytes);

      // Check basic syntax only. The ESP32 verifies the RSA signature.
      const metadata = validateManifest(manifest);

      // 3. Generate a GET URL for this fixed object only.
      const s3 = new S3Client({
        region: "auto",
        endpoint:
          `https://${env.R2_ACCOUNT_ID}.r2.cloudflarestorage.com`,
        credentials: {
          accessKeyId: env.R2_ACCESS_KEY_ID,
          secretAccessKey: env.R2_SECRET_ACCESS_KEY,
        },
      });

      const firmwareUrl = await getSignedUrl(
        s3,
        new GetObjectCommand({
          Bucket: BUCKET_NAME,
          Key: FIRMWARE_KEY,
        }),
        { expiresIn: URL_EXPIRY_SECONDS },
      );

      // 4. Encode the raw 256-byte signature for JSON transport.
      const bytes = new Uint8Array(signatureBytes);
      let binary = "";

      for (const byte of bytes) {
        binary += String.fromCharCode(byte);
      }

      const signatureB64 = btoa(binary);

      return jsonResponse({
        deviceId,
        url: firmwareUrl,
        manifest,
        signatureB64,
        version: metadata.version,
        sha256: metadata.sha256,
        size: metadata.size,
        expiresIn: URL_EXPIRY_SECONDS,
      });
    } catch (error) {
      console.error("OTA authorization error", error);
      return jsonResponse({ error: "Unable to prepare OTA release" }, 500);
    }
  },
};
```

**Important implementation notes**

- The Worker validates manifest syntax but does not replace the ESP32's RSA verification. The device remains the final authority on whether the signed metadata is trusted.
- The Worker signs a URL for `firmware.bin` only. It does not accept an arbitrary bucket or object key from the client.
- The example uses a read-only R2 token and does not publish the bucket.
- The code assumes `version.txt` follows the LF-only format expected by your existing `RELEASE_LOG` parser, including a newline after the firmware size.
- The sample's 4 MiB size limit matches your current `MAX_FIRMWARE_SIZE`; keep it aligned with the actual OTA partition.
- The code uses the AWS SDK in a Worker. If your generated project has trouble bundling the SDK, check the Wrangler compatibility configuration and the Cloudflare/AWS SDK versions rather than replacing the firmware OTA logic.

### 4.5 Deploy the Worker

After adding the code and secrets:

```bash
npx wrangler deploy
```

The deployment output will show a Worker URL similar to:

```text
https://esp32-ota-auth.<your-subdomain>.workers.dev
```

Your ESP32 will call that URL at the `/v1/ota` path. You can later configure a custom domain for the Worker endpoint if you own a domain and want a stable API hostname.

---

## 5. ESP32-S3 integration: minimal changes to your existing sketch

We'll keep the following functions from your current code:

- `verifyManifestSignature()`
- `parseManifest()`
- `isNewerVersion()`
- The existing OTA streaming, SHA-256 verification, and `Update` finalization logic

The main change is that `checkForFirmwareUpdate()` will call the Worker to obtain authenticated release metadata and a presigned firmware URL, rather than fetching three separate files from GitHub.

### 5.1 Add the ArduinoJson and Base64 includes

Add these beside your existing includes:

```cpp
#include <ArduinoJson.h>
#include <mbedtls/base64.h>
```

Your sketch already includes the mbedTLS public-key and SHA-256 headers needed by the RSA verification function. Pasted text

### 5.2 Add the Worker configuration

Add these to your user configuration:

```cpp
static const char OTA_AUTH_URL[] =
    "https://esp32-ota-auth.YOUR-SUBDOMAIN.workers.dev/v1/ota";

static const char DEVICE_ID[] = "esp32-lab-001";

// A unique random token issued for this device.
// This is NOT an R2 access key or R2 secret key.
static const char DEVICE_TOKEN[] =
    "REPLACE_WITH_THIS_DEVICE_RANDOM_TOKEN";
```

The device token must match the token registered for `esp32-lab-001` in the Worker's `DEVICE_TOKENS_JSON` secret.

**Do not use your R2 Secret Access Key as `DEVICE_TOKEN`.**

For a production fleet, provision a unique token per device and make it revocable. A static bearer token is a practical starting point for your lab, but a person who extracts it from a device can reuse it until it is revoked.

### 5.3 Configure CA certificates for both HTTPS endpoints

Your current sketch's `ROOT_CA` comment says DigiCert Global Root G2, but the PEM content in the attached file is actually **ISRG Root X1**. Do not assume that certificate is the correct trust anchor for your new Worker and R2 endpoints. Pasted text

Inspect the actual certificate chains presented by:

1. Your Worker hostname.
2. The R2 S3 endpoint returned in the presigned URL.

Use a verified root CA or CA bundle that validates each hostname's current certificate chain. If they require different roots, define separate CA constants. Do not simply copy an arbitrary server certificate and treat it as a root CA.

For clarity, the code below uses two separate names:

```cpp
// Replace these placeholders with verified PEM root CA material
// for the actual TLS chains used by your Worker and R2 endpoint.
static const char WORKER_ROOT_CA[] PROGMEM = R"PEM(
-----BEGIN CERTIFICATE-----
PASTE_VERIFIED_WORKER_ROOT_CA_HERE
-----END CERTIFICATE-----
)PEM";

static const char R2_ROOT_CA[] PROGMEM = R"PEM(
-----BEGIN CERTIFICATE-----
PASTE_VERIFIED_R2_ROOT_CA_HERE
-----END CERTIFICATE-----
)PEM";
```

These are **placeholders, not usable certificates**. Replace them before compiling. If both chains can be validated by the same trusted CA material, you can use one shared constant instead.

Keep using `WiFiClientSecure::setCACert()`; do not use `setInsecure()`.

### 5.4 Add storage for authenticated metadata

Your current OTA function receives `expectedHash` and `expectedSize` as parameters. We'll preserve that design and use output parameters to transfer the validated values from the Worker response.

Add these declarations in the secure-OTA section:

```cpp
String otaExpectedHash;
size_t otaExpectedSize = 0;
String otaReleaseVersion;
```

### 5.5 Implement `getSecureR2DownloadUrl()`

You requested a function returning the presigned URL. It will also verify the signed manifest and populate the expected hash, size and version variables.

That is intentional: **the function must not return a usable download URL to the OTA flow until the release metadata has passed signature and version validation.**

This implementation uses your existing `verifyManifestSignature()`, `parseManifest()` and `isNewerVersion()` functions.

```cpp
String getSecureR2DownloadUrl() {
  otaExpectedHash = "";
  otaExpectedSize = 0;
  otaReleaseVersion = "";

  WiFiClientSecure client;
  client.setCACert(WORKER_ROOT_CA);
  client.setHandshakeTimeout(15);

  HTTPClient http;
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);

  if (!http.begin(client, OTA_AUTH_URL)) {
    Serial.println("Worker HTTPS begin failed");
    return "";
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader(
      "Authorization",
      String("Bearer ") + DEVICE_TOKEN);

  // The device ID selects the corresponding per-device token entry.
  String requestBody =
      String("{\"deviceId\":\"") + DEVICE_ID + "\"}";

  Serial.println("Requesting authenticated OTA metadata...");

  int status = http.POST(requestBody);

  if (status != HTTP_CODE_OK) {
    Serial.printf("Worker request failed: HTTP=%d\n", status);
    http.end();
    return "";
  }

  int responseLength = http.getSize();

  // Bound JSON response size. A manifest is small; the firmware is
  // downloaded separately and is not buffered in this JSON response.
  if (responseLength > 4096) {
    Serial.println("Worker JSON response too large");
    http.end();
    return "";
  }

  String response = http.getString();
  http.end();

  if (response.length() == 0 || response.length() > 4096) {
    Serial.println("Invalid Worker response length");
    return "";
  }

  // ArduinoJson 6-compatible declaration; ArduinoJson 7 also supports
  // DynamicJsonDocument for compatibility.
  DynamicJsonDocument doc(4096);

  DeserializationError jsonError = deserializeJson(doc, response);

  if (jsonError) {
    Serial.printf("Worker JSON parse failed: %s\n",
                  jsonError.c_str());
    return "";
  }

  const char* manifestText = doc["manifest"];
  const char* signatureB64 = doc["signatureB64"];
  const char* firmwareUrl = doc["url"];

  if (manifestText == nullptr ||
      signatureB64 == nullptr ||
      firmwareUrl == nullptr) {
    Serial.println("Worker response missing required fields");
    return "";
  }

  String manifest = manifestText;
  String encodedSignature = signatureB64;
  String url = firmwareUrl;

  // Accept only the intended HTTPS R2 S3 endpoint and object path.
  // The exact presigned query string is retained unchanged.
  if (!url.startsWith("https://") ||
      url.indexOf(".r2.cloudflarestorage.com/firmware.bin?") < 0) {
    Serial.println("Rejected unexpected firmware URL");
    return "";
  }

  // Decode the raw RSA-2048 signature from Base64.
  uint8_t signature[RSA_SIGNATURE_SIZE];
  size_t decodedLength = 0;

  int rc = mbedtls_base64_decode(
      signature,
      sizeof(signature),
      &decodedLength,
      (const unsigned char*)encodedSignature.c_str(),
      encodedSignature.length());

  if (rc != 0 || decodedLength != RSA_SIGNATURE_SIZE) {
    Serial.println("Invalid Base64 RSA signature");
    return "";
  }

  // Authenticate the exact manifest bytes before trusting its fields.
  Serial.println("Verifying signed release manifest...");

  if (!verifyManifestSignature(
          manifest, signature, decodedLength)) {
    Serial.println("Rejected: manifest signature invalid");
    return "";
  }

  String version;
  String hash;
  size_t firmwareSize = 0;

#ifdef RELEASE_LOG
  String releaseNotes;

  if (!parseManifest(
          manifest, version, hash, firmwareSize, releaseNotes)) {
#else
  if (!parseManifest(
          manifest, version, hash, firmwareSize)) {
#endif
    Serial.println("Rejected: malformed signed manifest");
    return "";
  }

  // The release version must be strictly newer than the firmware
  // currently running on this device.
  if (!isNewerVersion(version, FIRMWARE_VERSION)) {
    Serial.println("No newer firmware release");
    return "";
  }

  // Cross-check the convenience fields returned by the Worker.
  // The signed manifest remains the authoritative source.
  const char* jsonVersion = doc["version"];
  const char* jsonHash = doc["sha256"];
  size_t jsonSize = doc["size"] | (size_t)0;

  if (jsonVersion == nullptr ||
      jsonHash == nullptr ||
      version != jsonVersion ||
      !hash.equalsIgnoreCase(jsonHash) ||
      firmwareSize != jsonSize) {
    Serial.println("Worker metadata does not match signed manifest");
    return "";
  }

#ifdef RELEASE_LOG
  Serial.printf("Release notes:\n%s\n", releaseNotes.c_str());
#endif

  otaReleaseVersion = version;
  otaExpectedHash = hash;
  otaExpectedSize = firmwareSize;

  Serial.printf("Authenticated release: %s\n",
                otaReleaseVersion.c_str());
  Serial.printf("Expected size: %u bytes\n",
                (unsigned)otaExpectedSize);

  return url;
}
```

#### Why verify before using the URL?

The URL itself is not proof of firmware authenticity. The device validates the RSA signature on the exact manifest before accepting the signed version, hash and size. It then checks that the convenience metadata in the JSON agrees with the signed fields.

The firmware URL is accepted only if it points to the intended HTTPS R2 S3 endpoint and `firmware.bin` path. This prevents the Worker response from silently redirecting the existing OTA logic to an arbitrary HTTPS host.

**Do not print the complete presigned URL to Serial in production.** Its query string is a temporary bearer credential.

#### One parser detail

Your current `RELEASE_LOG` version of `parseManifest()` requires the third newline and extracts the firmware size between the second and third newlines. Keep the manifest format consistent with that parser. Pasted text

### 5.6 Minimal change to your existing `downloadAndInstall()`

Your current function hard-codes `FIRMWARE_URL`. We only need to pass the newly obtained URL as a parameter.

**Before:**

```cpp
bool downloadAndInstall(const String& expectedHash,
                        size_t expectedSize) {
```

**After:**

```cpp
bool downloadAndInstall(const String& firmwareUrl,
                        const String& expectedHash,
                        size_t expectedSize) {
```

Inside this function, use the R2 CA material and supplied URL.

**Before:**

```cpp
WiFiClientSecure client;
client.setCACert(ROOT_CA);
client.setHandshakeTimeout(15);
```

**After:**

```cpp
WiFiClientSecure client;
client.setCACert(R2_ROOT_CA);
client.setHandshakeTimeout(15);
```

**Before:**

```cpp
if (!http.begin(client, FIRMWARE_URL)) {
```

**After:**

```cpp
if (!http.begin(client, firmwareUrl)) {
```

Everything else in `downloadAndInstall()` can remain unchanged initially, including:

```cpp
if (status != HTTP_CODE_OK ||
    contentLength < 0 ||
    (size_t)contentLength != expectedSize) {
  Serial.printf("Firmware rejected: HTTP=%d size=%d\n",
                status, contentLength);
  http.end();
  return false;
}

if (!Update.begin(expectedSize, U_FLASH)) {
  Serial.printf("Update.begin failed: %s\n",
                Update.errorString());
  http.end();
  return false;
}

if (!Update.setSHA256(expectedHash.c_str())) {
  Serial.println("Could not configure firmware SHA-256");
  Update.abort();
  http.end();
  return false;
}
```

Your existing `Update.writeStream()`, written-byte count check, `Update.end()` and reboot logic should also remain in place. These are the core installation checks we want to preserve. Pasted text

### 5.7 Replace the metadata-fetching part of `checkForFirmwareUpdate()`

Currently, `checkForFirmwareUpdate()` fetches the manifest and signature from separate URLs, verifies the signature, parses the manifest and then calls `downloadAndInstall()`. Pasted text

Replace that metadata-fetching sequence with this:

```cpp
void checkForFirmwareUpdate() {
  if (otaInProgress) return;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("OTA check skipped: Wi-Fi disconnected");
    return;
  }

  otaInProgress = true;

  Serial.println("\nStarting authenticated OTA");

  // 1. Authenticate to Worker, verify signed metadata,
  //    and obtain the short-lived R2 URL.
  String firmwareUrl = getSecureR2DownloadUrl();

  if (firmwareUrl.isEmpty()) {
    Serial.println("OTA check failed or no newer release");
    otaInProgress = false;
    return;
  }

  // 2. Download the firmware using the existing OTA logic.
  //    The URL is temporary; the hash and size came from the
  //    authenticated manifest.
  bool success = downloadAndInstall(
      firmwareUrl,
      otaExpectedHash,
      otaExpectedSize);

  if (!success) {
    Serial.println("OTA installation failed; current firmware retained");
  }

  // A successful installation normally restarts the device.
  otaInProgress = false;
}
```

This is the central migration: **one authenticated metadata request, followed by one firmware download**.

Remove the old manifest/signature fetching code from this function once the replacement compiles and passes testing. You can retain the old functions temporarily for comparison during development.

---

## 6. Test the Worker before testing the ESP32

Testing with `curl` helps separate Worker authentication issues from TLS and parsing issues on the device.

### Test A — Request update metadata

Run the following in a terminal, replacing the URL and token:

```bash
curl -i \
  -X POST \
  "https://esp32-ota-auth.YOUR-SUBDOMAIN.workers.dev/v1/ota" \
  -H "Authorization: Bearer YOUR_DEVICE_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"deviceId":"esp32-lab-001"}'
```

Expected result: HTTP `200` and JSON containing fields similar to:

```json
{
  "deviceId": "esp32-lab-001",
  "url": "https://YOUR-BUCKET.YOUR-ACCOUNT.r2.cloudflarestorage.com/firmware.bin?...",
  "manifest": "1.0.4\n...",
  "signatureB64": "...",
  "version": "1.0.4",
  "sha256": "...",
  "size": 1039536,
  "expiresIn": 300
}
```

The displayed values are illustrative, not real credentials or a valid presigned URL.

Test the failure cases too:

| Test | Expected result |
|---|---|
| No bearer token | HTTP 401 |
| Incorrect token | HTTP 401 |
| Unknown device ID | HTTP 401 |
| Missing metadata in R2 | HTTP 503 |
| Invalid manifest format | HTTP 500 |
| Valid device and release | HTTP 200 |

### Test B — Download using the presigned URL

Save the successful response as `response.json`, then extract the URL using `jq`:

```bash
URL=$(jq -r '.url' response.json)

curl --fail --show-error \
  "$URL" \
  -o firmware.bin
```

Verify the downloaded size and SHA-256 hash against the manifest. For example, on Windows PowerShell:

```powershell
(Get-Item .\firmware.bin).Length

Get-FileHash .\firmware.bin -Algorithm SHA256
```

The hash must match the signed manifest's SHA-256 field.

Do not share the output JSON or complete presigned URL publicly. Anyone who obtains a valid URL can use it until it expires.

### Test C — Test URL expiry

1. Request a new URL from the Worker.
2. Download the file immediately.
3. Request another URL and wait more than five minutes.
4. Try downloading through the expired URL.
5. Confirm that the expired request is rejected, then request a fresh URL.

Cloudflare documents expired presigned URL requests as returning an authorization error. [Cloudflare Docs](https://developers.cloudflare.com/r2/buckets/cors/?ref=arabroot.io\&utm_source=chatgpt.com)

---

## 7. ESP32 integration test sequence

Once the `curl` tests pass, upload the updated sketch over USB or your existing working OTA method.

Watch for this sequence:

```text
Starting authenticated OTA
Requesting authenticated OTA metadata...
Verifying signed release manifest...
Manifest signature valid
Authenticated release: 1.0.4
Expected size: 1039536 bytes
Streaming firmware to OTA partition...
Firmware hash verified; OTA finalized.
Restarting into the new firmware...
```

The exact output depends on the diagnostics in your existing code and the updated helper function.

After the reboot, confirm:

1. The device reports firmware version `1.0.4`.
2. Wi-Fi reconnects.
3. The next OTA check does not reinstall `1.0.4`.
4. A modified manifest with the old signature is rejected.
5. A modified firmware binary fails its SHA-256 check and is not finalized.
6. A Worker request with invalid credentials is rejected.
7. An expired presigned URL is not silently replaced by an unauthenticated or insecure download.

**One operational detail:** your current sketch checks for updates every two minutes, not every six hours, because the two-minute `OTA_CHECK_INTERVAL_MS` definition is active. Increase the interval after testing to reduce unnecessary authentication requests and avoid repeatedly checking the same release. Pasted text

---

## 8. Optional: configure a custom domain for the Worker

A custom domain is useful for a stable authentication API, such as `ota.example.com`. It is **not required** for presigned R2 downloads.

If you own a domain managed by Cloudflare:

1. Open **Workers & Pages** in the Cloudflare dashboard.
2. Select your `esp32-ota-auth` Worker.
3. Open its **Settings** or **Domains & Routes** section; the exact navigation can vary with the dashboard version.
4. Add a custom domain, such as `ota.example.com`.
5. Follow the DNS and certificate setup prompts.
6. Confirm that HTTPS works.
7. Change `OTA_AUTH_URL` in your ESP32 sketch to:

```cpp
static const char OTA_AUTH_URL[] =
    "https://ota.example.com/v1/ota";
```

8. Validate the domain's certificate chain and configure the appropriate root CA on the ESP32.

Keep the firmware download URL generated by the Worker pointing to the R2 S3 API endpoint. Do not replace it with the custom domain.

---

## 9. Before versus after: migration summary

| Component | Before: GitHub | After: Cloudflare R2 |
|---|---|---|
| Device authentication | No separate device-authentication endpoint | Worker validates device credentials |
| Manifest retrieval | `MANIFEST_URL` | Manifest returned by Worker |
| RSA signature retrieval | `SIGNATURE_URL` | Signature returned as Base64 JSON |
| Firmware URL | Hard-coded `FIRMWARE_URL` | Short-lived presigned GET URL |
| R2 credentials | Not applicable | Worker secrets only |
| TLS | Existing CA validation | CA validation for Worker and R2 |
| Manifest verification | Existing RSA verification | **Unchanged** |
| Version comparison | Existing version logic | **Unchanged** |
| Firmware download | `HTTPClient` streaming | Same streaming logic, supplied URL |
| Firmware hash check | `Update.setSHA256()` | **Unchanged** |
| OTA partition and reboot | Existing `Update` logic | **Unchanged** |

### What not to change

Do not remove or weaken these protections while migrating:

- `WiFiClientSecure` with `setCACert()`.
- RSA signature verification before trusting the manifest.
- Strict version and metadata validation.
- Firmware size and OTA partition capacity checks.
- `Update.setSHA256()` before writing the firmware stream.
- The check that the expected number of firmware bytes was written.
- Successful finalization before reboot.

Also keep the RSA private key offline and do not expose the R2 Secret Access Key to the device.

### One improvement I recommend after the first successful migration

Your current RSA-signed manifest design is already useful, but the next hardening step should be **anti-rollback protection**.

Your `isNewerVersion()` comparison prevents an ordinary downgrade relative to the version hard-coded into the currently running firmware. It does not provide a hardware-backed version floor. A production device may need a protected monotonic version counter or another trusted anti-rollback mechanism, depending on its threat model.

---

## 10. Migration checklist

Work through these items in order.

- [ ] Create a private `esp32-firmware` R2 bucket.
- [ ] Upload `firmware.bin`, `version.txt` and `version.sig`.
- [ ] Confirm the RSA signature verifies against the exact manifest bytes.
- [ ] Create an R2 Object Read only API token, scoped to the bucket.
- [ ] Configure the Worker R2 binding.
- [ ] Store R2 credentials and device tokens in Worker secrets.
- [ ] Deploy the Worker.
- [ ] Verify valid and invalid authentication using `curl`.
- [ ] Confirm the Worker returns the correct manifest and signature.
- [ ] Confirm the presigned URL downloads the expected firmware.
- [ ] Verify the URL expires as expected.
- [ ] Configure the correct CA trust for the Worker and R2 endpoints.
- [ ] Add the ArduinoJson and Base64 includes.
- [ ] Add `getSecureR2DownloadUrl()`.
- [ ] Change `downloadAndInstall()` to accept the supplied URL.
- [ ] Replace the old metadata-fetching sequence in `checkForFirmwareUpdate()`.
- [ ] Test a valid update.
- [ ] Test a tampered manifest and a tampered firmware image.
- [ ] Confirm the device boots into the new firmware and does not repeatedly reinstall the same release.

**My recommendation:** implement the Worker first and get the `curl` tests passing. Then make the three small ESP32 changes: add the authenticated metadata helper, parameterize `downloadAndInstall()`, and replace the metadata-fetching section of `checkForFirmwareUpdate()`.

This preserves your existing OTA implementation while adding authenticated device access and temporary R2 download authorization.
