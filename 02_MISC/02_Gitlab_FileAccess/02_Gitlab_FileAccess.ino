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
static const char* WIFI_SSID = "Nightingale_IoT";
static const char* WIFI_PASSWORD = "1122334455";

static const char* PROJECT_ID = "88068597";  //"88009717";
static const char* BRANCH = "main";

static constexpr size_t MAX_TEXT_INPUT_BYTES = 8UL * 1024UL;
static constexpr size_t MAX_TEXT_UPLOAD_BYTES = 256UL * 1024UL;


WiFiClientSecure tlsClient;
GitlabClient* gitlab = nullptr;

static void runMemoryManagerDemo() {
  Serial.println("[MemoryDemo] Testing PSRAM file operations...");
  PsramFile demo;
  const uint8_t first[] = { 0x00, 0x41, 0xFF };
  const uint8_t extra[] = { 0x10, 0x20 };
  const uint8_t replacement[] = { 0xDE, 0xAD, 0xBE, 0xEF };

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
  Serial.println(" LIST /              ->list root-level files");
  Serial.println(" LIST folder         ->list files in assets directory");
  Serial.println(" GET folder/logo.png ->download file to PSRAM, print SHA-256, free buffer");
  Serial.println(" TEXTUPLOAD          ->Prompt for text and create, overwrite, or append a GitLab file");
  Serial.println(" UPLOAD remote/path /localfile  ->create a NEW remote file from LittleFS");
  Serial.println(" UPDATE remote/path /localfile  ->update an EXISTING remote file");
  Serial.println(" RM remote/path      ->delete a remote repository file (irreversible commit)");
  Serial.println(" DEL token           ->delete GitLab PAT from NVS");
  Serial.println(" HELP                ->show this help");
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
  token = "";  // drop this local copy as soon as possible
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

  // Print contents if the file extension is .txt (case-insensitive).
  String lowerPath = path;
  lowerPath.toLowerCase();

  if (lowerPath.endsWith(".txt")) {
    Serial.println("\n========== FILE CONTENT BEGIN ==========");

    // Write the exact bytes from PSRAM; no null terminator required.
    Serial.write(data, len);

    // Ensure the closing marker starts on a new line.
    if (len == 0 || data[len - 1] != '\n') {
      Serial.println();
    }

    Serial.println("=========== FILE CONTENT END ===========");
  }

  // Transfer ownership of the PSRAM buffer to PsramFile.
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


static bool uploadLittleFSFile(const String& remotePath, const String& localPath, bool update) {
  if (!gitlab && !initializeGitlab()) return false;
  if (!LittleFS.begin(false)) {
    Serial.println("[LittleFS] Mount failed.");
    return false;
  }
  if (localPath.length() == 0 || !localPath.startsWith("/")) {
    Serial.println("Local source must be an absolute LittleFS path, e.g. /upload.bin");
    return false;
  }
  File file = LittleFS.open(localPath, "r");
  if (!file || file.isDirectory()) {
    Serial.println("[LittleFS] Cannot open local source file.");
    if (file) file.close();
    return false;
  }
  size_t len = file.size();
  if (len == 0 || len > 256UL * 1024UL) {
    Serial.println("[Upload] Source file must be 1 byte to 256 KiB.");
    file.close();
    return false;
  }
  uint8_t* data = (uint8_t*)ps_malloc(len);
  if (!data) {
    Serial.println("[Upload] PSRAM allocation failed.");
    file.close();
    return false;
  }
  size_t got = file.read(data, len);
  file.close();
  if (got != len) {
    free(data);
    Serial.println("[Upload] Failed to read complete local file.");
    return false;
  }

  String commit = String(update ? "Update " : "Upload ") + remotePath + " from ESP32-S3";
  bool ok = update
              ? gitlab->updateFile(remotePath, data, len, commit)
              : gitlab->uploadNewFile(remotePath, data, len, commit);
  free(data);
  if (!ok) Serial.println(gitlab->lastError());
  return ok;
}

static void runRemoteDelete(const String& remotePath) {
  if (!gitlab && !initializeGitlab()) return;
  if (remotePath.length() == 0) {
    Serial.println("Usage: RM path/to/remote-file");
    return;
  }
  Serial.printf("[WARNING] Deleting remote GitLab file: %s\\n", remotePath.c_str());
  if (gitlab->deleteRemoteFile(remotePath, "Delete " + remotePath + " from ESP32-S3")) {
    Serial.println("[GitLab] Remote file deleted.");
  } else {
    Serial.println(gitlab->lastError());
  }
}
//********  BEGIN: Helper functions  ***********************************
// Serial-to-GitLab Text Upload Command
//***************************************************************


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

      // Echo to serial port
      Serial.println(out);

      return true;
    }

    if (c == '\r') {
      skipNextLF = true;

      // Echo to serial port
      Serial.println(out);

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

  if (remotePath.isEmpty() || remotePath.startsWith("/") || remotePath.indexOf("..") >= 0) {
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
    if (!finalText.reserve(newText.length() + 1) || !finalText.concat(newText) || finalText.length() != newText.length()) {
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
      if (!finalText.reserve(newText.length() + 1) || !finalText.concat(newText) || finalText.length() != newText.length()) {
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
      if (oldLen > MAX_TEXT_UPLOAD_BYTES || separatorBytes > MAX_TEXT_UPLOAD_BYTES - oldLen || newText.length() > MAX_TEXT_UPLOAD_BYTES - oldLen - separatorBytes) {
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

//********  END: Helper functions  ***********************************
// Serial-to-GitLab Text Upload Command
//***************************************************************


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
  } else if (command == "UPLOAD" || command == "UPDATE") {
    int secondSplit = argument.indexOf(' ');
    if (secondSplit < 0) {
      Serial.println("Usage: UPLOAD|UPDATE remote/path /localfile");
      return;
    }
    String remotePath = argument.substring(0, secondSplit);
    String localPath = argument.substring(secondSplit + 1);
    remotePath.trim();
    localPath.trim();
    if (remotePath.length() == 0 || localPath.length() == 0) {
      Serial.println("Usage: UPLOAD|UPDATE remote/path /localfile");
      return;
    }
    bool ok = uploadLittleFSFile(remotePath, localPath, command == "UPDATE");
    if (ok) Serial.println("[GitLab] File operation completed.");
  } else if (command == "RM") {
    runRemoteDelete(argument);
  } else if (command == "GET") {
    if (argument.length() == 0) {
      Serial.println("Usage: GET path/to/file");
      return;
    }
    runGet(argument);
  } else if (command == "TEXTUPLOAD") {
    runTextUpload();

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
