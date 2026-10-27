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
https://espressif.github.io/arduino-esp32/package_esp32_index.json
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

**Security limitation:** Preferences stores the token in NVS, but the code does not enable NVS encryption. Someone with sufficient physical access and the ability to read the flash may be able to recover it. See the project's [README_SETUP.md](https://github.com/ooikk/Arduino-Documentation/blob/main/02_MISC/02_Gitlab_FileAccess/README_SETUP.md) for the next steps toward NVS encryption, flash encryption and Secure Boot.

**References:**
- [Create a GitLab Personal Access Token: Fine-Grained](https://github.com/ooikk/Arduino-Documentation/blob/main/02_MISC/02_Gitlab_FileAccess/Gitlab_FileAccess.md#create-a-gitlab-fine-grained-personal-access-token)

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

cfg.gitlabUrl = "https://gitlab.com";
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

---
# Serial-to-GitLab Text Upload Command

## Overview

Let's add a serial-to-GitLab text upload command to your existing ESP32-S3 GitLab client. This avoids the LittleFS dependency entirely: you type the filename, destination path, and text directly into Serial Monitor, and the ESP32 commits the content to your private GitLab repository.

For an existing GitLab file, we'll offer three choices:

- **Overwrite** — replace the entire file with the text you enter.
- **Append** — retrieve the existing text, add your new text to the end, and commit the combined content.
- **Cancel** — leave the remote file unchanged.

One important distinction: GitLab's Repository Files API does not provide a direct “append” operation. Append must be implemented as a read–combine–update sequence, which is appropriate for text files but can encounter a conflict if another client edits the same file simultaneously.

### 1. Proposed serial interaction

We'll add a new command named `TEXTUPLOAD` and keep your existing `UPLOAD` and `UPDATE` commands unchanged.

Example session:

```text
> TEXTUPLOAD

Enter GitLab file path (e.g. notes.txt):
notes.txt

Enter text line by line.
Enter a line containing .END to finish.

> ESP32-S3 GitLab text upload test
> Created from Arduino Serial Monitor.
> Testing append and overwrite.
> .END

Checking GitLab file: notes.txt
File already exists.

Choose action:
  O = Overwrite
  A = Append
  C = Cancel
> A

Existing file downloaded.
New content appended.
Uploading combined content...

[SUCCESS] GitLab commit created.
File: notes.txt
Operation: Append
```

For a new file, the command creates it without asking whether to overwrite or append.

### 2. Add the serial input functions

Add these helper functions to your `.ino` file. They collect input one line at a time, support multiline text, and use `.END` as the terminator.

```cpp
// Read one line from Serial Monitor.
// Assumes Serial Monitor line ending is set to Newline
// or Both NL & CR.
static String readSerialLine(const char* prompt) {
  Serial.print(prompt);

  while (true) {
    if (Serial.available()) {
      String line = Serial.readStringUntil('\n');
      line.trim();
      return line;
    }
    delay(10);
  }
}

// Collect multiline text until a line containing .END.
// This example deliberately limits input to 8 KiB.
static bool readMultilineText(String& content) {
  static const size_t MAX_TEXT_BYTES = 8 * 1024;

  content = "";

  Serial.println("Enter text line by line.");
  Serial.println("Enter a line containing .END to finish.");
  Serial.println();

  while (true) {
    if (!Serial.available()) {
      delay(10);
      continue;
    }

    String line = Serial.readStringUntil('\n');
    line.trim();

    if (line == ".END") {
      break;
    }

    // Preserve line boundaries.
    if (content.length() + line.length() + 1 > MAX_TEXT_BYTES) {
      Serial.println("[ERROR] Text exceeds 8 KiB limit.");
      content = "";
      return false;
    }

    content += line;
    content += '\n';

    Serial.print("> ");
    Serial.println(line);
  }

  if (content.isEmpty()) {
    Serial.println("[ERROR] No text entered.");
    return false;
  }

  return true;
}
```

Note: This simple version trims leading and trailing whitespace from each line. If you need to preserve spaces exactly, I can provide a byte-preserving serial input implementation instead.

### 3. Add the `TEXTUPLOAD` workflow

Your existing GitLab client already has methods for downloading files, creating files, and updating files. The following workflow reuses those capabilities.

Because I don't have the exact current declarations from your `GitlabClient.h`, check the argument order of `uploadNewFile()`, `updateFile()`, and `downloadFileToPSRAM()` against your existing code before compiling.

```cpp
static void runTextUpload() {
  if (!gitlab && !initializeGitlab()) {
    return;
  }

  String remotePath = readSerialLine(
      "Enter GitLab file path (e.g. notes.txt): ");

  if (remotePath.isEmpty() ||
      remotePath.startsWith("/") ||
      remotePath.indexOf("..") >= 0) {
    Serial.println("[ERROR] Invalid repository path.");
    return;
  }

  String newText;
  if (!readMultilineText(newText)) {
    return;
  }

  Serial.printf("\nChecking GitLab file: %s\n",
                remotePath.c_str());

  uint8_t* oldData = nullptr;
  size_t oldLen = 0;

  bool exists = gitlab->downloadFileToPSRAM(
      remotePath, &oldData, &oldLen);

  String operation = "Create";
  String finalText = newText;

  if (exists) {
    Serial.println("File already exists.");
    Serial.println("O = Overwrite");
    Serial.println("A = Append");
    Serial.println("C = Cancel");

    String choice = readSerialLine("> ");
    choice.toUpperCase();

    if (choice == "C") {
      free(oldData);
      Serial.println("[CANCELLED] Remote file unchanged.");
      return;
    }

    if (choice == "O") {
      operation = "Overwrite";
    } else if (choice == "A") {
      operation = "Append";

      // Keep the old file content, then append new text.
      finalText.reserve(oldLen + newText.length() + 1);
      finalText = "";

      finalText.concat(
          reinterpret_cast<const char*>(oldData), oldLen);
      finalText += newText;
    } else {
      free(oldData);
      Serial.println("[ERROR] Invalid choice.");
      return;
    }

    free(oldData);
    oldData = nullptr;
  } else {
    // Important: only treat a confirmed HTTP 404 as
    // "file not found". A 401, 403, timeout, or TLS error
    // must abort instead of trying to create the file.
    Serial.println(
        "[ERROR] Could not read existing file or confirm "
        "that it is absent.");
    Serial.println(gitlab->lastError());
    Serial.println(
        "Check the HTTP status before enabling file creation.");
    return;
  }

  Serial.printf("Operation: %s\n", operation.c_str());
  Serial.printf("Final text size: %u bytes\n",
                (unsigned)finalText.length());

  // Call the appropriate existing GitLab API function here.
  // For a confirmed existing file:
  //
  //   gitlab->updateFile(remotePath, ...);
  //
  // For a confirmed missing file:
  //
  //   gitlab->uploadNewFile(remotePath, ...);
  //
  // Use the exact signatures from GitlabClient.h.
}
```

#### Important correction before using this workflow

The example above intentionally stops rather than guessing that every download failure means the file is absent. Your current download method returns a Boolean and stores the failure in `lastError()`, so it needs a reliable way to distinguish HTTP 404 from other errors before supporting automatic creation.

Also, the existing upload API uses Base64 encoding and has a configured 256 KiB limit. For this first version, the 8 KiB serial-input limit is conservative and suitable for text files.

### 4. Connect the new command to your command parser

In the section that processes the command entered in Serial Monitor, add:

```cpp
else if (command == "TEXTUPLOAD") {
  runTextUpload();
}
```

And add the following to your `HELP` output:

```text
TEXTUPLOAD  Prompt for text and create, overwrite, or append a GitLab file
```

### 5. One small change needed in `GitlabClient`

To finish this cleanly, I recommend adding a method that explicitly checks whether a remote file exists and returns its HTTP status. That avoids accidentally interpreting an authentication failure as a missing file.

Please paste the declarations from `GitlabClient.h` for these methods:

```cpp
downloadFileToPSRAM()
uploadNewFile()
updateFile()
lastError()
```

With those exact signatures, I can provide the complete, compile-ready `runTextUpload()` implementation, including correct handling of a new file, overwrite, append, and cancellation.

## Complete Code Implementation

I'll make the workflow safe for your ESP32-S3 GitLab client: create when the file is confirmed missing, prompt before overwriting an existing file, append by combining the existing text with the new text, and cancel without modifying GitLab.

One important implementation detail: your current `downloadFileToPSRAM()` returns only `true` or `false`. A failed download alone cannot tell us whether the file is missing (HTTP 404) or the request failed due to a token, TLS, or network problem. The implementation must distinguish those cases before creating a new file.

Below is the implementation for your `.ino` file. It uses your existing `gitlab` object, `initializeGitlab()`, `downloadFileToPSRAM()`, and `lastError()`.

One caveat: I can't guarantee compilation against your exact `GitlabClient.h` without seeing its method declarations. The two write calls near the end are marked for verification because their signatures aren't present in the conversation.

### 1. Add the serial input helpers

These preserve spaces inside text lines and accept multiline input until `.END`. Set Serial Monitor line ending to `Newline` or `Both NL & CR`.


```cpp

static constexpr size_t MAX_TEXT_INPUT_BYTES = 8UL * 1024UL;
static constexpr size_t MAX_TEXT_UPLOAD_BYTES = 256UL * 1024UL;

// Read one complete serial line, preserving spaces within the line.
// Serial Monitor should use "Newline" or "Both NL & CR".
static bool readSerialLine(String& out, const char* prompt) {
  static bool skipNextLF = false;

  Serial.print(prompt);
  out = "";

  while (true) {
    if (!Serial.available()) {
      delay(2);
      continue;
    }

    const char c = static_cast<char>(Serial.read());

    // If the previous line ended with CR, consume the LF in CRLF.
    if (skipNextLF) {
      skipNextLF = false;
      if (c == '\n') continue;
    }

    if (c == '\n') {
      return true;
    }

    if (c == '\r') {
      skipNextLF = true;
      return true;
    }

    if (!out.concat(c)) {
      Serial.println("\n[ERROR] Not enough memory for serial input.");
      return false;
    }
  }
}

// Read multiline text until .END. Enter .CANCEL to abort.
static bool readMultilineText(String& out) {
  out = "";

  if (!out.reserve(MAX_TEXT_INPUT_BYTES + 1)) {
    Serial.println("[ERROR] Cannot reserve memory for text input.");
    return false;
  }

  Serial.println("Enter text, one line at a time.");
  Serial.println("Enter .END on a line by itself to finish.");
  Serial.println("Enter .CANCEL on a line by itself to abort.");

  while (true) {
    String line;
    if (!readSerialLine(line, "> ")) {
      return false;
    }

    if (line == ".END") {
      break;
    }

    if (line == ".CANCEL") {
      out = "";
      Serial.println("[CANCELLED] Text input aborted.");
      return false;
    }

    // Each input line is stored with a trailing LF.
    if (line.length() + 1 > MAX_TEXT_INPUT_BYTES - out.length()) {
      out = "";
      Serial.println("[ERROR] Text exceeds the 8 KiB input limit.");
      return false;
    }

    if (!out.concat(line) || !out.concat('\n')) {
      out = "";
      Serial.println("[ERROR] Memory allocation failed while building text.");
      return false;
    }
  }

  if (out.isEmpty()) {
    Serial.println("[ERROR] Empty text; nothing uploaded.");
    return false;
  }

  return true;
}
```

### 2. Add `runTextUpload()`

This handles existing files and cancellation. It only treats an error as “file missing” when `lastError()` starts with `HTTP 404`.

```cpp
static void runTextUpload() {
  if (!gitlab && !initializeGitlab()) {
    Serial.println("[ERROR] GitLab client initialization failed.");
    return;
  }

  String remotePath;
  if (!readSerialLine(
          remotePath,
          "GitLab destination path (e.g. notes.txt; or .CANCEL): ")) {
    return;
  }

  remotePath.trim();

  if (remotePath == ".CANCEL") {
    Serial.println("[CANCELLED] No changes made.");
    return;
  }

  if (remotePath.isEmpty() ||
      remotePath.startsWith("/") ||
      remotePath.indexOf("..") >= 0) {
    Serial.println("[ERROR] Invalid repository path.");
    return;
  }

  String newText;
  if (!readMultilineText(newText)) {
    return;
  }

  if (newText.length() > MAX_TEXT_UPLOAD_BYTES) {
    Serial.println("[ERROR] Text exceeds the 256 KiB upload limit.");
    return;
  }

  Serial.printf("\n[GitLab] Checking '%s'...\n", remotePath.c_str());

  uint8_t* oldData = nullptr;
  size_t oldLen = 0;

  const bool downloaded =
      gitlab->downloadFileToPSRAM(remotePath, &oldData, &oldLen);

  bool fileMissing = false;

  if (!downloaded) {
    const String err = gitlab->lastError();
    Serial.printf("[GitLab] Read failed: %s\n", err.c_str());

    // Only the client's explicit HTTP 404 is treated as a candidate
    // for a new file. A 404 may also mean branch/project/access trouble.
    if (err.startsWith("HTTP 404")) {
      fileMissing = true;
    } else {
      Serial.println(
          "[ABORT] Cannot verify the remote file. No write attempted.");
      return;
    }
  }

  String finalText;
  String operation;
  String commitMessage;

  if (fileMissing) {
    Serial.println(
        "The remote file returned HTTP 404. This may mean it is missing, "
        "but can also indicate a branch/project/access issue.");
    Serial.println("Attempt to create this path? (Y/N)");

    String choice;
    if (!readSerialLine(choice, "> ")) {
      return;
    }
    choice.trim();
    choice.toUpperCase();

    if (choice != "Y") {
      Serial.println("[CANCELLED] No changes made.");
      return;
    }

    // Copy new content and verify the copy succeeded.
    if (!finalText.reserve(newText.length() + 1) ||
        !finalText.concat(newText) ||
        finalText.length() != newText.length()) {
      Serial.println("[ERROR] Could not prepare new file content.");
      return;
    }

    operation = "Create";
    commitMessage = "Create text file from ESP32-S3";
  } else {
    // A successful download may contain zero bytes; that means the file
    // exists but is empty. downloadFileToPSRAM() must support that case.
    Serial.printf("Remote file exists (%u bytes).\n", (unsigned)oldLen);
    Serial.println("Choose action:");
    Serial.println("  O = Overwrite");
    Serial.println("  A = Append");
    Serial.println("  C = Cancel");

    String choice;
    if (!readSerialLine(choice, "> ")) {
      free(oldData);
      return;
    }

    choice.trim();
    choice.toUpperCase();

    if (choice == "C") {
      free(oldData);
      Serial.println("[CANCELLED] No changes made.");
      return;
    }

    if (choice == "O") {
      if (!finalText.reserve(newText.length() + 1) ||
          !finalText.concat(newText) ||
          finalText.length() != newText.length()) {
        free(oldData);
        Serial.println("[ERROR] Could not prepare overwrite content.");
        return;
      }

      operation = "Overwrite";
      commitMessage = "Overwrite text file from ESP32-S3";
    } else if (choice == "A") {
      // A separator is needed only if the existing file is non-empty
      // and does not already end with LF.
      const bool oldEndsWithNewline =
          oldLen > 0 && oldData[oldLen - 1] == '\n';

      const size_t separatorBytes =
          (oldLen > 0 && !oldEndsWithNewline) ? 1 : 0;

      // Check additions without allowing size_t overflow.
      if (oldLen > MAX_TEXT_UPLOAD_BYTES ||
          separatorBytes > MAX_TEXT_UPLOAD_BYTES - oldLen ||
          newText.length() >
              MAX_TEXT_UPLOAD_BYTES - oldLen - separatorBytes) {
        free(oldData);
        Serial.println(
            "[ERROR] Combined file exceeds the 256 KiB upload limit.");
        return;
      }

      const size_t combinedLen =
          oldLen + separatorBytes + newText.length();

      if (!finalText.reserve(combinedLen + 1)) {
        free(oldData);
        Serial.println("[ERROR] Cannot allocate combined text buffer.");
        return;
      }

      bool built = true;

      if (oldLen > 0) {
        built = finalText.concat(
            reinterpret_cast<const char*>(oldData),
            static_cast<unsigned int>(oldLen));
      }

      if (built && separatorBytes) {
        built = finalText.concat('\n');
      }

      if (built) {
        built = finalText.concat(newText);
      }

      if (!built || finalText.length() != combinedLen) {
        free(oldData);
        Serial.println("[ERROR] Failed to build complete appended text.");
        return;
      }

      operation = "Append";
      commitMessage = "Append text from ESP32-S3";
    } else {
      free(oldData);
      Serial.println("[CANCELLED] Invalid choice; no changes made.");
      return;
    }

    free(oldData);
    oldData = nullptr;
  }

  // Final safety check before sending data to GitLab.
  const size_t finalLen = finalText.length();

  if (finalLen == 0) {
    Serial.println("[ERROR] Final content is empty; upload cancelled.");
    return;
  }

  if (finalLen > MAX_TEXT_UPLOAD_BYTES) {
    Serial.println("[ERROR] Final content exceeds 256 KiB.");
    return;
  }

  Serial.printf("\n[GitLab] Operation: %s\n", operation.c_str());
  Serial.printf("[GitLab] Destination: %s\n", remotePath.c_str());
  Serial.printf("[GitLab] Final size: %u bytes\n", (unsigned)finalLen);
  Serial.println("Commit these changes? (Y/N)");

  String confirm;
  if (!readSerialLine(confirm, "> ")) {
    return;
  }
  confirm.trim();
  confirm.toUpperCase();

  if (confirm != "Y") {
    Serial.println("[CANCELLED] No GitLab write attempted.");
    return;
  }

  const uint8_t* bytes =
      reinterpret_cast<const uint8_t*>(finalText.c_str());

  bool success = false;

  if (fileMissing) {
    success = gitlab->uploadNewFile(
        remotePath, bytes, finalLen, commitMessage);
  } else {
    success = gitlab->updateFile(
        remotePath, bytes, finalLen, commitMessage);
  }

  if (success) {
    Serial.printf("[SUCCESS] %s completed.\n", operation.c_str());
    Serial.printf("GitLab path: %s\n", remotePath.c_str());
  } else {
    Serial.printf("[ERROR] GitLab %s failed: %s\n",
                  operation.c_str(),
                  gitlab->lastError().c_str());
  }
}
```

### 3. Modify GitlabClient.cpp to support empty files

In `downloadFileToPSRAM()`, find this block near the end:

```cpp
if (used == 0) {
  free(buffer);
  setError("Downloaded file is empty.");
  return false;
}
```

Replace it with:

```cpp

if (used == 0) {
  // HTTP 200 with zero bytes is a successful download of an empty file.
  // Return the allocated buffer so the caller can free() it normally.
  *outBuffer = buffer;
  *outLen = 0;

  Serial.println("[GitLab] Download complete: empty file (0 bytes).");
  return true;
}

```

Keep the existing length-mismatch check immediately before this block. That check ensures a response declaring a nonzero content length cannot be mistaken for a valid empty file.

This change lets `runTextUpload()` distinguish an existing zero-byte file from a failed download.

### 4. Fix JSON newline escaping

In `GitlabClient.cpp`, find `jsonEscape()` and replace this line:

```cpp
case '\n': out += "\n"; break;
```

with:

```cpp
case '\n': out += "\\n"; break;
```

The distinction matters: JSON strings must encode line feeds as the two characters `\` and `n`; an unescaped literal newline makes the JSON invalid.

For completeness, the relevant switch cases should look like this:

```cpp

switch (c) {
  case '"':  out += "\\\""; break;
  case '\\': out += "\\\\"; break;
  case '\b': out += "\\b"; break;
  case '\f': out += "\\f"; break;
  case '\n': out += "\\n"; break;
  case '\r': out += "\\r"; break;
  case '\t': out += "\\t"; break;

  default:
    if (static_cast<uint8_t>(c) < 0x20) {
      const char hex[] = "0123456789ABCDEF";
      out += "\\u00";
      out += hex[(static_cast<uint8_t>(c) >> 4) & 0x0F];
      out += hex[static_cast<uint8_t>(c) & 0x0F];
    } else {
      out += c;
    }
}
```

### 5. Recommended improvement to `writeFileRequest()`

Your current `writeFileRequest()` correctly enforces the 256 KiB input limit before Base64 encoding. However, Base64 expands the input to approximately 4/3 of its original size, and the JSON payload is built in a separate `String`. On an ESP32-S3, that can create significant memory pressure.

At minimum, check the payload allocation before using it. Replace the existing payload-building section with this:

```cpp
  String payload;
  const size_t payloadCapacity =
      encodedLen + commitMessage.length() +
      _config.branch.length() + 128;

  if (!payload.reserve(payloadCapacity)) {
    free(encoded);
    setError("Memory allocation failed for JSON upload payload.");
    return false;
  }

  payload += "{\"branch\":\"";
  payload += jsonEscape(_config.branch);
  payload += "\",\"commit_message\":\"";
  payload += jsonEscape(commitMessage);
  payload += "\",\"content\":\"";
  payload += encoded;
  payload += "\",\"encoding\":\"base64\"}";

  free(encoded);
  encoded = nullptr;

  if (payload.length() == 0) {
    setError("Failed to construct JSON upload payload.");
    return false;
  }
```

This check helps detect allocation failure, although `String` concatenation does not provide a complete guarantee that every subsequent operation succeeds. For larger uploads, a future improvement would be to stream the encoded payload rather than holding the source, Base64 output, and JSON payload in memory simultaneously.

### 6. Integration checklist

Before compiling, make sure the following are true:
- [ ] The corrected `readSerialLine()` replaces the original version.
- [ ] `readMultilineText()` and `runTextUpload()` replace the old implementations.
- [ ] The empty-file change is applied to `downloadFileToPSRAM()`.
- [ ] `jsonEscape()` correctly escapes newlines.
- [ ] The existing `uploadNewFile()` and `updateFile()` declarations remain unchanged.
- [ ] The command dispatcher recognizes `TEXTUPLOAD`.
- [ ] The Serial Monitor line ending is set to **Newline** or Both **NL** & **CR**.

One clarification on the payload allocation suggestion: checking `payload.length() > payloadCapacity` is not a reliable way to detect a failed `String` allocation, because `String` can grow beyond its reserved capacity. Keep the `reserve()` check, but don't treat that length comparison as a definitive allocation-failure test.



### 7. Register the command

In your existing serial command dispatcher, add:

```cpp
else if (command == "TEXTUPLOAD") {
  runTextUpload();
}
```

Add this line to `HELP`:

```text
TEXTUPLOAD  Enter text and create, overwrite, or append a GitLab file
```

### 8. Test with a new file first

**Suggested test sequence**

1. Create a new `notes.txt` containing two lines.
2. Run `TEXTUPLOAD` again and choose `O` to overwrite it.
3. Run it again and choose `A` to append a third line.
4. Download `notes.txt` with your existing `GET` command and verify the contents.
5. Try `C` and `.CANCEL` and confirm no GitLab write occurs.
6. Test a zero-byte file and verify that append works.
7. Try an invalid destination path and a combined file exceeding 256 KiB.

One final limitation remains: a `404` can mean the file is missing, but it can also indicate an invalid branch or inaccessible resource. The code asks before attempting creation, and the GitLab API's response to that write remains the final authority.

For example:

```text
TEXTUPLOAD
GitLab destination path (e.g. notes.txt): esp32-notes.txt
Enter text, one line at a time.
Enter .END on a line by itself to finish.
> ESP32-S3 serial upload test
> First line of notes.
> Second line of notes.
> .END
Remote file does not exist.
Create it? (Y/N)
> Y
```


Then test appending another line, and finally test overwriting the file.

Before compiling: please paste the declarations of `uploadNewFile()`, `updateFile()`, and `downloadFileToPSRAM()` from `GitlabClient.h`. The method argument order may differ, and I can then adjust the implementation to match your actual client exactly.

---

# Gitlab Path Access

The URL does not know about your file-repository project by itself, and the token does not contain the file path. Your ESP32-S3 identifies the target repository through the project ID in the URL, identifies the file through the repository path, and proves its access rights through the token in the HTTP header.

Let's use your actual ESP32-S3 configuration to explain how these pieces work together.

## 1. How GitLab knows which file to return

Suppose your sketch executes:

```cpp
GET text.txt
```

Your code constructs this URL:

```text
https://gitlab.com/api/v4/projects/12345678/repository/files/text.txt/raw?ref=main
```
where `PROJECT_ID = 12345678`

### 1. SERVER

```text
https://gitlab.com
```

Connect to GitLab's HTTPS server.

### 2. PROJECT IDENTIFIER

```text
/api/v4/projects/12345678
```

Select the GitLab project whose numeric ID is `12345678`.

### 3. FILE IDENTIFIER

```text
/repository/files/text.txt/raw
```

Request the raw contents of `text.txt` from that project's repository.

### 4. BRANCH

```text
?ref=main
```

Read the file from the `main` branch.

GitLab combines these URL parameters to locate the requested file. The repository path is relative to the root of the repository, so `text.txt` means the file at the root, whereas `docs/text.txt` means the file inside the `docs` directory.

## 2. What role does the token play?

Your sketch sends the token separately in the HTTP request header:

```cpp
http.addHeader("PRIVATE-TOKEN", _token);
```

The request therefore has two independent parts:

| Part | Purpose |
|---|---|
| **URL** | Identifies the resource: which GitLab server, project, branch, and file to access. |
| **Token** | Authenticates the request: which authorized identity is making the request, and which operations it is permitted to perform. |

The token is not embedded with the project path. GitLab checks the token's validity, permissions, and the user's access to the target project. A valid token does not automatically grant access to every project.

For example, your fine-grained token can be restricted to `file-repository/main-file` and the required repository operations. If you change the URL to another project, GitLab will not automatically grant that token access to the other project.

## 3. How your sketch connects these pieces

Your `.ino` file sets the project configuration:

```cpp
static const char* PROJECT_ID = "12345678";
static const char* BRANCH = "main";

cfg.gitlabUrl = "https://gitlab.com";
cfg.projectId = PROJECT_ID;
cfg.branch = BRANCH;
```

Then `initializeGitlab()` loads the token from NVS and passes it to the client:

```cpp
gitlab->begin(cfg, token);
```

Finally, `downloadFileToPSRAM()` constructs the file URL and adds the authentication header.

```cpp
String url = apiBase() + "/repository/files/" +
             encodePathForFileEndpoint(repoFilePath) +
             "/raw?ref=" + encodeQuery(_config.branch);

http.addHeader("PRIVATE-TOKEN", _token);
```

This design separates configuration, authentication, and file selection.

## 4. Can you access another GitLab project just by changing the URL?

Not necessarily. Consider these cases:

| Change | Expected outcome |
|---|---|
| Change `text.txt` to `Indoor.bmp` in the same project | Works if the file exists and the token has read permission. |
| Change `main` to another existing branch | Works if the file exists on that branch and the token has permission. |
| Change the project ID to another private project | Only works if the token and its associated identity are authorized for that project. |
| Remove the token header | A private repository request should fail authentication. |
| Use a token that allows reading but not writing | Downloads may work, while uploads, updates, or deletes are denied. |

One useful detail: the numeric project ID is not a secret. It identifies the project, but it does not grant access to it. The token must still be protected.

For your ESP32-S3 application, the overall model is:

**Request = Server + Project_ID + File_Path + Branch + Token**

Each part has a distinct role, and GitLab uses them together to determine whether it can return or modify the requested file.


---

# Create a GitLab Personal Access Token: Fine-Grained 

## 1. Objective

Create a GitLab token that grants the ESP32-S3 permission to list, download, upload, update, and delete files in a specific private repository.

- **GitLab group:** `file-repository`
- **GitLab project:** `main-file`
- **Token type:** Fine-grained Personal Access Token
- **Token description:** `write read access`
- **Expiration date:** Choose a suitable date
- **Repository access:** Restricted to `file-repository/main-file`

## 2. Open Personal Access Tokens

1. Sign in to your GitLab account.
2. Open the [GitLab Personal Access Tokens page](https://gitlab.com/-/user_settings/personal_access_tokens).
3. Choose the option to create a new token.
4. Select **Fine-grained token** if GitLab presents a choice of token types.

## 3. Configure the token

Enter the following details:

| Setting | Value |
|---|---|
| Token type | Fine-grained token |
| Description | `write read access` |
| Expiration | Select an appropriate expiry date |
| Group and project access | Only specific groups or projects I have access to |
| Selected project | `file-repository/main-file` |

Do not grant access to unrelated projects.

## 4. Configure repository permissions

Under **Group and project permissions**, configure the **Repository** permissions as follows:

| Permission | Required | Purpose |
|---|---|---|
| Create | Yes | Create new repository files |
| Read | Yes | List and download repository files |
| Update | Yes | Modify existing repository files |
| Delete | Yes | Delete repository files |

Select only the permissions required by the application. The four permissions above match the configuration shown in the GitLab token details screenshot.

**Note:** Fine-grained tokens use specific resource permissions rather than the traditional broad `api` scope. Therefore, the `api` checkbox may not be available.

## 5. Create and save the token

1. Review the token type, expiry date, project restriction, and permissions.
2. Click the button to create or generate the token.
3. Copy the token immediately and store it securely.

GitLab may display the complete token only once. Do not include the actual token in source code, documentation, screenshots, or public repositories.

## 6. Configure the ESP32-S3

1. Open the ESP32-S3 GitLab File Access project in Arduino IDE.
2. Configure the GitLab URL as `https://gitlab.com`.
3. Set the project ID to the numeric ID shown on the `main-file` project page.
4. Use the existing `NvsTokenManager` to provision the token into NVS.
5. Keep TLS certificate validation enabled with a verified root CA.

The token should be sent in the HTTP request header expected by the GitLab API implementation, for example:

```http
PRIVATE-TOKEN: <your_token>
```

Do not print the token to the Serial Monitor.

## 7. Test the permissions

After provisioning the token, test each operation using a non-critical test file:

1. List files in the repository.
2. Download an existing file.
3. Upload a new test file.
4. Update the test file.
5. Delete the test file.

Check the HTTP response codes and confirm the changes in GitLab. Fine-grained permissions must support the exact API endpoints used by the ESP32 application, so verify all five operations rather than assuming that selecting the permissions guarantees every endpoint will work.

## 8. Security considerations

- Restrict the token to `file-repository/main-file`.
- Set an expiration date and renew the token when necessary.
- Store the token in NVS instead of hardcoding it in the sketch.
- Remember that NVS storage alone does not guarantee encryption.
- Never disable TLS certificate validation to work around authentication or connection errors.
- Revoke the token if it is exposed or no longer required.

**Expected result:** The ESP32-S3 has a dedicated token configured for the private `file-repository/main-file` project, with repository permissions intended for file read/write management.


# GitLab Token Types

The two tokens serve different purposes in GitLab. For your ESP32-S3 project, the key difference is which resources the token can access, what it can do, and which account or project it is tied to.

Your screenshot shows the [**Deploy tokens**](https://github.com/ooikk/Arduino-Documentation/blob/main/02_MISC/02_OTA/02_Security/Security.md#step-4-create-a-read-only-deploy-token) section under `file-repository/main-file → Settings → Repository`. That is a third token type, separate from both legacy token and the fine-grained token you created earlier.

## 1. Compare the three token types

| Feature | Personal Access Token (PAT): Legacy Token| Personal Access Token (PAT): Fine-grained | Deploy Token |
|---|---|---|---|
| Associated with | Your GitLab user | Your GitLab user, with restricted resource access | A project or group |
| Restrict to one project | Not inherently; depends on token type and configuration | Yes, as configured in your screenshot | Yes, if created at project level |
| Download repository files | Yes, with appropriate permissions | Yes, with appropriate permissions | Yes, with `read_repository` scope |
| Upload through REST API | Yes, with suitable API scope and role | Depends on supported API permissions | No — deploy tokens do not provide general Repository Files API write access |
| Git push over HTTPS | Possible with appropriate permissions | Depends on supported permissions | Yes, with `write_repository` scope |
| Uses your personal identity | Yes | Yes | No; uses a generated deploy-token username |
| Best use | General API development | Narrowly scoped API access | Automated repository or package access |

The main limitation of the Deploy Token is important: it cannot authenticate to the GitLab REST API. Its `read_repository` permission supports repository cloning/pulling, while `write_repository` is not a Deploy Token scope. The `write_repository` scope belongs to personal, group, and project access tokens. [GitLab Docs](https://docs.gitlab.com/user/project/deploy_tokens/)

## 2. What is Deploy Token can do

The deploy token shows a token named `file-repository-main` with these scopes:

- `read_package_registry`
- `write_package_registry`

These permissions are for the package registry, not for repository files. They will not authorize your ESP32 sketch's current GitLab file API calls. [GitLab Docs](https://docs.gitlab.com/user/project/deploy_tokens/)

## 3. Which token should you use for the ESP32?

For your current design, which uses `HTTPClient` and GitLab REST API endpoints to list, download, upload, and delete files, I recommend:

Use the **fine-grained Personal Access Token** you created earlier, provided its selected repository permissions authorize the exact endpoints your sketch uses. Your screenshot showed it restricted to `file-repository/main-file` with Repository **Create**, **Read**, **Update**, and **Delete** permissions.

That is a closer match to your goal than the Deploy Token. Fine-grained tokens are designed to limit access to selected resources and API permissions. [GitLab Docs](https://docs.gitlab.com/security/tokens/)

If an operation fails, check its HTTP response and the fine-grained permission mapping before changing the token.

## 4. Test your token

You can define the token as a string variable directly in the PowerShell script. Replace `YOUR_GITLAB_TOKEN_HERE` with your actual token.

### Updated PowerShell script

#### GitLab configuration

```powershell
$projectId = "YOUR_NUMERIC_PROJECT_ID"
$token = "YOUR_GITLAB_TOKEN_HERE"
```

#### Test repository access

```powershell
# GitLab configuration
$projectId = "YOUR_NUMERIC_PROJECT_ID"
$token = "YOUR_GITLAB_TOKEN_HERE"

# Test repository access
$uri = "https://gitlab.com/api/v4/projects/$projectId/repository/tree?ref=main"

try {
    $result = Invoke-RestMethod `
        -Uri $uri `
        -Headers @{ "PRIVATE-TOKEN" = $token } `
        -Method Get

    Write-Host "Repository access successful."
    $result | Select-Object name, type, path
}
catch {
    Write-Host "Repository request failed."

    if ($_.Exception.Response) {
        Write-Host "HTTP status:" ([int]$_.Exception.Response.StatusCode)
    }

    Write-Host $_.ErrorDetails.Message
}
finally {
    $token = $null
}
```

#### Test print a text file from Gitlab

Upload a text file to Gitlab, example:
File name: `text.txt`

```powershell
# GitLab configuration
$projectId = "YOUR_NUMERIC_PROJECT_ID"
$token = "YOUR_GITLAB_TOKEN_HERE"
$filePath = "text.txt"
$branch = "main"

$headers = @{ "PRIVATE-TOKEN" = $token }

try {
    # 1. List repository files
    $treeUri = "https://gitlab.com/api/v4/projects/$projectId/repository/tree?ref=$branch"

    $files = Invoke-RestMethod `
        -Uri $treeUri `
        -Headers $headers `
        -Method Get

    Write-Host "`nRepository files:"
    $files | Select-Object name, type, path | Format-Table -AutoSize

    # 2. Download and print text.txt contents
    $encodedPath = [Uri]::EscapeDataString($filePath)
    $fileUri = "https://gitlab.com/api/v4/projects/$projectId/repository/files/$encodedPath/raw?ref=$branch"

    $content = Invoke-RestMethod `
        -Uri $fileUri `
        -Headers $headers `
        -Method Get

    Write-Host "`nContents of $filePath :"
    Write-Host "----------------------------------------"
    Write-Output $content
    Write-Host "`n----------------------------------------"
}
catch {
    Write-Host "GitLab request failed."

    if ($_.Exception.Response) {
        Write-Host "HTTP status:" ([int]$_.Exception.Response.StatusCode)
    }

    Write-Host $_.ErrorDetails.Message
}
finally {
    $token = $null
    $headers = $null
}
```

### How to use it

1. Replace `YOUR_NUMERIC_PROJECT_ID` with your GitLab project's numeric ID.
2. Replace `YOUR_GITLAB_TOKEN_HERE` with your fine-grained Personal Access Token.
3. Ensure `ref=main` matches your repository's branch.
4. Run the script.

### Security note

Avoid saving this script with your real token in a shared folder or committing it to Git. The token is stored as plain text in the script and may remain in your PowerShell history or editor backups. Since this is a private repository, keep the token's permissions restricted to the operations your ESP32 needs.

---

# C++ Modules: `GitlabClient`, `PSRamFileManager`, and `NVSTokenManager`

In your ESP32-S3 project, `GitlabClient`, `PSRamFileManager`, and `NVSTokenManager` are modules that serve different purposes. The main difference is that `GitlabClient` is designed as a C++ class, while the other two can be implemented as collections of standalone functions.

Let's examine why `GitlabClient` uses `GitlabClient::functionName()` in its implementation but is called using `gitlab->functionName()` in your `.ino` file.

## 1. Understand the three layers of a C++ module

A typical module in your project has three parts:

| Layer | File | Purpose |
|---|---|---|
| Declaration (interface) | `.h` | Tells other files which functions, classes, and data are available. |
| Implementation | `.cpp` | Contains the actual function definitions and logic. |
| Usage (caller) | `.ino` | Calls the functions to perform the required operation. |

For example, your GitLab client may have this structure:

```text
02_Gitlab_FileAccess/
│
├── 02_Gitlab_FileAccess.ino    Main program
│
├── GitlabClient.h              Class declaration
├── GitlabClient.cpp            Class implementation
│
├── PSRamFileManager.h          PSRAM functions
├── PSRamFileManager.cpp        PSRAM implementation
│
├── NVSTokenManager.h           NVS functions
└── NVSTokenManager.cpp         NVS implementation
```

The header files provide the interfaces. The `.cpp` files implement them. The `.ino` file coordinates the modules.

## 2. Why does `GitlabClient.cpp` use `GitlabClient::`?

Consider a simplified version of your class declaration in `GitlabClient.h`:

```cpp
class GitlabClient {
public:
    bool uploadNewFile(
        const String& repoFilePath,
        const uint8_t* data,
        size_t len,
        const String& commitMessage
    );

    bool updateFile(
        const String& repoFilePath,
        const uint8_t* data,
        size_t len,
        const String& commitMessage
    );
};
```

This declares a class called `GitlabClient` with two member functions.

It tells the compiler that these functions belong to the class, but it does not provide their implementations.

In `GitlabClient.cpp`, you might see:

```cpp
bool GitlabClient::uploadNewFile(
    const String& repoFilePath,
    const uint8_t* data,
    size_t len,
    const String& commitMessage
) {
    // Implementation goes here
}
```

The important part is:

```cpp
GitlabClient::uploadNewFile
```

The `::` operator is called the **scope resolution operator**.

It tells the compiler:

```text
uploadNewFile() is a member function of the GitlabClient class.
```

Without the class name, this would be a different declaration:

```cpp
bool uploadNewFile(...) {
    // ...
}
```

That would define an ordinary standalone function, not the member function declared in the class.

### Why separate the declaration and implementation?

You could put the entire function implementation inside the class declaration in the header. However, separating them has several benefits:

- The header presents a clean interface.
- The `.cpp` file contains the implementation details.
- Other modules can use the class without needing to read its implementation.
- Changes to implementation details often do not require changes to the interface.

This is especially useful for `GitlabClient`, which handles HTTPS, authentication, HTTP requests, Base64 encoding, and GitLab API responses.

## 3. Why does the `.ino` file use `gitlab->uploadNewFile()`?

Now consider how your main program calls the function:

```cpp
gitlab->uploadNewFile(
    remotePath,
    bytes,
    length,
    commitMessage
);
```

The key is the `->` operator.

`gitlab` is a pointer to a `GitlabClient` object. The `->` operator accesses a member of the object to which that pointer refers.

Let's break this down.

### Step 1: Declare a pointer

Your initialization code may contain something similar to:

```cpp
GitlabClient* gitlab = nullptr;
```

This declares `gitlab` as a pointer to a `GitlabClient` object.

Initially:

```text
gitlab
  |
  v
nullptr
```

No object is assigned to the pointer yet.

### Step 2: Create the object

Your project uses dynamic allocation with `new`, along these lines:

```cpp
gitlab = new (std::nothrow) GitlabClient(tlsClient);
```

Assuming allocation succeeds, this creates a `GitlabClient` object and stores its address in `gitlab`.

Conceptually:

```text
gitlab pointer
     |
     v
+----------------------------+
| GitlabClient object        |
|                            |
| TLS client reference       |
| Configuration              |
| Authentication state       |
| Error information          |
+----------------------------+
```

The constructor receives `tlsClient`, allowing the object to use the TLS client for HTTPS communication.

The actual object members depend on your class implementation.

### Step 3: Call a member function through the pointer

You can now write:

```cpp
gitlab->uploadNewFile(
    remotePath,
    bytes,
    length,
    commitMessage
);
```

This is shorthand for:

```cpp
(*gitlab).uploadNewFile(
    remotePath,
    bytes,
    length,
    commitMessage
);
```

Both expressions call `uploadNewFile()` on the object pointed to by `gitlab`.

The operators have different purposes:

| Operator | Purpose | Example |
|---|---|---|
| `::` | Identifies which class or namespace a function belongs to. | `GitlabClient::uploadNewFile()` |
| `->` | Accesses a member through an object pointer. | `gitlab->uploadNewFile()` |
| `.` | Accesses a member directly through an object. | `client.uploadNewFile()` |

**Important:** `GitlabClient::uploadNewFile()` in the `.cpp` file defines the member function. It is not generally the way you call that non-static member function on a particular object.

For example, if you created an ordinary object instead of a pointer:

```cpp
GitlabClient client(tlsClient);

client.uploadNewFile(
    remotePath,
    bytes,
    length,
    commitMessage
);
```

You would use the `.` operator.

## 4. Why use a class for GitLab but simpler functions for PSRAM and NVS?

This is the most important architectural difference in your project.

A C++ module does not have to be a class. You can organize a module using standalone functions or a class, depending on its responsibilities.

### A. `GitlabClient`: a class that maintains state

Your GitLab client has several related responsibilities:

- Maintaining configuration, such as the project ID and branch.
- Using a TLS client to establish HTTPS connections.
- Managing authentication with the GitLab token.
- Uploading, downloading, updating, and deleting files.
- Reporting errors through `lastError()`.

These operations share configuration and state, making a class a natural choice.

For example, a simplified class might look like this:

```cpp
class GitlabClient {
private:
    String _token;
    String _branch;
    String _lastError;

public:
    bool begin(const String& token);

    bool uploadNewFile(
        const String& path,
        const uint8_t* data,
        size_t len,
        const String& commitMessage
    );

    bool downloadFileToPSRAM(
        const String& path,
        uint8_t** outBuffer,
        size_t* outLen
    );

    String lastError() const;
};
```

This is an illustration of the design, not a replacement for your existing header.

An object can maintain its own configuration and error information.

For example:

```cpp
gitlab->uploadNewFile(...);

if (!gitlab->lastError().isEmpty()) {
    Serial.println(gitlab->lastError());
}
```

The object can retain the error produced by its most recent operation.

### Why is this useful?

Suppose you eventually access two GitLab projects.

You could create two objects with different configurations:

```cpp
GitlabClient* clientA;
GitlabClient* clientB;
```

Each object could maintain its own project configuration and error state.

You would not have to redesign the module around a separate collection of global variables.

This is one advantage of object-oriented programming.

### B. `PSRamFileManager`: a collection of related operations

Your PSRAM file manager may perform operations such as:

- Checking PSRAM availability.
- Allocating memory buffers.
- Reading and writing file data in memory.
- Releasing allocated memory.

If these operations do not require a persistent object with its own configuration or state, standalone functions can be sufficient.

For example:

```cpp
// PSRamFileManager.h

bool initializePSRAM();

uint8_t* allocatePSRAM(size_t size);

void releasePSRAM(uint8_t* buffer);
```

The implementation could reside in `PSRamFileManager.cpp`:

```cpp
// Illustrative implementation

uint8_t* allocatePSRAM(size_t size) {
    return static_cast<uint8_t*>(ps_malloc(size));
}

void releasePSRAM(uint8_t* buffer) {
    free(buffer);
}
```

The main program can call these functions directly:

```cpp
uint8_t* buffer = allocatePSRAM(4096);

if (buffer != nullptr) {
    // Use the buffer.

    releasePSRAM(buffer);
}
```

Notice that there is no `GitlabClient`-style object pointer.

The functions are called directly because they are standalone functions.

The `ps_malloc()` and `free()` calls above illustrate the approach; use the actual function names and memory-management rules from your existing module.

### C. `NVSTokenManager`: functions for managing persistent storage

Your NVS token manager may provide functions to:

- Load the GitLab token from NVS.
- Provision a token through the Serial Monitor.
- Save the token.
- Check whether a token exists.

These can also be standalone functions if they operate on a known NVS namespace and key.

For example:

```cpp
// NVSTokenManager.h

bool nvs_loadGitlabToken(String& token);

bool nvs_provisionGitlabTokenFromSerial();
```

Your main program can call:

```cpp
String token;

if (nvs_loadGitlabToken(token)) {
    Serial.println("GitLab token loaded successfully.");
}
```

Again, these are illustrative signatures; retain the signatures defined in your actual project.

The NVS module can hide the storage details from the main program, just as `GitlabClient` hides GitLab API details.

### Comparison

| Characteristic | `GitlabClient` | `PSRamFileManager` | `NVSTokenManager` |
|---|---|---|---|
| Typical design | C++ class | Standalone functions | Standalone functions |
| Maintains object-specific state | Yes | Not necessarily | Not necessarily |
| Requires object construction | Yes, for the class design | No, if using standalone functions | No, if using standalone functions |
| Example definition | `GitlabClient::uploadNewFile()` | `allocatePSRAM()` | `nvs_loadGitlabToken()` |
| Example call | `gitlab->uploadNewFile()` | `allocatePSRAM()` | `nvs_loadGitlabToken(token)` |
| Main purpose | Communicate with GitLab | Manage memory buffers | Manage persistent credentials |

The distinction is not that GitLab is inherently more complex than PSRAM or NVS. Rather, the class design groups operations around an object that maintains related state, while standalone functions group operations by module.

You could implement PSRAM or NVS as classes too, if that provided a meaningful benefit.

## 5. How the three modules work together

Your `.ino` file acts as the coordinator.

A simplified initialization flow looks like this:

```text
              Main .ino
                  |
                  v
        +-------------------+
        | Initialize Wi-Fi  |
        +-------------------+
                  |
                  v
        +-------------------+
        | NVSTokenManager   |
        | Load GitLab token |
        +-------------------+
                  |
                  v
        +-------------------+
        | GitlabClient      |
        | Initialize HTTPS  |
        | and configuration |
        +-------------------+
                  |
                  v
        +-------------------+
        | PSRamFileManager  |
        | Allocate buffers  |
        +-------------------+
                  |
                  v
        +-------------------+
        | GitLab operation  |
        | Download/upload   |
        +-------------------+
```

This illustrates the roles, not a mandatory initialization order for every module. Your actual code determines which modules need to be initialized first.

For example, your existing GitLab initialization sequence loads the token from NVS before initializing the GitLab client.

When you upload a file:

```cpp
gitlab->uploadNewFile(
    remotePath,
    bytes,
    length,
    commitMessage
);
```

the main program passes the file path and data to the GitLab client. The client handles the HTTP request and GitLab API interaction.

The PSRAM manager, if used to prepare or store the file buffer, manages the memory independently. The NVS manager provides the persistent token.

Each module has a distinct responsibility.

## 6. Why do we need both `.h` and `.cpp` files?

Consider the declaration in `GitlabClient.h`:

```cpp
bool uploadNewFile(
    const String& repoFilePath,
    const uint8_t* data,
    size_t len,
    const String& commitMessage
);
```

This tells the compiler the function's interface, including its parameter types and return type.

The corresponding implementation in `GitlabClient.cpp`:

```cpp
bool GitlabClient::uploadNewFile(
    const String& repoFilePath,
    const uint8_t* data,
    size_t len,
    const String& commitMessage
) {
    // Validate input.
    // Encode data.
    // Construct the GitLab API request.
    // Send the request.
    // Record the result.
}
```

contains the actual logic.

The header allows the `.ino` file to call the function without knowing how it works internally.

The same separation applies to standalone functions in your PSRAM and NVS modules.

For example:

```cpp
#include "GitlabClient.h"
#include "PSRamFileManager.h"
#include "NVSTokenManager.h"
```

These include directives make the corresponding declarations available to the `.ino` file.

The `.cpp` files provide the implementations that the linker connects to the compiled program.

## 7. One important improvement: you do not always need `new`

Your current GitLab client uses dynamic allocation, apparently along these lines:

```cpp
gitlab = new (std::nothrow) GitlabClient(tlsClient);
```

This is valid, and it can be appropriate when you need dynamic object creation or optional allocation.

However, if the GitLab client is created once and used throughout the program, you could also construct it as a normal object.

For example, if the class supports the required constructor:

```cpp
GitlabClient gitlab(tlsClient);
```

You would then call:

```cpp
gitlab.uploadNewFile(
    remotePath,
    bytes,
    length,
    commitMessage
);
```

instead of:

```cpp
gitlab->uploadNewFile(
    remotePath,
    bytes,
    length,
    commitMessage
);
```

The second approach uses a pointer and dynamic allocation; the first uses a directly declared object.

Do not change your current implementation without checking its initialization requirements. Your existing design may intentionally allocate the object only after Wi-Fi and token initialization.

For your ESP32-S3, both approaches are valid. The choice depends on your lifecycle and memory-management requirements.

## Final takeaway

Remember these three distinctions:

```cpp
// 1. Class member function definition in GitlabClient.cpp
bool GitlabClient::uploadNewFile(...) {
    // Implementation
}

// 2. Call through a pointer to a GitlabClient object
gitlab->uploadNewFile(...);

// 3. Call a standalone function from another module
nvs_loadGitlabToken(token);
```

The first uses `::` to identify the class that owns the function. The second uses `->` to access a member through an object pointer. The third calls a standalone function directly.

For your modular ESP32-S3 project, the key design principle is to expose a clear interface in each header and keep implementation details inside the corresponding `.cpp` file. Whether a module uses a class or standalone functions should depend on whether it benefits from maintaining object-specific state—not simply on how complicated its implementation is.

# Understanding `tlsClient` in Your ESP32-S3 GitLab Project

In your ESP32-S3 GitLab file-access project, `tlsClient` is an object that provides the TLS-secured network connection used by `GitlabClient` to communicate with GitLab over HTTPS.

Let's break down this statement:

```cpp
gitlab = new (std::nothrow) GitlabClient(tlsClient);
```

There are three important parts:

- `new (std::nothrow)`: dynamically allocates memory for a `GitlabClient` object and returns `nullptr` if allocation fails.
- `GitlabClient(tlsClient)`: calls the `GitlabClient` constructor, passing `tlsClient` to it.
- `gitlab = ...`: stores the address of the newly created object in the `gitlab` pointer.

The key question is where `tlsClient` comes from. It must be declared and initialized somewhere before this statement executes.

## 1. Where is `tlsClient` declared?

In your project, it should be declared in the `.ino` file or another shared source file, typically as:

```cpp
WiFiClientSecure tlsClient;
```

This creates a `WiFiClientSecure` object named `tlsClient`.

Make sure the relevant header is included:

```cpp
#include <WiFiClientSecure.h>
```

If declared at global scope in your `.ino` file, it can be used by `initializeGitlab()` and other functions in that file.

## 2. How does it get passed into `GitlabClient`?

The flow is:

```text
Main .ino file
WiFiClientSecure tlsClient;
Creates the TLS client object
        |
        v
Passes a reference to the object
GitlabClient constructor
GitlabClient(tlsClient)
        |
        v
GitlabClient.cpp
Uses the TLS client for HTTPS requests to GitLab
```

For example, `GitlabClient.h` might declare the constructor like this:

```cpp
class GitlabClient {
private:
    WiFiClientSecure& _tls;

public:
    explicit GitlabClient(WiFiClientSecure& tlsClient);
};
```

And `GitlabClient.cpp` might define it like this:

```cpp
GitlabClient::GitlabClient(WiFiClientSecure& tlsClient)
    : _tls(tlsClient) {
}
```

Here, `_tls` is a reference to the original TLS client, not a separate copy.

When `GitlabClient` needs to make an HTTPS request, it can use `_tls` with `HTTPClient`.

For example:

```cpp
HTTPClient http;

if (http.begin(_tls, url)) {
    // Configure and perform the HTTP request.
    http.end();
}
```

This is illustrative; use your existing implementation's member names and request-handling logic.

## 3. Where does the CA certificate fit in?

Your GitLab client also needs to configure the TLS client with the appropriate trusted root CA certificate, for example:

```cpp
tlsClient.setCACert(GITLAB_ROOT_CA);
```

This configures certificate verification for the TLS connection. Your `GitlabConfig` may instead pass the CA certificate to `GitlabClient::begin()`, which configures the client internally.

The distinction is:

- `tlsClient` provides the secure network connection.
- The CA certificate establishes which certificate authority is trusted.
- The GitLab token authenticates your request to the GitLab API.

These three things have different purposes.

## 4. One important lifetime rule

Because `GitlabClient` may retain a reference to `tlsClient`, the TLS object must remain alive for as long as the GitLab client uses it.

A global declaration is suitable:

```cpp
WiFiClientSecure tlsClient;
GitlabClient* gitlab = nullptr;
```

Then, after the required setup:

```cpp
gitlab = new (std::nothrow) GitlabClient(tlsClient);
```

In summary: `tlsClient` is the HTTPS transport object supplied to `GitlabClient`. In your project, check the `.ino` file for `WiFiClientSecure tlsClient;`, and check `GitlabClient.h` and `.cpp` for the constructor that receives and stores it.

# How `gitlab->` Accesses `GitlabClient` Member Functions

The statement

```cpp
gitlab = new (std::nothrow) GitlabClient(tlsClient);
```

creates **one** `GitlabClient` object. It does not automatically link every function to the object. The class declaration in `GitlabClient.h` defines which functions belong to that class. The object can then access those public member functions through `gitlab->`.

Let's walk through the relationship using your actual functions.

## 1. The relationship between the class, object and functions

<img width="70%" height="auto" alt="image" src="https://github.com/user-attachments/assets/53dae025-5390-4210-b80e-893fe318e628" />


```text
GitlabClient class
The blueprint declared in GitlabClient.h

begin()
listFiles()
downloadFileToPSRAM()
uploadNewFile()
updateFile()
deleteRemoteFile()
lastError()
    |
    v
Construct an object with tlsClient

gitlab pointer
GitlabClient* gitlab
Points to the created object in memory
    |
    v
Call public member functions
gitlab->begin(...)
gitlab->listFiles(...)
gitlab->uploadNewFile(...)
```

The class defines the available member functions. The pointer `gitlab` identifies which instance of that class you want to use.

## 2. What happens during object creation?

Suppose you have:

```cpp
GitlabClient* gitlab = nullptr;

gitlab = new (std::nothrow) GitlabClient(tlsClient);
```

There are three steps.

### Step 1 — `GitlabClient* gitlab`

Declares a pointer that can hold the address of a `GitlabClient` object.

### Step 2 — `new (std::nothrow) GitlabClient(tlsClient)`

Allocates memory for one object and invokes its constructor, passing `tlsClient` to it.

### Step 3 — Assign the returned pointer to `gitlab`

If allocation succeeds, `gitlab` points to the new object. If allocation fails, `gitlab` becomes `nullptr`.

The object contains its own instance data, such as the TLS client reference, configuration and error string, depending on your class implementation.

The member functions are compiled program code; they are not copied into the object every time you create an instance. The object stores the state those functions operate on.

## 3. How do all your functions become accessible through `gitlab->`?

They must be declared as **public member functions** inside the `GitlabClient` class.

For example, your header might contain:

```cpp
class GitlabClient {
public:
    bool begin(const GitlabConfig& config,
               const String& accessToken);

    bool listFiles(const String& path,
                   std::vector<String>& outFileNames,
                   bool recursive = false);

    bool downloadFileToPSRAM(const String& repoFilePath,
                             uint8_t** outBuffer,
                             size_t* outLen);

    bool uploadNewFile(
        const String& repoFilePath,
        const uint8_t* data,
        size_t len,
        const String& commitMessage =
            "Upload file from ESP32-S3");

    bool updateFile(
        const String& repoFilePath,
        const uint8_t* data,
        size_t len,
        const String& commitMessage =
            "Update file from ESP32-S3");

    bool deleteRemoteFile(
        const String& repoFilePath,
        const String& commitMessage =
            "Delete file from ESP32-S3");

    String lastError() const {
        return _lastError;
    }

private:
    String _lastError;

    // Other private members, such as TLS client and configuration.
};
```

This is a simplified illustration of the class structure; retain your existing private members and declarations.

Because these functions are under `public:`, the main program can call them through the object pointer.

For example:

```cpp
gitlab->begin(config, token);

gitlab->listFiles("/", files, false);

gitlab->uploadNewFile(
    "notes.txt",
    data,
    length,
    "Upload notes"
);

Serial.println(gitlab->lastError());
```

Each call invokes the corresponding function on the object pointed to by `gitlab`.

**Important:** Creating the object does not automatically execute `begin()`. You still need to call `begin(config, token)` to initialize the GitLab client.

## 4. How does the compiler connect the `.h` and `.cpp` files?

Your `GitlabClient.h` declares the class and its member functions. Your `GitlabClient.cpp` implements those functions.

For example, the header declares:

```cpp
bool uploadNewFile(
    const String& repoFilePath,
    const uint8_t* data,
    size_t len,
    const String& commitMessage
);
```

The `.cpp` file defines the same member function:

```cpp
bool GitlabClient::uploadNewFile(
    const String& repoFilePath,
    const uint8_t* data,
    size_t len,
    const String& commitMessage
) {
    // Actual GitLab upload implementation
}
```

The `GitlabClient::` scope qualifier tells the compiler that the definition belongs to the `GitlabClient` class.

During compilation and linking, the compiler and linker associate the declaration with its implementation. The call:

```cpp
gitlab->uploadNewFile(...);
```

then invokes that implementation on the selected object.

A function declared in the header but never implemented will generally cause a linker error if it is used and no other matching definition is available.

## 5. Does `gitlab->` work for every function in the project?

No. It works for accessible members of the `GitlabClient` object, not arbitrary functions.

| Function | Can call using `gitlab->`? | Reason |
|---|---|---|
| `begin()` | Yes | Public member of `GitlabClient` |
| `listFiles()` | Yes | Public member of `GitlabClient` |
| `downloadFileToPSRAM()` | Yes | Public member of `GitlabClient` |
| `uploadNewFile()` | Yes | Public member of `GitlabClient` |
| `nvs_loadGitlabToken()` | No, not normally | Standalone NVS function, not a GitLab client member |
| `allocatePSRAM()` | No, not normally | Standalone PSRAM function, not a GitLab client member |

For a standalone function, you call it directly:

```cpp
nvs_loadGitlabToken(token);
```

You would only call `gitlab->someFunction()` if `someFunction()` were actually declared as an accessible member of `GitlabClient`.

## 6. One final detail: `lastError() const`

Your declaration:

```cpp
String lastError() const {
    return _lastError;
}
```

is a member function implemented directly inside the header's class declaration.

The trailing `const` means this member function promises not to modify the object's member state through the ordinary `this` pointer.

It returns a `String` containing the error message, so you can write:

```cpp
Serial.println(gitlab->lastError());
```

The function itself is available through the same object pointer as your other public members.

## In summary

- `GitlabClient` is the class that declares the available member functions.
- `new (std::nothrow) GitlabClient(tlsClient)` constructs one object.
- `gitlab` stores a pointer to that object.
- `gitlab->functionName()` invokes a public member function on that object.
- The `.cpp` file provides the implementations for functions declared in the header.
- The object's state is stored per instance; the compiled function code is generally shared.

So yes, all seven functions you listed can be accessed through `gitlab->` provided they are declared as public members of `GitlabClient` and their implementations are correctly linked.

# Understanding `explicit` in the `GitlabClient` Constructor

Yes, `explicit` is a C++ keyword that can be used with a constructor. In your `GitlabClient.h`:

```cpp
explicit GitlabClient(WiFiClientSecure& tlsClient);
```

This is a constructor declaration, not its implementation. Let's break it down.

## 1. Understanding each part

| Syntax | Meaning |
|---|---|
| `explicit` | Prevents the constructor from being used for certain implicit conversions. |
| `GitlabClient` | The constructor has the same name as the class. |
| `WiFiClientSecure&` | Accepts a reference to an existing `WiFiClientSecure` object. |
| `tlsClient` | The parameter name, referring to the supplied TLS client. |
| `;` | Ends the declaration; the implementation is elsewhere. |

## 2. Where is the constructor implemented?

In `GitlabClient.cpp`, you would typically define it like this:

```cpp
#include "GitlabClient.h"

GitlabClient::GitlabClient(WiFiClientSecure& tlsClient)
    : _tls(tlsClient)
{
}
```

This assumes your header declares a reference member named `_tls`:

```cpp
class GitlabClient {
public:
    explicit GitlabClient(WiFiClientSecure& tlsClient);

private:
    WiFiClientSecure& _tls;
};
```

The important part is:

```cpp
: _tls(tlsClient)
```

This is the **member initializer list**. It initializes `_tls` as a reference to the existing object passed into the constructor. It does not create another `WiFiClientSecure` object or copy the existing one.

For example, in your `.ino` file:

```cpp
WiFiClientSecure tlsClient;

GitlabClient* gitlab = nullptr;

gitlab = new (std::nothrow) GitlabClient(tlsClient);
```

The relationship is:

<img width="70%" height="auto" alt="image" src="https://github.com/user-attachments/assets/f7fd1750-d8bc-490a-9590-9abbf25dd18c" />


```text
.ino file
WiFiClientSecure tlsClient;
Creates the TLS client object
Passes a reference to the constructor

GitlabClient object
_tls references tlsClient
Uses that TLS client for secure HTTPS communication
```

The TLS object must remain alive for as long as `GitlabClient` uses its reference.

## 3. Why use `explicit`?

Consider a constructor without `explicit`:

```cpp
class GitlabClient {
public:
    GitlabClient(WiFiClientSecure& tlsClient);
};
```

C++ could permit certain implicit conversions, such as:

```cpp
WiFiClientSecure tlsClient;

GitlabClient client = tlsClient;
```

Here, the compiler could implicitly construct a `GitlabClient` from `tlsClient`.

With `explicit`:

```cpp
explicit GitlabClient(WiFiClientSecure& tlsClient);
```

That implicit construction is prohibited. You must construct the object directly:

```cpp
GitlabClient client(tlsClient);
```

Or use your dynamic allocation pattern:

```cpp
GitlabClient* gitlab =
    new (std::nothrow) GitlabClient(tlsClient);
```

Both are valid.

## 4. Does `explicit` affect `newGitlab`?

No. It does not prevent you from creating another object or assigning another pointer.

```cpp
GitlabClient* newGitlab =
    new (std::nothrow) GitlabClient(tlsClient);
```

This remains valid because you explicitly call the constructor with `tlsClient`.

**My recommendation:** Keep `explicit` in your `GitlabClient` constructor. It makes the class's construction intentional and helps prevent accidental implicit conversions. It does not change how the TLS reference works or how you call methods such as `gitlab->listFiles(...)`.

# Reusing `GitlabClient` for ESP32-S3 OTA Firmware Updates

You can reuse the `GitlabClient` class structure for your previous ESP32-S3 OTA firmware-update project, especially because both projects need to communicate with GitLab over HTTPS using a TLS client and an authentication token.

However, there is an important distinction: you can reuse the TLS connection, GitLab authentication, and file-download infrastructure, but firmware updating should remain a separate responsibility from ordinary repository file access.

## 1. What can be reused?

Your current architecture already has most of the building blocks.

<img width="75%" height="auto" alt="image" src="https://github.com/user-attachments/assets/fe7e3cf9-48c4-4140-ac11-5e3f3ae85125" />


```text
Shared infrastructure
WiFiClientSecure · CA verification · GitLab authentication · HTTP requests

GitLab file access
Download, upload, update and delete repository files.

OTA updater
Verify firmware authenticity and integrity, then install and reboot.
```

| Component | Reuse? | Notes |
|---|---|---|
| `WiFiClientSecure` and trusted CA | Yes | Reuse the secure transport configuration. |
| GitLab token from NVS | Yes | Reuse the token manager and permission checks. |
| GitLab project/branch configuration | Usually | Can share configuration if both projects use the same repository and branch. |
| HTTP GET and response handling | Yes, with care | Firmware may be much larger than ordinary files, so streaming is preferable. |
| `downloadFileToPSRAM()` | Not ideal for firmware | It buffers the entire file in PSRAM. Your firmware is better streamed into the OTA partition. |
| Firmware signature and hash verification | Keep in OTA module | These are security-critical OTA responsibilities. |
| Arduino `Update` and reboot logic | Keep in OTA module | Repository file access should not be responsible for installing firmware. |

## 2. Recommended class structure

For your existing ESP32-S3 N16R8 project, I would keep `GitlabClient` as the shared network-access layer and create a separate `OTAUpdater` class.

```text
ESP32-S3 Application (.ino)
          |
          +-----------------------+
          |                       |
          v                       v
    GitlabClient              OTAUpdater
          |                       |
          |                       +--> Download manifest
          |                       +--> Verify RSA signature
          |                       +--> Verify SHA-256
          |                       +--> Stream firmware
          |                       +--> Arduino Update
          |                       +--> Reboot
          |
          +--> WiFiClientSecure
          +--> GitLab authentication
          +--> HTTP request handling
          +--> Repository file operations
          |
          v
      GitLab Server
```

The two classes can share the same TLS client, but `OTAUpdater` should handle the firmware installation workflow.

### Example of how the classes might be connected

In your main `.ino` file:

```cpp
WiFiClientSecure tlsClient;

GitlabClient* gitlab = nullptr;
OTAUpdater* ota = nullptr;
```

After Wi-Fi and token initialization:

```cpp
gitlab = new (std::nothrow) GitlabClient(tlsClient);

if (gitlab != nullptr) {
    // Initialize GitlabClient with your existing configuration and token.
    // Then initialize OTAUpdater using the shared GitlabClient.
}
```

The OTA class could receive a reference to the GitLab client:

```cpp
class OTAUpdater {
public:
    explicit OTAUpdater(GitlabClient& client);

    bool checkAndInstallUpdate();

private:
    GitlabClient& _gitlab;
};
```

This is a proposed interface, not code that can be dropped into your existing project without implementing the corresponding methods.

The benefit is that `OTAUpdater` can reuse the GitLab client's authenticated download capabilities without duplicating the connection and token-management logic.

## 3. One important issue: repository files versus firmware packages

Your current GitLab file-access module accesses repository files. Your previous OTA project used the GitLab Generic Package Registry for firmware distribution.

These use different API endpoints.

- **Repository files:** GitLab Repository Files API or raw-file endpoint.
- **OTA firmware:** Generic Package Registry endpoint, such as `/api/v4/projects/{id}/packages/generic/...`.

Therefore, the current `downloadFileToPSRAM()` method may not directly retrieve firmware from the package URL without modification. Its endpoint construction and authentication handling need to support the package registry.

I would add a separate download method for package artifacts rather than complicating your existing repository-file functions.

## 4. Why firmware should not be downloaded entirely into PSRAM

Your ESP32-S3 has 8 MB of PSRAM, but buffering a full firmware image is unnecessary. Your existing OTA approach can stream the firmware response into the inactive OTA partition using Arduino's `Update` API.

A secure OTA workflow should:

1. Fetch the signed manifest over verified HTTPS.
2. Verify the manifest signature using your trusted public key.
3. Check the product, firmware version, expected image size and SHA-256.
4. Stream firmware into the inactive OTA partition while calculating the hash.
5. Verify the final image size and hash, and ensure the image is valid.
6. Finalize the update and reboot only after all checks succeed.

HTTPS, a GitLab token and SHA-256 alone do not establish firmware authenticity. Retain your existing RSA signature verification and secure-boot protections where configured.

## My recommendation

Reuse the existing `GitlabClient` for transport and authentication, then add an `OTAUpdater` module that handles signed manifests, firmware streaming and installation. Keep `NVSTokenManager` and your PSRAM utilities independent.

This gives you one shared GitLab connection layer for repository file management and OTA, while preserving a clear separation between ordinary file operations and
