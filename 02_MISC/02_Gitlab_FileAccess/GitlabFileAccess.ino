#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <mbedtls/sha256.h>

#include "GitlabRootCA.h"
#include "NvsTokenManager.h"
#include "GitlabClient.h"
#include "PsramFileManager.h"

// Demo convenience only: for a real/public repository, move these to a local
// secrets header excluded from source control. Never put the GitLab PAT here.
static const char* WIFI_SSID = "YOUR_WIFI_SSID";
static const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

static const char* PROJECT_ID = "PUT_YOUR_PROJECT_ID_OR_group%2Fproject_HERE";
static const char* BRANCH = "main";

WiFiClientSecure tlsClient;
GitlabClient* gitlab = nullptr;

static void runMemoryManagerDemo() {
  Serial.println("[MemoryDemo] Testing PSRAM file operations...");
  PsramFile demo;
  const uint8_t first[] = {0x00, 0x41, 0xFF};
  const uint8_t extra[] = {0x10, 0x20};
  const uint8_t replacement[] = {0xDE, 0xAD, 0xBE, 0xEF};

  if (!fileNew(demo, "memory-demo.bin", sizeof(first))) return;
  memcpy(demo.data, first, sizeof(first));
  if (!fileAppend(demo, (uint8_t*)extra, sizeof(extra))) {
    fileDelete(demo);
    return;
  }
  Serial.printf("[MemoryDemo] After append: %u bytes\\n", (unsigned)demo.len);
  if (!fileOverwrite(demo, (uint8_t*)replacement, sizeof(replacement))) {
    fileDelete(demo);
    return;
  }
  Serial.printf("[MemoryDemo] After overwrite: %u bytes\\n", (unsigned)demo.len);
  fileDelete(demo);
  Serial.println("[MemoryDemo] fileDelete completed.");
}

static void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  LIST /              list root-level files");
  Serial.println("  LIST assets         list files in assets directory");
  Serial.println("  GET assets/logo.png download file to PSRAM, print SHA-256, free buffer");
  Serial.println("  DEL token           delete GitLab PAT from NVS");
  Serial.println("  HELP                show this help");
}

static bool connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return true;
  if (String(WIFI_SSID) == "YOUR_WIFI_SSID") {
    Serial.println("[WiFi] Edit WIFI_SSID and WIFI_PASSWORD first.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WiFi] Connecting");
  uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 30000) {
    Serial.print('.');
    delay(500);
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Connection failed.");
    return false;
  }
  Serial.print("[WiFi] Connected. IP=");
  Serial.println(WiFi.localIP());
  return true;
}

static void printSha256(const uint8_t* data, size_t len) {
  uint8_t digest[32];
  mbedtls_sha256(data, len, digest, 0);
  Serial.print("[SHA-256] ");
  for (size_t i = 0; i < sizeof(digest); ++i) {
    if (digest[i] < 16) Serial.print('0');
    Serial.print(digest[i], HEX);
  }
  Serial.println();
}

static bool initializeGitlab() {
  if (!connectWiFi()) return false;

  String token = nvs_loadGitlabToken();
  if (token.length() == 0) {
    token = "";
    if (!nvs_provisionGitlabTokenFromSerial()) return false;
    token = nvs_loadGitlabToken();
  }
  if (token.length() == 0) {
    Serial.println("[GitLab] No token available.");
    return false;
  }

  GitlabConfig cfg;
  cfg.gitlabUrl = "https://gitlab.com";
  cfg.projectId = PROJECT_ID;
  cfg.branch = BRANCH;
  cfg.rootCA = String(GITLAB_ROOT_CA);

  // Keep token only for this client lifetime; never log it.
  if (gitlab) {
    delete gitlab;
    gitlab = nullptr;
  }
  gitlab = new (std::nothrow) GitlabClient(tlsClient);
  if (!gitlab) {
    Serial.println("[GitLab] Failed to allocate client.");
    token = "";
    return false;
  }

  bool ok = gitlab->begin(cfg, token);
  token = ""; // drop this local copy as soon as possible
  if (!ok) {
    Serial.println(gitlab->lastError());
    return false;
  }
  Serial.println("[GitLab] Client initialized.");
  return true;
}

static void runList(String path) {
  if (!gitlab && !initializeGitlab()) return;
  path.trim();
  if (path == "/") path = "";

  std::vector<String> names;
  if (!gitlab->listFiles(path, names, false)) {
    Serial.println(gitlab->lastError());
    return;
  }
  Serial.printf("[GitLab] Files in '%s': %u\n", path.c_str(), (unsigned)names.size());
  for (const String& name : names) Serial.println(name);
}

static void runGet(const String& path) {
  if (!gitlab && !initializeGitlab()) return;

  uint8_t* data = nullptr;
  size_t len = 0;
  if (!gitlab->downloadFileToPSRAM(path, &data, &len)) {
    Serial.println(gitlab->lastError());
    return;
  }

  Serial.printf("[GET] Downloaded '%s': %u bytes\n", path.c_str(), (unsigned)len);
  printSha256(data, len);

  // Demonstrate the PsramFileManager ownership model without copying the
  // payload: take ownership of the download buffer and free it via fileDelete.
  PsramFile downloaded;
  downloaded.name = path;
  downloaded.data = data;
  downloaded.len = len;
  data = nullptr;

  // Optional persistence example:
  // if (fileSaveToLittleFS(downloaded, "/download.bin"))
  //   Serial.println("[LittleFS] Saved /download.bin");

  fileDelete(downloaded);
  Serial.println("[GET] Buffer released.");
}

static void handleCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  int split = line.indexOf(' ');
  String command = split < 0 ? line : line.substring(0, split);
  String argument = split < 0 ? "" : line.substring(split + 1);
  command.toUpperCase();
  argument.trim();

  if (command == "HELP") {
    printHelp();
  } else if (command == "DEL" && argument.equalsIgnoreCase("token")) {
    nvs_clearGitlabToken();
    if (gitlab) {
      delete gitlab;
      gitlab = nullptr;
    }
    Serial.println("[Security] Reboot to provision a new token.");
  } else if (command == "LIST") {
    runList(argument.length() ? argument : "/");
  } else if (command == "GET") {
    if (argument.length() == 0) {
      Serial.println("Usage: GET path/to/file");
      return;
    }
    runGet(argument);
  } else {
    Serial.println("Unknown command. Type HELP.");
  }
}

void setup() {
  Serial.begin(115200);
  uint32_t waitStart = millis();
  while (!Serial && millis() - waitStart < 4000) delay(10);
  delay(200);

  Serial.println();
  Serial.println("ESP32-S3 Secure GitLab File Access");
  Serial.printf("Chip: %s, cores: %u\n", ESP.getChipModel(), ESP.getChipCores());

  bool psramOk = psramInit();
  Serial.printf("psramInit(): %s\n", psramOk ? "true" : "false");
  Serial.printf("psramFound(): %s\n", psramFound() ? "true" : "false");
  Serial.printf("PSRAM total: %u bytes, free: %u bytes\n",
                (unsigned)ESP.getPsramSize(), (unsigned)ESP.getFreePsram());
  Serial.printf("Internal free heap: %u bytes\n", (unsigned)ESP.getFreeHeap());

  if (!LittleFS.begin(false)) {
    Serial.println("[LittleFS] Mount failed. No auto-format performed.");
    Serial.println("Check partition scheme and filesystem contents.");
  } else {
    Serial.printf("[LittleFS] Mounted; total=%u, used=%u bytes\n",
                  (unsigned)LittleFS.totalBytes(), (unsigned)LittleFS.usedBytes());
  }

  // Prompt for a PAT only after Serial is available. Network setup occurs when
  // a LIST/GET command is issued, so the user can first inspect the setup.
  if (!nvs_hasGitlabToken()) {
    Serial.println("[NVS] Token is not provisioned yet.");
    nvs_provisionGitlabTokenFromSerial();
  }

  runMemoryManagerDemo();
  printHelp();
}

void loop() {
  static String input;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      if (input.length() > 0) {
        handleCommand(input);
        input = "";
      }
    } else if (input.length() < 256) {
      input += c;
    } else {
      input = "";
      Serial.println("[Serial] Command too long; cleared.");
    }
  }
  delay(5);
}
