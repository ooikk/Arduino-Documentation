# ESP32-S3 N16R8 GitLab File Access

Arduino IDE 2.x / Espressif Arduino-ESP32 Core 3.x. Project modules:
- `GitlabFileAccess.ino`
- `NvsTokenManager.h/.cpp`
- `GitlabClient.h/.cpp`
- `PsramFileManager.h/.cpp`
- `GitlabRootCA.h` (paste a verified root CA PEM here)

## Arduino IDE setup
1. Install Arduino IDE 2.x.
2. File > Preferences > Additional Boards Manager URLs:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. Tools > Board > Boards Manager: install/update **esp32 by Espressif Systems**, version 3.x.
4. Select **ESP32S3 Dev Module** (or the exact ESP32-S3 DevKitC-1 entry if available).
5. Configure Tools:
   - Flash Size: 16MB (or 16MB for N16R8)
   - PSRAM: OPI PSRAM
   - Partition Scheme: 16MB with OTA, or 16M Flash (3MB APP/9.9MB FATFS) if that is the scheme shown by your installed core
   - USB CDC On Boot: Enabled is convenient for native USB serial, but use the port that actually appears on your PC
6. Install **ArduinoJson** from Library Manager. `Preferences`, `LittleFS`, `WiFi`, `WiFiClientSecure`, and `HTTPClient` are provided by the ESP32 Arduino core; do not install separate libraries for these.
7. Copy all files in this folder into one Arduino sketch folder named `GitlabFileAccess`. Open `GitlabFileAccess.ino`.

## Root CA is intentionally a separate file
Open `GitlabRootCA.h`. Replace its placeholder with the PEM-encoded **trusted root CA that validates the certificate chain presented by `gitlab.com` on your network**. Do not paste the GitLab leaf/server certificate and do not disable verification. The certificate chain can change; verify it with a trusted browser/OS certificate viewer or a trusted CA source before embedding it. `WiFiClientSecure::setCACert()` expects PEM text beginning `-----BEGIN CERTIFICATE-----`.

If GitLab's serving chain changes, update this CA file only after verifying the new chain. TLS failure is safer than silently disabling validation.

## GitLab token
Create a narrowly scoped token with `read_api` and `read_repository`, and access only a project the token is authorized to read. For a project access token, ensure its role has repository read access. Never commit the token or share it in screenshots/logs.
On first boot, the sketch prompts `Enter GitLab PAT:`. Enter the token followed by Enter. Do not paste it into source code. Serial terminal local echo may show what you type; disable local echo if your terminal supports it.

`Preferences` stores the token in NVS, but this does **not** mean the NVS contents are encrypted. See the NVS encryption notes below.

## Configure project
Edit in `GitlabFileAccess.ino`:
- `WIFI_SSID` and `WIFI_PASSWORD` (demo Wi-Fi credentials are compile-time strings; use a secrets header excluded from version control for real projects)
- `PROJECT_ID` (numeric GitLab project ID or URL-encoded path such as `group%2Fproject`)
- `BRANCH` (for example `main`)

The Wi-Fi credentials are intentionally separate from the GitLab PAT. For production, keep all credentials out of public source control.

## PSRAM checks
The sketch calls `psramInit()` and checks `psramFound()`. Select `OPI PSRAM` in Tools. `psramFound()` is a detection result; `psramInit()` attempts initialization. The code prints available heap and PSRAM capacity, but leaves sufficient memory for networking and application tasks.

## Serial commands
- `LIST /` — list files and directories at repository root (the implementation outputs file names only)
- `LIST assets` — list files in `assets`
- `GET assets/logo.png` — download the raw file into PSRAM, print byte count and SHA-256, then free the temporary buffer
- `DEL token` — delete the saved PAT from NVS; reboot/re-enter a token to continue
- `HELP` — show commands

Use a newline-terminated command. `GET` is binary-safe; it never treats the downloaded file as a `String`.

## Notes / limits
- The API tree endpoint returns entries for one directory unless `recursive=true`. Pagination uses `page` and `per_page=100`.
- A missing path can return HTTP 404. A 401/403 normally means invalid/expired token or insufficient permission.
- Files are downloaded to a dynamically grown PSRAM allocation. A maximum file size is configured in `GitlabClient.h`; adjust it to suit the project and available memory. Very large files may need a streaming-to-LittleFS or SD implementation rather than full in-memory buffering.
- The raw file endpoint returns exact bytes. Git LFS files may return an LFS pointer unless `lfs=true` is supported/used by the server and endpoint. This sample does not follow LFS objects.
- The class is designed for a single request at a time and is not thread-safe.

## NVS encryption: later hardening
This sketch does not enable NVS encryption. For production:
1. Use an ESP-IDF project/configuration based on the exact ESP-IDF version bundled with your Arduino-ESP32 Core.
2. Configure NVS encryption and the required key-protection mechanism (commonly flash encryption / secure boot as part of a production security design).
3. Provision keys securely, test recovery and factory-reset procedures, and understand that enabling encryption can require erasing/reprovisioning NVS.
4. Do not assume `Preferences.begin()` itself encrypts values. Consult Espressif's current NVS Encryption and Flash Encryption documentation for the selected IDF version.
5. Secure Boot and flash encryption are separate controls and should be planned together; do not enable irreversible eFuses until the recovery and manufacturing process is validated.

## Integrity vs authenticity
TLS validates the server's certificate chain and hostname; it does not guarantee a file is the expected release. For firmware or safety-relevant assets, compare the downloaded bytes against a SHA-256 digest delivered through a trusted authenticated manifest, or verify a digital signature whose public key is pinned in firmware. A digest fetched from the same compromised source is not a sufficient authenticity check by itself.
