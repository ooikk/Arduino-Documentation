[![logo-color](https://img.shields.io/badge/Index-Readme_Setup-8B639B?logo=github&logoColor=white&labelColor=5B7E3C)](https://github.com/ooikk/Arduino-Documentation/blob/main/02_MISC/02_Gitlab_FileAccess/README_SETUP.md)

# Secure GitLab File Access Library

We'll build this as a reusable Arduino library-style project, using separate `.h` and `.cpp` files. The design will follow the security principles from your ESP32-S3 Secure OTA work: authenticate the GitLab server, keep the access token in NVS rather than source code, and stream binary files directly into PSRAM.

The implementation targets your ESP32-S3 DevKitC-1 N16R8 with Arduino IDE 2.x and Espressif's Arduino-ESP32 Core 3.x.

## 1. Architecture

The project will have four layers:

| Module | Responsibility | Memory/storage |
|---|---|---|
| `NvsTokenManager` | Provision, save, retrieve and delete the GitLab Personal Access Token (PAT) | NVS |
| `GitlabClient` | Authenticate HTTPS requests, list repository files, handle pagination and download files | Flash for code; PSRAM for downloaded data |
| `PsramFileManager` | Allocate, replace, append, delete, save and load files | PSRAM and LittleFS |
| `GitlabFileAccess.ino` | Connect Wi-Fi, initialize storage, provision credentials and process serial commands | Application |

The data flow is:

```text
                         ESP32-S3 N16R8
┌────────────────────────────────────────────────────────────┐
│ Arduino IDE application                                    │
│                                                            │
│  Main sketch                                               │
│      │                                                     │
│      ├── Wi-Fi connection                                  │
│      │                                                     │
│      ├── NvsTokenManager ────────────────► NVS             │
│      │       Stores GitLab PAT                             │
│      │                                                     │
│      ├── GitlabClient                                      │
│      │       │                                             │
│      │       ├── WiFiClientSecure                          │
│      │       ├── Root CA verification                      │
│      │       ├── GitLab REST API                           │
│      │       └── HTTPS binary stream ─────► PSRAM          │
│      │                                                     │
│      └── PsramFileManager                                  │
│              ├── Allocate / append / overwrite ──► PSRAM   │
│              └── Persistent file storage ────────► LittleFS│
└────────────────────────────────────────────────────────────┘
                              │
                              │ HTTPS with certificate
                              │ validation and PAT
                              ▼
                     gitlab.com/api/v4
```

### Important security distinctions

- **TLS server authentication:** `setCACert()` validates the certificate chain against a trusted root CA. The implementation will not use `setInsecure()`.
- **GitLab API authentication:** The PAT authorizes repository access. It is not a substitute for TLS.
- **Binary integrity:** Streaming into PSRAM without converting the payload to `String` prevents text-encoding corruption, but it does not independently prove that the downloaded file is the intended firmware or asset. For stronger integrity, verify a trusted SHA-256 digest; for stronger authenticity, obtain that digest through a trusted, authenticated release or manifest process.
- **Credential protection:** Preferences stores the PAT in NVS, but ordinary Preferences storage does not automatically encrypt it. NVS encryption requires additional ESP-IDF configuration and provisioning.
- **Memory management:** A downloaded buffer is owned by the caller, which must release it with `free()`.

One additional design decision is important: `GitlabClient` will receive a `WiFiClientSecure` instance from the application instead of embedding Wi-Fi credentials or creating its own Wi-Fi connection. This makes the module easier to reuse with other ESP32 projects.

## 2. Download the complete Arduino project

I've prepared the project with separate header and implementation files, a main sketch, setup documentation, and a dedicated root-CA configuration file.

**ESP32S3_GitLabFileAccess.zip**

The archive contains:

| File | Purpose |
|---|---|
| `GitlabFileAccess.ino` | Main sketch, Wi-Fi setup, PSRAM diagnostics and serial commands |
| `NvsTokenManager.h` | NVS token-management interface |
| `NvsTokenManager.cpp` | Token storage, retrieval, deletion and Serial provisioning |
| `GitlabClient.h` | GitLab configuration and client interface |
| `GitlabClient.cpp` | HTTPS authentication, repository listing, pagination and binary download |
| `PsramFileManager.h` | In-memory file-management interface |
| `PsramFileManager.cpp` | PSRAM allocation, append, overwrite, deletion and LittleFS operations |
| `GitlabRootCA.h` | TLS root certificate configuration |
| `README_SETUP.md` | Installation, configuration and security notes |

The source files are already separated into Arduino IDE tabs/files, so you can reuse the modules in future sketches.

**Important before the first upload:** `GitlabRootCA.h` deliberately contains a placeholder, not a fabricated certificate. You must replace it with a verified root CA PEM that validates the certificate chain presented by `gitlab.com`. Until you do so, the client will refuse to initialize. This is intentional: an incorrect certificate or disabled certificate verification would undermine the security requirement.

### 2.1 Install the project

1. Download and extract the ZIP.
2. Create an Arduino sketch folder named `GitlabFileAccess`.
3. Copy the extracted source files into that folder.
4. Open `GitlabFileAccess.ino`.
5. Install `ArduinoJson` through Library Manager.
6. Select the correct ESP32-S3 board and configure the memory settings below.

### 2.2 Arduino IDE configuration

**Board Manager URL**

Open **File → Preferences** and add:

```text
[https://espressif.github.io/arduino-esp32/package_esp32_index.json](https://espressif.github.io/arduino-esp32/package_esp32_index.json)
```

Then open **Boards Manager**, search for `esp32`, and install **esp32 by Espressif Systems**, version 3.x.

Select **ESP32S3 Dev Module**, or the exact ESP32-S3 DevKitC-1 entry if your installation provides one.

Configure the **Tools** menu:

| Setting | Value |
|---|---|
| Board | ESP32S3 Dev Module or the appropriate DevKitC-1 entry |
| Flash Size | 16MB |
| PSRAM | OPI PSRAM |
| Partition Scheme | 16MB with OTA, or the compatible 16MB scheme available in your installed core |
| USB CDC On Boot | Enabled if you use the native USB serial interface |

The N16R8's 8MB PSRAM is separate from its 16MB flash. Selecting the correct PSRAM mode is essential.

### 2.3 Required libraries

| Component | Installation |
|---|---|
| ArduinoJson | Install through Library Manager |
| Preferences | Included with Arduino-ESP32 |
| LittleFS | Included with Arduino-ESP32 |
| WiFi | Included with Arduino-ESP32 |
| WiFiClientSecure | Included with Arduino-ESP32 |
| HTTPClient | Included with Arduino-ESP32 |
| mbedTLS SHA-256 | Included with the ESP32 platform |

No separate installation is needed for the ESP32 platform's built-in libraries.

### 2.4 Configure your Wi-Fi and project

Open `GitlabFileAccess.ino` and update these settings:

```cpp
static const char* WIFI_SSID = "YOUR_WIFI_SSID";
static const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

static const char* PROJECT_ID =
    "PUT_YOUR_PROJECT_ID_OR_group%2Fproject_HERE";

static const char* BRANCH = "main";
```

For your GitLab project, replace the placeholder with its numeric project ID or URL-encoded project path. The numeric ID is the simplest choice.

For example:

```cpp
static const char* PROJECT_ID = "12345678";
static const char* BRANCH = "main";
```

The number above is an example, not your actual project ID.

For a project you intend to publish, move Wi-Fi credentials into a local secrets header excluded from version control. The GitLab PAT must never be placed in source code.

## 3. Configure TLS and provision the GitLab token

These are the two security-critical configuration steps.

### 3.1 Configure the root CA

Open `GitlabRootCA.h`:

```cpp
#pragma once

static const char GITLAB_ROOT_CA[] PROGMEM = R"PEM(
REPLACE_WITH_VERIFIED_ROOT_CA_PEM
)PEM";
```

Replace the placeholder with the complete PEM-encoded root certificate.

The certificate must include:

```text
-----BEGIN CERTIFICATE-----
...verified root CA certificate data...
-----END CERTIFICATE-----
```

### How to obtain the correct certificate

1. Open <https://gitlab.com> in a browser on your computer.
2. Open the site's certificate information and inspect its certificate chain.
3. Identify the trusted root CA that validates the chain.
4. Obtain the root certificate from the CA's official website or another trusted certificate source.
5. Verify the certificate's identity and fingerprint before embedding it.

GitLab's documentation explains certificate chains and common verification failures: [GitLab SSL troubleshooting](https://docs.gitlab.com/omnibus/settings/ssl/ssl_troubleshooting/).

Do not simply copy the website's leaf certificate and assume it is a root CA. Also, don't assume the same root will remain correct indefinitely; certificate chains can change.

The client applies the configured root CA using:

```cpp
_tls.setCACert(_config.rootCA.c_str());
```

The code also rejects the placeholder certificate. There is no `setInsecure()` fallback.

### 3.2 Create the GitLab access token

For read-only repository access, create a token with these scopes:

- `read_api`
- `read_repository`

GitLab documents these scopes in its [access-token scope reference](https://docs.gitlab.com/security/tokens/access_token_scopes/) and [repository files API documentation](https://docs.gitlab.com/api/repository_files/).

Use a project access token if you want to restrict access to a single project and your project permissions allow it. Give the token the minimum role and lifetime required.

When the sketch first boots, it checks NVS for an existing token.

If none is found, it prompts:

```text
[NVS] No GitLab token is stored.
Enter GitLab PAT:
```

Enter the token followed by `Enter`.

The sketch saves it through Preferences and does not print the token's value.

**Security limitation:** Preferences stores the token in NVS, but the code does not enable NVS encryption. Someone with sufficient physical access and the ability to read the flash may be able to recover it. See the project's `README_SETUP.md` for the next steps toward NVS encryption, flash encryption and Secure Boot.

## 4. How the modules work

### 4.1 NvsTokenManager

The module exposes the four requested functions:

```cpp
bool nvs_saveGitlabToken(const String& token);
String nvs_loadGitlabToken();
bool nvs_hasGitlabToken();
void nvs_clearGitlabToken();
```

It uses the NVS namespace:

```cpp
static const char* NVS_NAMESPACE = "gitlab-auth";
static const char* NVS_KEY_TOKEN = "pat";
```

For example, loading the token is straightforward:

```cpp
String token = nvs_loadGitlabToken();

if (token.length() == 0) {
    Serial.println("No token available.");
}
```

The module handles NVS opening and writing failures. The application should not assume that a token was saved successfully just because provisioning was attempted.

### 4.2 GitlabClient

The class accepts an externally created TLS client:

```cpp
WiFiClientSecure tlsClient;
GitlabClient client(tlsClient);
```

Its configuration is:

```cpp
GitlabConfig cfg;

cfg.gitlabUrl = "[https://gitlab.com](https://gitlab.com)";
cfg.projectId = "12345678";
cfg.branch = "main";
cfg.rootCA = String(GITLAB_ROOT_CA);
```

Initialize it with the token retrieved from NVS:

```cpp
String token = nvs_loadGitlabToken();

if (!client.begin(cfg, token)) {
    Serial.println(client.lastError());
}
```

The client uses GitLab's REST API for repository listing and raw-file retrieval. The relevant official documentation is available for the [repository tree API](https://docs.gitlab.com/api/repositories/) and [repository files API](https://docs.gitlab.com/api/repository_files/).

### Listing files

```cpp
std::vector<String> files;

if (client.listFiles("", files)) {
    for (const String& name : files) {
        Serial.println(name);
    }
}
```

For a directory:

```cpp
client.listFiles("assets", files);
```

For recursive listing:

```cpp
client.listFiles("", files, true);
```

The implementation requests up to 100 entries per page and continues until it reaches the final page. It returns file names rather than directory names.

Note that this interface returns names, not full repository paths. If you need to recursively traverse a directory tree and retain each file's full path, the interface should be extended to return both the entry type and its path.

### Downloading binary files

The download function is:

```cpp
bool downloadFileToPSRAM(
    const String& repoFilePath,
    uint8_t** outBuffer,
    size_t* outLen
);
```

Example:

```cpp
uint8_t* data = nullptr;
size_t len = 0;

if (client.downloadFileToPSRAM(
        "assets/logo.png", &data, &len)) {

    Serial.printf("Downloaded %u bytes\n",
                  (unsigned)len);

    // Process data as bytes.
    // Do not convert it to String.

    free(data);
    data = nullptr;
    len = 0;
}
```

The implementation:

- Builds the raw-file API URL.
- Configures the GitLab authentication header.
- Establishes HTTPS using the supplied root CA.
- Reads the response as a byte stream.
- Allocates the receive buffer in PSRAM using `ps_malloc()`.
- Expands the allocation when the response exceeds the current capacity.
- Reports download progress.
- Checks the expected response length when the server provides it.
- Returns the buffer and its byte length to the caller.

The implementation limits a download to 6 MiB by default. You can adjust that limit in `GitlabClient.cpp` according to your application and available PSRAM.

**Important:** A download buffer must always be released with `free()`, including buffers allocated using `ps_malloc()`.

### 4.3 PsramFileManager

The `PsramFile` structure is:

```cpp
struct PsramFile {
    String name;
    uint8_t* data = nullptr;
    size_t len = 0;
};
```

The module supports these operations:

| Function | Behaviour |
|---|---|
| `fileNew()` | Creates a new buffer in PSRAM |
| `fileOverwrite()` | Replaces the existing contents |
| `fileAppend()` | Allocates a larger buffer and appends bytes |
| `fileDelete()` | Releases the buffer |
| `fileSaveToLittleFS()` | Saves the buffer to LittleFS |
| `fileLoadFromLittleFS()` | Loads a file from LittleFS into PSRAM |

For example:

```cpp
PsramFile img;

if (fileNew(img, "logo.png", 1024)) {
    Serial.printf("Allocated %u bytes\n",
                  (unsigned)img.len);
}

fileDelete(img);
```

The project also includes a small memory-management demonstration in `setup()`, exercising creation, append, overwrite and deletion.

The `fileAppend()` implementation allocates a new buffer and copies the existing data. This is simple and predictable for learning, but it is not the most efficient approach for repeated appends to very large files.

## 5. Test the project

### Test 1: Confirm PSRAM initialization

Upload the sketch and open Serial Monitor at 115200 baud.

You should see output similar to:

```text
ESP32-S3 Secure GitLab File Access
Chip: ESP32-S3, cores: 2
psramInit(): true
psramFound(): true
PSRAM total: 8388608 bytes, free: ...
Internal free heap: ...
```

The free-memory values depend on the current firmware and runtime allocations.

If PSRAM initialization fails, verify:

- The selected board is the ESP32-S3.
- Flash Size is 16MB.
- PSRAM is set to OPI PSRAM.
- You are using a compatible ESP32 Arduino Core 3.x release.

### Test 2: Provision the token

On first boot, enter the GitLab PAT when prompted.

The expected output is similar to:

```text
[NVS] No GitLab token is stored.
Enter GitLab PAT:
[NVS] GitLab token saved (value not displayed).
```

The token is now stored in NVS and should survive an ordinary reboot.

### Test 3: List repository files

Enter:

```text
LIST /
```

The sketch connects to Wi-Fi, initializes the GitLab client and retrieves the repository tree.

Example output:

```text
[WiFi] Connected. IP=192.168.1.100
[GitLab] Client initialized.
[GitLab] Files in '': 3
README.md
LICENSE
platformio.ini
```

These names are illustrative. Your repository contents will differ.

To list files inside a directory:

```text
LIST assets
```

### Test 4: Download a binary file

Make sure the repository contains a file at the specified path. Then enter:

```text
GET assets/logo.png
```

Example output:

```text
[GitLab] Download: 2048 / 8192 bytes (25%)
[GitLab] Download: 4096 / 8192 bytes (50%)
[GitLab] Download: 8192 / 8192 bytes (100%)
[GitLab] Download complete: 8192 bytes in PSRAM.
[GET] Downloaded 'assets/logo.png': 8192 bytes
[SHA-256] ...
[GET] Buffer released.
```

The hash is calculated over the downloaded bytes.

The printed SHA-256 hash is diagnostic, not an independent integrity check. To establish that the file is the intended asset, compare the result against a trusted expected digest or verify a digital signature.

The sample releases the buffer after calculating the hash. To retain it for image decoding or other processing, remove the `fileDelete(downloaded)` call and manage its lifetime carefully.

### Test 5: Exercise the memory manager

At startup, the sketch exercises the PSRAM file manager.

Example:

```text
[MemoryDemo] Testing PSRAM file operations...
[MemoryDemo] After append: 5 bytes
[MemoryDemo] After overwrite: 4 bytes
[MemoryDemo] fileDelete completed.
```

This test uses binary data, including values such as `0x00` and `0xFF`, to demonstrate that the memory manager does not rely on null-terminated strings.

### Test 6: Delete the saved token

Enter:

```text
DEL token
```

Expected output:

```text
[NVS] GitLab token deleted.
[Security] Reboot to provision a new token.
```

This removes the saved token from NVS. It does not revoke the token on GitLab's servers; revoke it separately in GitLab if necessary.

### Troubleshooting HTTP errors

| Error | Likely causes |
|---|---|
| HTTP 401 | Invalid, expired or revoked token |
| HTTP 403 | Insufficient project permissions |
| HTTP 404 | Incorrect project ID, missing path or branch, or inaccessible resource |
| TLS verification failure | Wrong root CA, incomplete chain, incorrect system time in some TLS configurations, or network interception |
| PSRAM allocation failure | Insufficient contiguous memory or a download exceeding available capacity |
| LittleFS mount failure | Incorrect partition configuration or incompatible filesystem contents |

For TLS problems, do not solve the issue by disabling certificate verification. Investigate the certificate chain and the specific verification error instead.

## 6. Recommended next steps for your Secure OTA work

Once repository listing and binary downloads work reliably, I recommend hardening the system in this order:

1. **Verify file integrity:** Compare downloaded files against a trusted SHA-256 digest.
2. **Authenticate releases:** For firmware, verify a digital signature using a public key embedded in the device.
3. **Encrypt credentials at rest:** Configure NVS encryption and an appropriate flash-encryption and Secure Boot strategy.
4. **Reduce memory use:** Add a streaming-to-LittleFS option for large assets so the entire file does not need to fit in PSRAM.
5. **Improve repository listing:** Return full repository paths and file metadata, especially when recursive traversal is enabled.
6. **Add a secure manifest:** Fetch a manifest containing the release version, expected size and integrity information, then verify it before processing or installing the file.

The key architectural benefit is that `GitlabClient` can now serve as a reusable repository-access layer for configuration files, images, JSON documents and future OTA manifests. The same pattern can also be extended to download firmware images, but firmware installation should have its own signature verification, partition management and rollback controls rather than treating a successful download as sufficient evidence that an image is safe to install.
