#include "PsramFileManager.h"
#include <LittleFS.h>
#include <string.h>

bool fileNew(PsramFile& file, const String& name, size_t initialSize) {
  fileDelete(file);
  file.name = name;
  if (initialSize == 0) return true;
  file.data = (uint8_t*)ps_malloc(initialSize);
  if (!file.data) {
    Serial.println("[File] PSRAM allocation failed in fileNew().");
    file.name = "";
    return false;
  }
  memset(file.data, 0, initialSize);
  file.len = initialSize;
  return true;
}

bool fileOverwrite(PsramFile& file, uint8_t* newData, size_t newLen) {
  if (newLen > 0 && newData == nullptr) return false;

  uint8_t* replacement = nullptr;
  if (newLen > 0) {
    replacement = (uint8_t*)ps_malloc(newLen);
    if (!replacement) {
      Serial.println("[File] PSRAM allocation failed in fileOverwrite().");
      return false;
    }
    memcpy(replacement, newData, newLen);
  }

  if (file.data) free(file.data);
  file.data = replacement;
  file.len = newLen;
  return true;
}

bool fileAppend(PsramFile& file, uint8_t* extraData, size_t extraLen) {
  if (extraLen == 0) return true;
  if (extraData == nullptr || file.len > SIZE_MAX - extraLen) return false;

  size_t newLen = file.len + extraLen;
  uint8_t* replacement = (uint8_t*)ps_malloc(newLen);
  if (!replacement) {
    Serial.println("[File] PSRAM allocation failed in fileAppend().");
    return false;
  }
  if (file.len > 0 && file.data) memcpy(replacement, file.data, file.len);
  memcpy(replacement + file.len, extraData, extraLen);

  if (file.data) free(file.data);
  file.data = replacement;
  file.len = newLen;
  return true;
}

void fileDelete(PsramFile& file) {
  if (file.data) free(file.data);
  file.data = nullptr;
  file.len = 0;
  file.name = "";
}

bool fileSaveToLittleFS(PsramFile& file, const String& fsPath) {
  if (fsPath.length() == 0 || (file.len > 0 && file.data == nullptr)) return false;
  File out = LittleFS.open(fsPath, "w");
  if (!out) {
    Serial.println("[LittleFS] Failed to open file for writing.");
    return false;
  }
  size_t written = file.len ? out.write(file.data, file.len) : 0;
  out.flush();
  out.close();
  if (written != file.len) {
    Serial.println("[LittleFS] Incomplete write.");
    return false;
  }
  return true;
}

bool fileLoadFromLittleFS(const String& fsPath, PsramFile& file) {
  File in = LittleFS.open(fsPath, "r");
  if (!in) {
    Serial.println("[LittleFS] Failed to open file for reading.");
    return false;
  }
  size_t size = in.size();
  uint8_t* replacement = nullptr;
  if (size > 0) {
    replacement = (uint8_t*)ps_malloc(size);
    if (!replacement) {
      in.close();
      Serial.println("[LittleFS] PSRAM allocation failed.");
      return false;
    }
    size_t got = in.read(replacement, size);
    if (got != size) {
      free(replacement);
      in.close();
      Serial.println("[LittleFS] Incomplete read.");
      return false;
    }
  }
  in.close();

  if (file.data) free(file.data);
  file.data = replacement;
  file.len = size;
  file.name = fsPath;
  return true;
}
