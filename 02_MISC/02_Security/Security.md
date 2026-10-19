# ESP32-S3 Security Engineering — Module 1

**Target:** ESP32-S3 DevKitC-1 · N16R8 · Arduino IDE 2.x · Espressif Arduino-ESP32 Core 3.x

We'll approach this as a security engineering course, not a collection of Arduino examples. The objective is to build a device that remains trustworthy when an attacker can access its network, copy its flash contents, replace firmware files on a server, or physically handle the device.

One important distinction throughout this course: a security feature being available in the ESP32-S3 silicon does not mean it is enabled, configured correctly, or protected by your current Arduino firmware.

## 1.1 — The ESP32-S3 security architecture

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

[embeddedcomputing.com](https://embeddedcomputing.com/technology/ai-machine-learning/espressif-providing-support-for-the-aiot-market-with-its-dual-core-esp32-s3?utm_source=chatgpt.com)

Think of the security hardware as several independent mechanisms rather than one master security switch.

### eFuse controller — hardware-rooted configuration

Contains one-time-programmable fields and key blocks. These control security settings, key purposes, read/write protection, boot policy and other irreversible decisions.

### Secure Boot V2 — firmware authenticity

The ESP32-S3 ROM bootloader and second-stage bootloader participate in a chain of trust. Secure Boot V2 uses RSA-3072 with RSA-PSS signatures; the trusted public-key digest is anchored in eFuse.

— ESP-IDF Programming Guide latest documentation

### AES / XTS — flash and external-memory encryption

Hardware transparently encrypts and decrypts supported memory accesses. Flash Encryption protects off-chip flash contents; S3 also supports external SPI RAM encryption through the memory cache.

— ESP-IDF Programming Guide v6.1 documentation

### SHA and RSA accelerators — cryptographic operations

Accelerators help perform hashing and public-key operations. Hardware acceleration improves performance; it does not establish trust unless the correct keys and verification policies are used.

### HMAC and Digital Signature peripheral

These provide hardware-assisted cryptographic operations for supported key configurations. They can be useful for device authentication and signing operations without exposing protected key material to ordinary application code. The exact scheme depends on the selected peripheral mode and eFuse key purpose.

### Key Manager

Supports hardware-managed key derivation and related key operations on the S3. It is not a general-purpose vault into which Arduino applications can freely import arbitrary keys; its supported functions and configuration must be checked against the S3 documentation.

**Reference:** [ESP32-S3 Technical Reference Manual](https://documentation.espressif.com/esp32-s3_technical_reference_manual_en.pdf?utm_source=chatgpt.com)

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

**Submit answers and continue**

Ready to proceed? Our next practical milestone is **Lab 3: Valid vs Insecure TLS**, using `WiFiClientSecure` and `HTTPClient` on your ESP32-S3. Before the lab, we'll cover Module 2's trust model and Module 3's certificate validation, so you understand not only how to make the connection work, but also what each test proves.