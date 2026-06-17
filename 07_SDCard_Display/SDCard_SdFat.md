# SdFat 

SdFat is the original, fully-featured library written by Bill Greiman. It is lower-level, more powerful, and supports advanced features like exFAT, multiple cards, and faster performance.    

SD.h (the one included with the ESP32 Arduino core) is actually a wrapper around an older version of SdFat. It provides a simplified, Arduino-friendly interface.    

Because they share the same underlying codebase, they are structurally similar. However, they have diverged over time.    

**What Works (and what doesn't)**
|Aspect	| Compatibility|
| -|-|
|Basic operations (open, read, write, close, exists)|	✅ Highly compatible – most code works with just a few changes.|
|Class names (File, SdFile)|	❌ SdFat uses File only in compatibility mode. Without it, you use SdFile.|
|SD global object|	✅ You can create SdFat SD; to mimic the global object.|
|open() modes|	⚠️ Slightly different defaults (e.g., FILE_WRITE may behave differently).|
|begin() parameters|	⚠️ SdFat offers more options (SPI speed, bus, CS pin) – but the basic begin() works similarly.|
|SPI bus sharing|	✅ Both work, but SdFat gives you finer control.|
|Error handling|	⚠️ SdFat often returns explicit error codes, while SD.h uses boolean success/failure.|
|ExFAT support|	✅ SdFat supports exFAT; SD.h usually does not.|
|Multi-card support|	✅ SdFat supports it; SD.h only supports one card.|

**How to Migrate from SD.h to SdFat (Compatibility Mode)**    
The easiest way is to use SdFat's backward compatibility mode, which makes it behave like SD.h.

Change your code like this:
```
// Instead of:
#include <SD.h>

// Use:
#include <SdFat.h>
SdFat SD;  // Creates a global SD object that mimics SD.h
```

After this, most of your existing code (e.g., SD.open(), file.read(), file.close()) will work without further changes. The author of SdFat states that this mode is "very compatible" but warns that some programs might have issues.     

**Known Differences That May Cause Problems**    

1. SD.begin(csPin) – works, but SdFat may be stricter about SPI bus speed. You can adjust it manually.
2. File open modes – FILE_READ and FILE_WRITE are defined in SdFat, but you may need to add O_READ, O_WRITE, O_CREAT if you use the non-compatibility API.
3. file.seek() – works, but SdFat has more precise positioning functions.
4. Memory usage – SdFat may use slightly more RAM depending on the features you enable.

**Should You Switch to SdFat?**   
|Reason|	Recommendation|
|-|-|
|You need exFAT support|	✅ Yes, switch.|
|You want better performance|	✅ Yes, SdFat is often faster.|
|You need to handle multiple cards|	✅ Yes, switch.|
|You have strange compatibility issues with SD.h (like you experienced)|	✅ Yes, trying SdFat may fix them.|
|Your code is simple and works fine|	❌ Not necessary – stick with SD.h.|

**Final Verdict**   
SdFat is not 100% compatible, but it is about 95% compatible when using the compatibility mode. Most projects migrate without issues, and the benefits (speed, exFAT, multiple cards) make it worthwhile. Since you are already facing problems with SD.h, trying SdFat is a smart troubleshooting step – and the migration is simple enough to test in just a few minutes.


## Library    

Library SdFat.h by Bill Greiman    
https://github.com/greiman/SdFat

## Code Samples    

Initialization and setup:     
```
#include <SPI.h>
#include "SdFat.h"
#include <TFT_eSPI.h>

// Define the SD Chip Select pin (must match wiring)
#define VSPI_PIN  // SD card is using VSPI and TFT is using HSPI

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
#define SD_FREQUENCY 16 // 16MHz or 4MHz

SdFs sd; // SdFs supports both FAT32 and exFAT

#ifdef VSPI_PIN
SPIClass sdSPI(VSPI);
#else
SPIClass sdSPI(HSPI);
#endif


void setup() {
  Serial.begin(115200);
  delay(1000);

  // 🔧 FIX: Create an SdSpiConfig object to tell SdFat to use your custom 'sdSPI' bus
  // Parameters: (CS_Pin, SPI_Mode, Max_Speed, SPIClass_Pointer)
  SdSpiConfig sdConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(SD_FREQUENCY), &sdSPI);

  // Initialize using SdFat directly (FAT32/exFAT)
  if (!sd.begin(sdConfig)) {
    Serial.println("❌ SD Card Mount Failed");
    return;
  }
  Serial.println("✅ SD Card Mounted");
  //checkReadOnlyState();
  Serial.println("SD Card initialized successfully.");
  tft.println("SD Card OK");
  // 3. Use SdFat syntax to get card size (sectorCount * 512 bytes)
  uint64_t cardSize = (sd.card()->sectorCount() * 512ULL) / (1024 * 1024);
  //uint64_t cardSize = sd.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  tft.printf("Size: %lluMB", cardSize);
  //listRootdir();
  //listDirRecursive("/", 3);
  //listSpecificFolder("/Arduino");
  Serial.println("File List Successfully.");
  tft.println("File List Successfully.");
}

void loop() {
  // ----
}



```

Below are some functions to read file name from directory.     

```
/* Put these declaratio above setup() */
void checkReadOnlyState(void);
// Call this in setup(): listDirRecursive("/", 3);
void listDirRecursive(String , uint8_t ); 
// Call this in setup(): listSpecificFolder("/YOUR_FOLDER_NAME");
void listSpecificFolder(const char* );

//*****************************
void listRootdir() {
  FsFile root; // Use FsFile, NOT standard File 
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
    file.close(); // Close the file before opening the next one
  }
  root.close();
}

//*****************************
void checkReadOnlyState() {
  SdCard *card = sd.card();
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

//*****************************
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

//*****************************
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

```
