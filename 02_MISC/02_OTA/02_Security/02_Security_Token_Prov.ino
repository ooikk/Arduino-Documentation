#include <Arduino.h>
#include <Preferences.h>

// Must match the namespace and key used by your OTA module.
static const char* NVS_NAMESPACE = "ota";
static const char* TOKEN_KEY     = "gitlab_token";

static const size_t MAX_TOKEN_LENGTH = 256;

String readSerialLine() {
  // Read one line; the token is never printed by this sketch.
  String line = Serial.readStringUntil('\n');

  // Remove the CR from CR/LF line endings.
  if (line.endsWith("\r")) {
    line.remove(line.length() - 1);
  }

  return line;
}

void printMenu() {
  Serial.println();
  Serial.println("=== ESP32-S3 OTA Token Provisioner ===");
  Serial.println("Commands:");
  Serial.println("  SET_TOKEN    Store/replace the GitLab read token");
  Serial.println("  CHECK_TOKEN  Check whether a token exists");
  Serial.println("  DELETE_TOKEN Delete the stored token");
  Serial.println("  HELP         Show commands");
  Serial.println();
  Serial.print("> ");
}

bool storeToken(const String& token) {
  if (token.length() == 0 || token.length() > MAX_TOKEN_LENGTH) {
    Serial.println("Invalid token length.");
    return false;
  }

  Preferences prefs;

  if (!prefs.begin(NVS_NAMESPACE, false)) {
    Serial.println("ERROR: Failed to open NVS.");
    return false;
  }

  size_t bytesWritten = prefs.putString(TOKEN_KEY, token);

  // Verify that the stored value matches the submitted token.
  String savedToken = prefs.getString(TOKEN_KEY, "");
  bool success =
      bytesWritten > 0 && savedToken == token;

  prefs.end();

  if (success) {
    Serial.println("SUCCESS: Token stored and verified in NVS.");
  } else {
    Serial.println("ERROR: Token storage or verification failed.");
  }

  return success;
}

void checkToken() {
  Preferences prefs;

  if (!prefs.begin(NVS_NAMESPACE, true)) {
    Serial.println("ERROR: Failed to open NVS.");
    return;
  }

  String token = prefs.getString(TOKEN_KEY, "");
  prefs.end();

  if (token.length() > 0) {
    Serial.println("A token exists in NVS.");
    Serial.printf("Stored token length: %u characters\n",
                  (unsigned)token.length());
    Serial.printf("First 4 characters of token: %s\n",
                   token.substring(0, min((unsigned int)4,
                   (unsigned int)token.length())).c_str());
  } else {
    Serial.println("No token found in NVS.");
  }

  token = "";
}

void deleteToken() {
  Preferences prefs;

  if (!prefs.begin(NVS_NAMESPACE, false)) {
    Serial.println("ERROR: Failed to open NVS.");
    return;
  }

  bool success = prefs.remove(TOKEN_KEY);
  prefs.end();

  if (success) {
    Serial.println("Stored token deleted.");
  } else {
    Serial.println("Token was not deleted; it may not exist.");
  }
}

void processCommand(String command) {
  command.trim();

  if (command.equalsIgnoreCase("SET_TOKEN")) {
    Serial.println();
    Serial.println("Paste your GitLab READ-ONLY Deploy Token.");
    Serial.println("Input is not echoed by this sketch.");
    Serial.println("Press Enter when finished.");
    Serial.print("Token: ");

    String token = readSerialLine();

    if (token.length() > MAX_TOKEN_LENGTH) {
      Serial.println();
      Serial.println("ERROR: Token is too long.");
      token = "";
      return;
    }

    Serial.println();
    storeToken(token);

    // Best-effort clearing of the temporary RAM string.
    token = "";
  }
  else if (command.equalsIgnoreCase("CHECK_TOKEN")) {
    checkToken();
  }
  else if (command.equalsIgnoreCase("DELETE_TOKEN")) {
    deleteToken();
  }
  else if (command.equalsIgnoreCase("HELP")) {
    printMenu();
    return;
  }
  else {
    Serial.println("Unknown command. Enter HELP.");
  }
}

void setup() {
  Serial.begin(115200);

  // Allow time for USB Serial Monitor to connect.
  delay(1500);

  // Prevent a partial input line from waiting indefinitely.
  Serial.setTimeout(60000);

  Serial.println();
  Serial.println("ESP32-S3 OTA Token Provisioner ready.");
  Serial.println("Open Serial Monitor at 115200 baud.");
  Serial.println("Use SET_TOKEN to provision credentials.");

  printMenu();
}

void loop() {
  if (Serial.available() > 0) {
    String command = readSerialLine();
    processCommand(command);
    printMenu();
  }
}
