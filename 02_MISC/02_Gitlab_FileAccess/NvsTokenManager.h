#pragma once
#include <Arduino.h>

bool nvs_saveGitlabToken(const String& token);
String nvs_loadGitlabToken();
bool nvs_hasGitlabToken();
void nvs_clearGitlabToken();

// Blocks until a non-empty token is entered on Serial, then stores it.
// Never prints the token itself.
bool nvs_provisionGitlabTokenFromSerial();
