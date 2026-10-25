#include "NvsTokenManager.h"
#include <Preferences.h>

static const char* NVS_NAMESPACE = "gitlab-auth";
static const char* NVS_KEY_TOKEN = "pat";

bool nvs_saveGitlabToken(const String& token) {
  if (token.length() == 0 || token.length() > 512) {
    Serial.println("[NVS] Refusing empty or unusually long token.");
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(NVS_NAMESPACE, false)) {
    Serial.println("[NVS] Failed to open namespace for writing.");
    return false;
  }

  size_t written = prefs.putString(NVS_KEY_TOKEN, token);
  prefs.end();

  if (written != token.length()) {
    Serial.println("[NVS] Token write failed or was incomplete.");
    return false;
  }
  Serial.println("[NVS] GitLab token saved (value not displayed).");
  return true;
}

String nvs_loadGitlabToken() {
  Preferences prefs;
  if (!prefs.begin(NVS_NAMESPACE, true)) {
    Serial.println("[NVS] Failed to open namespace for reading.");
    return String();
  }
  String token = prefs.getString(NVS_KEY_TOKEN, "");
  prefs.end();
  return token;
}

bool nvs_hasGitlabToken() {
  Preferences prefs;
  if (!prefs.begin(NVS_NAMESPACE, true)) {
    return false;
  }
  bool found = prefs.isKey(NVS_KEY_TOKEN) &&
               prefs.getString(NVS_KEY_TOKEN, "").length() > 0;
  prefs.end();
  return found;
}

void nvs_clearGitlabToken() {
  Preferences prefs;
  if (!prefs.begin(NVS_NAMESPACE, false)) {
    Serial.println("[NVS] Failed to open namespace for deletion.");
    return;
  }
  bool ok = prefs.remove(NVS_KEY_TOKEN);
  prefs.end();
  Serial.println(ok ? "[NVS] GitLab token deleted." :
                      "[NVS] Token key was not present or deletion failed.");
}

bool nvs_provisionGitlabTokenFromSerial() {
  Serial.println();
  Serial.println("[NVS] No GitLab token is stored.");
  Serial.println("Enter GitLab PAT:");
  Serial.println("Input is not echoed by this sketch; terminal local echo may still be enabled.");

  String token;
  token.reserve(128);
  const unsigned long started = millis();
  const unsigned long timeoutMs = 5UL * 60UL * 1000UL;

  while (millis() - started < timeoutMs) {
    while (Serial.available()) {
      char c = (char)Serial.read();
      if (c == '\r' || c == '\n') {
        if (token.length() > 0) {
          bool saved = nvs_saveGitlabToken(token);
          token = "";
          return saved;
        }
      } else if (c >= 0x20 && c <= 0x7E && token.length() < 512) {
        token += c;
      }
    }
    delay(10);
  }

  token = "";
  Serial.println("[NVS] Provisioning timed out. Reset or reboot to try again.");
  return false;
}
