// Define the SD Chip Select pin (must match wiring)
#define VSPI_PIN  // SD card is using VSPI and TFT is using HSPI

#include <SPI.h>
//#include <SD.h>
#include "SdFat.h"
#include <TFT_eSPI.h>

#define SDFAT  // Use SdFat library
SdFs sd;       // SdFs supports both FAT32 and exFAT

#ifdef VSPI_PIN
#define SD_SCLK_PIN 4
#define SD_MISO_PIN 5
#define SD_MOSI_PIN 6
#else
#define SD_SCLK_PIN 12
#define SD_MISO_PIN 13
#define SD_MOSI_PIN 11
#endif

#define SD_CS_PIN 7
#define SD_FREQUENCY 16000000  // 16MHz or 4MHz

TFT_eSPI tft = TFT_eSPI();

#ifdef VSPI_PIN
SPIClass sdSPI(VSPI);
#else
SPIClass sdSPI(HSPI);
#endif

void checkReadOnlyState(void);
// Call this in setup(): listDirRecursive("/", 3);
void listDirRecursive(String, uint8_t);
// Call this in setup(): listSpecificFolder("/YOUR_FOLDER_NAME");
void listSpecificFolder(const char*);

void setup() {
  Serial.begin(115200);
  delay(1000);

  /*
  // CRITICAL: Initialize SD card FIRST
  // Ensure TFT CS is HIGH (disabled) before SD init
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  delay(100);
*/

  // Initialize TFT Display
  Serial.println("Initializing TFT...");
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  Serial.println("TFT ready...");
  tft.println("TFT ready...");

  // Explicitly bind SPI to your SPI pins BEFORE SD.begin()
  // SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN);
  sdSPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN);  // SCK, MISO, MOSI

#ifdef SDFAT
  // 🔧 FIX: Create an SdSpiConfig object to tell SdFat to use your custom 'sdSPI' bus
  // Parameters: (CS_Pin, SPI_Mode, Max_Speed, SPIClass_Pointer)
  SdSpiConfig sdConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(1), &sdSPI);

  // Initialize using SdFat directly (FAT32/exFAT)
  if (!sd.begin(sdConfig)) {
    Serial.println("❌ SD Card Mount Failed");
    return;
  }

#else
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS_PIN, SPI, SD_FREQUENCY)) {
    Serial.println("❌ SD Card Mount Failed");
    return;
  }
#endif
  Serial.println("✅ SD Card Mounted");

  checkReadOnlyState();


  Serial.println("SD Card initialized successfully.");
  tft.println("SD Card OK");

  // 3. Use SdFat syntax to get card size (sectorCount * 512 bytes)
  uint64_t cardSize = (sd.card()->sectorCount() * 512ULL) / (1024 * 1024);

  //uint64_t cardSize = sd.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  tft.printf("Size: %lluMB", cardSize);

  //listRootdir();
  //listDirRecursive("/", 3);
  listSpecificFolder("/Arduino");
  Serial.println("File List Successfully.");
  tft.println("File List Successfully.");
}

void loop() {
  // ----
}

void listRootdir() {

#ifdef SDFAT
  FsFile root;  // Use FsFile, NOT standard File
  if (!root.open("/")) {
    Serial.println("Failed to open root directory");
    return;
  }

  FsFile file;
  // Open next file in the root directory
  while (file.openNext(&root, O_RDONLY)) {
    if (file.isFile()) {
      char name[50];
      file.getName(name, sizeof(name));
      Serial.print(" - File: ");
      Serial.println(name);
    }
    file.close();  // Close the file before opening the next one
  }
  root.close();

#else

  File root = sd.open("/");
  if (!root) {

    Serial.println("Failed to open root directory");

    tft.println("Failed to open root directory");

    return;
  }
  File file = root.openNextFile();
  while (file) {
    String fileName = String(file.name());

    String fullPath = "/" + fileName;
    Serial.println("Displaying: " + fullPath);

    tft.printf("Displaying: /%s\n", fileName.c_str());
    //tft.println("Displaying: " + fullPath);

    file.close();
    file = root.openNextFile();
  }
  root.close();
#endif
}

void checkReadOnlyState() {
  SdCard* card = sd.card();
  if (!card) {
    Serial.println("❌ Cannot access low-level card object");
    return;
  }

  csd_t csd;
  if (!card->readCSD(&csd)) {
    Serial.println("❌ Failed to read CSD register");
    return;
  }

  // CORRECTED: Use tempWriteProtect(), NOT tmpWriteProtect()
  if (csd.permWriteProtect() || csd.tempWriteProtect()) {
    Serial.println("⚠️ Current Read-only State : Yes");
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("READ ONLY: YES");

    if (csd.permWriteProtect()) {
      Serial.println("   -> Cause: Permanent Write Protection (Card is likely failing/dead)");
    }
    if (csd.tempWriteProtect()) {
      Serial.println("   -> Cause: Temporary Write Protection");
    }
  } else {
    Serial.println("✅ Current Read-only State : No");
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("READ ONLY: NO");
  }
}

// Call this in setup(): listDirRecursive("/", 3);
void listDirRecursive(String dirname, uint8_t levels) {
  Serial.printf("🔍 Scanning directory: %s\n", dirname.c_str());

  FsFile root;
  if (!root.open(dirname.c_str())) {
    Serial.println("❌ Failed to open directory");
    return;
  }

  FsFile file;
  while (file.openNext(&root, O_RDONLY)) {
    char name[50];
    file.getName(name, sizeof(name));

    if (file.isDirectory()) {
      Serial.print("  📁 [DIR]  : ");
      Serial.println(name);

      // If we haven't reached the depth limit, search inside this folder
      if (levels > 0) {
        String nextpath = dirname + "/" + name;
        listDirRecursive(nextpath, levels - 1);
      }
    } else {
      Serial.print("  📄 [FILE] : ");
      Serial.print(name);
      Serial.print("  (Size: ");
      Serial.print(file.size());
      Serial.println(" bytes)");
    }
    file.close();
  }
  root.close();
}

// Call this in setup(): listSpecificFolder("/YOUR_FOLDER_NAME");
void listSpecificFolder(const char* folderPath) {
  Serial.printf("🔍 Opening specific folder: %s\n", folderPath);

  FsFile dir;
  if (!dir.open(folderPath)) {
    Serial.print("❌ Failed to open folder: ");
    Serial.println(folderPath);
    Serial.println("   -> Check spelling! Folders are case-sensitive.");
    return;
  }

  FsFile file;
  int fileCount = 0;

  while (file.openNext(&dir, O_RDONLY)) {
    char name[50];
    file.getName(name, sizeof(name));

    // Only print if it's a file (ignores sub-folders inside this folder)
    if (!file.isDirectory()) {
      Serial.print("  📄 Found File: ");
      Serial.println(name);
      fileCount++;
    }
    file.close();
  }

  dir.close();

  if (fileCount == 0) {
    Serial.println("⚠️ No files found in this folder.");
  } else {
    Serial.printf("✅ Found %d files.\n", fileCount);
  }
}