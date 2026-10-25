#pragma once
#include <Arduino.h>

struct PsramFile {
  String name;
  uint8_t* data = nullptr; // PSRAM-owned byte buffer
  size_t len = 0;
};

bool fileNew(PsramFile& file, const String& name, size_t initialSize);
bool fileOverwrite(PsramFile& file, uint8_t* newData, size_t newLen);
bool fileAppend(PsramFile& file, uint8_t* extraData, size_t extraLen);
void fileDelete(PsramFile& file);
bool fileSaveToLittleFS(PsramFile& file, const String& fsPath);
bool fileLoadFromLittleFS(const String& fsPath, PsramFile& file);
