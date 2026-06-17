#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>


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
#define SD_FREQUENCY 16000000  // 24000000 16000000 4000000

#define LIST_FILES
#define FILE_EXT "jpg"


TFT_eSPI tft = TFT_eSPI();

// 1️⃣ Define SPI class for SD
#ifdef VSPI_PIN
SPIClass sdSPI(VSPI);
#else
SPIClass sdSPI(HSPI);
#endif

String* getFileListByExtension(const char* extension, int& fileCount);
bool displayJPG(const char* filename, TFT_eSPI& tft, size_t maxSize);
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);


void setup() {
  Serial.begin(115200);
  delay(1000);

  // 1️⃣ Set all CS pins high to deselect devices before initialization
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  delay(100);

  // 2️⃣ Initialize the TFT first. This also sets up the SPI bus
  //  (HSPI or VSPI, because of USE_HSPI_PORT or USE_FSPI_PORT in User_Setup.h)
  Serial.println("Initializing TFT...");
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);

  // 3️⃣ Get the SPI bus instance that the TFT is using
  // SPIClass& sdSPI = tft.getSPIinstance();

  // 3️⃣ Explicitly bind SPI to your SPI pins BEFORE SD.begin()
  // SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN);
  sdSPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN);  // SCK, MISO, MOSI


  // 4️⃣ Now initialize the SD card on the SPI bus
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {
    Serial.println("SD Card initialization failed!");
    tft.println("SD Card init failed!");
    while (1) delay(1000);
  }

  Serial.printf("Free heap before open: %u\n", ESP.getFreeHeap());


  // ****************** Get card information
  // Get the card object and check read-only
  uint8_t cardType = SD.cardType();
  uint64_t cardSize = SD.cardSize();
  //bool isReadOnly = SD.card()->readOnly();  // may not be available in all versions


  if (cardType == CARD_MMC) {
    Serial.println("Card type: MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("Card type: SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("Card type: SDHC");
  } else {
    Serial.println("Unknown card type");
  }

  Serial.printf("Card type: %d, Size: %llu MB\n", cardType, cardSize / (1024 * 1024));
  //Serial.printf("Read-only status: %s\n", isReadOnly ? "Yes" : "No");


// ****************** Get card information  


#ifdef LIST_FILES
  int count = 0;
  String* files = getFileListByExtension("*", count);  // Get all files

  if (files != nullptr) {
    Serial.println("Files found:");
    tft.println("File found:");
    for (int i = 0; i < count; i++) {
      Serial.println(files[i]);
      tft.println(files[i]);
    }
    delete[] files;  // Free allocated memory
  } else {
    Serial.println("SD card error or no files.");
  }
  Serial.println("Continue next...");
  waitForSerial();

#endif  // #ifdef LIST_FILES

  int fileCount = 0;
#ifdef LIST_FILES
  files = getFileListByExtension(FILE_EXT, fileCount);
#else
  String* files = getFileListByExtension(FILE_EXT, fileCount);
#endif

uint32_t start_Time;

if (files && fileCount > 0) {
  for (int i = 0; i < fileCount; i++) {
    Serial.print("Displaying: ");
    Serial.println(files[i]);
    tft.fillScreen(TFT_BLACK);
    // Display the file – centered automatically

    start_Time = millis();

    if (displayJPG(files[i].c_str(), tft, 100 * 1024)) {
      // Serial.println("  OK");
    } else {
      Serial.println("  Failed to display");
    }

    // Wait a moment before next image
    //      delay(3000);
    Serial.printf("Process time :  %lu ms \n", millis() - start_Time);
    Serial.println("Continue next image...");
    waitForSerial();
  }
  delete[] files;
} else {
  Serial.println("No image files found!");
}

Serial.println("Exit: No more image files...");
tft.fillScreen(TFT_BLACK);
}

void loop() {
  // ----
}

void listFilesAbsolute(File dir, const String& extension, const String& prefix, String*& fileList, int& count) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;

    String fullPath = prefix + entry.name();

    if (entry.isDirectory()) {
      // Recursively scan subdirectory with updated prefix
      listFilesAbsolute(entry, extension, fullPath + "/", fileList, count);
    } else {
      // Extract extension
      String fileExt = "";
      int dotIndex = fullPath.lastIndexOf('.');
      if (dotIndex != -1) {
        fileExt = fullPath.substring(dotIndex + 1);
        fileExt.toLowerCase();
      }

      bool match = (extension == "*") || (fileExt == extension);
      if (match) {
        // Add to array
        String* newList = new String[count + 1];
        for (int i = 0; i < count; i++) newList[i] = fileList[i];
        newList[count] = fullPath;
        delete[] fileList;
        fileList = newList;
        count++;
      }
    }
    entry.close();
  }
}

/**
 * Recursively collects file paths from the SD card.
 * 
 * @param dir          Directory to scan (start with "/" for root).
 * @param extension    File extension to match (e.g., "txt", "jpg", or "*" for all).
 * @param fileList     Reference to a pointer that will point to the allocated array of strings.
 * @param count        Reference to an integer that will hold the number of files found.
 */
void listFiles(File dir, const String& extension, String*& fileList, int& count) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;  // No more files/directories

    if (entry.isDirectory()) {
      // Recursively scan subdirectories
      listFiles(entry, extension, fileList, count);
    } else {
      // Get the file name and full path
      String fullPath = String(entry.name());
      String fileExt = "";

      // Extract extension (characters after last dot)
      int dotIndex = fullPath.lastIndexOf('.');
      if (dotIndex != -1) {
        fileExt = fullPath.substring(dotIndex + 1);
        fileExt.toLowerCase();
      }

      // Check extension match
      bool match = false;
      if (extension == "*") {
        match = true;
      } else {
        String extLower = extension;
        extLower.toLowerCase();
        if (fileExt == extLower) match = true;
      }

      if (match) {
        // Expand array size by one and add the new path
        String* newList = new String[count + 1];
        for (int i = 0; i < count; i++) {
          newList[i] = fileList[i];
        }
        newList[count] = fullPath;
        delete[] fileList;   // Free old array
        fileList = newList;  // Point to new array
        count++;
      }
    }
    entry.close();
  }
}

int countFiles(File dir, const String& extension, const String& prefix) {
  int cnt = 0;
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    if (entry.isDirectory()) {
      cnt += countFiles(entry, extension, prefix + entry.name() + "/");
    } else {
      String fullPath = prefix + entry.name();
      String ext = "";
      int dot = fullPath.lastIndexOf('.');
      if (dot != -1) ext = fullPath.substring(dot + 1);
      ext.toLowerCase();
      if (extension == "*" || ext == extension) cnt++;
    }
    entry.close();
  }
  return cnt;
}

void collectFiles(File dir, const String& extension, const String& prefix, String*& out, int& idx) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    if (entry.isDirectory()) {
      collectFiles(entry, extension, prefix + entry.name() + "/", out, idx);
    } else {
      String fullPath = prefix + entry.name();
      String ext = "";
      int dot = fullPath.lastIndexOf('.');
      if (dot != -1) ext = fullPath.substring(dot + 1);
      ext.toLowerCase();
      if (extension == "*" || ext == extension) {
        out[idx++] = fullPath;
      }
    }
    entry.close();
  }
}

/**
 * User‑friendly wrapper: returns an array of file paths (with names) matching a given extension.
 * 
 * @param extension   File extension to filter (e.g., "csv", "bin"). Use "*" for all files.
 * @param fileCount   Output parameter: number of entries in the returned array.
 * @return            Pointer to a dynamically allocated array of String objects.
 *                    The caller is responsible for deleting[] the array when done.
 */
String* getFileListByExtension(const char* extension, int& fileCount) {
  File root = SD.open("/");
  if (!root) {
    fileCount = 0;
    return nullptr;
  }

  fileCount = countFiles(root, String(extension), "/");
  if (fileCount == 0) {
    root.close();
    return nullptr;
  }

  String* result = new String[fileCount];
  int idx = 0;
  root.rewindDirectory();
  collectFiles(root, String(extension), "/", result, idx);
  root.close();
  return result;
}



// Forward declaration of the rendering callback
//bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);

/**
 * @brief Display a JPEG from SD card with auto‑rotation and smart scaling.
 *
 * The image orientation is matched to the screen (longer side aligns with longer side).
 * If the image is larger than the screen, it is down‑scaled to *cover* the display
 * (edges are cropped). If it is smaller, it is shown centered (letterboxed).
 *
 * @param filename   Full path to the .jpg file (e.g., "/Shangrila.jpg")
 * @param tft        Reference to your TFT_eSPI object
 * @param maxSize    Maximum RAM usage (TJpg_Decoder uses ~3.5KB fixed, this param is kept for compatibility)
 * @return           true on success, false on error
 */
bool displayJPG(const char* filename, TFT_eSPI& tft, size_t maxSize) {
  // ------------------------------------------------------------------
  // 1. Get JPEG dimensions without decoding
  // ------------------------------------------------------------------
  uint16_t imgW = 0, imgH = 0;
  JRESULT result = TJpgDec.getSdJpgSize(&imgW, &imgH, filename);
  if (result != JDR_OK || imgW == 0 || imgH == 0) {
    Serial.print("Decoding error: ");
    Serial.println(result);  // 0 = OK, 1 = parameter error, 2 = out of memory, etc.
    return false;
  }

  // ------------------------------------------------------------------
  // 2. Save current rotation and auto‑rotate screen if needed
  // ------------------------------------------------------------------
  /*
  uint8_t currentRot = tft.getRotation();
  int sw = tft.width();
  int sh = tft.height();
  bool scrLandscape = (sw >= sh);
  bool imgLandscape = (imgW >= imgH);

  if (imgLandscape != scrLandscape) {
    tft.setRotation((currentRot + 1) % 4);  // 90° clockwise
    sw = tft.width();                       // update after rotation
    sh = tft.height();
  }
*/
  // After reading image width, image height

  uint8_t currentRot = tft.getRotation();

  if (imgW > imgH) {
    // Image is landscape
    tft.setRotation(1);
  } else tft.setRotation(2);

  int sw = tft.width();  // update after rotation
  int sh = tft.height();


  // ------------------------------------------------------------------
  // 3. Compute the best integer scale (1, 2, 4, or 8)
  //    "Cover" mode if possible, otherwise "fit" mode.
  // ------------------------------------------------------------------
  float ratioW = (float)imgW / sw;
  float ratioH = (float)imgH / sh;
  float minRatio = (ratioW < ratioH) ? ratioW : ratioH;

  uint8_t scale = 1;
  if (minRatio >= 1.0f) {
    // Image is large enough to cover the screen – choose largest power‑of‑2 ≤ minRatio
    while (scale * 2 <= minRatio && scale < 8) {
      scale *= 2;
    }
  } else {
    // Image is smaller in at least one dimension – fit entirely (letterbox)
    float maxRatio = (ratioW > ratioH) ? ratioW : ratioH;
    while (scale < maxRatio && scale < 8) {
      scale *= 2;
    }
  }

  // Compute scaled image size and centre position
  int scaledW = imgW / scale;
  int scaledH = imgH / scale;
  int x = (sw - scaledW) / 2;
  int y = (sh - scaledH) / 2;

  // ------------------------------------------------------------------
  // 4. Configure TJpg_Decoder
  // ------------------------------------------------------------------
  TJpgDec.setJpgScale(scale);
  TJpgDec.setCallback(tft_output);
  TJpgDec.setSwapBytes(true);
  // ------------------------------------------------------------------
  // 5. Draw the JPEG from SD card
  // ------------------------------------------------------------------
  // Note: drawSdJpg() draws at the specified (x,y) position.
  // The callback (tft_output) handles clipping at screen edges.
  result = TJpgDec.drawSdJpg(x, y, filename);

  // ------------------------------------------------------------------
  // 6. Restore original screen rotation
  // ------------------------------------------------------------------
  tft.setRotation(currentRot);

  return (result == JDR_OK);
}


// ------------------------------------------------------------------
// Rendering callback – called by TJpg_Decoder for each MCU block
// ------------------------------------------------------------------
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}



void waitForSerial() {

  // Clear the serial buffer so it doesn't instantly loop again
  while (Serial.available())
    Serial.read();
  Serial.println("Image displayed! Press <SEND> (or type a character) to advance");
  // Wait here until a character is received over Serial
  while (!Serial.available())
    ;
}