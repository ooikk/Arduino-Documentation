#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <PNGdec.h>  // PNG decoder library


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
#define FILE_EXT "png"

//#define SMALLER_IMAGE
#define SCR_WIDTH 200
#define SCR_HEIGHT 150
#define SCR_OFFSET_X 20
#define SCR_OFFSET_Y 30

TFT_eSPI tft = TFT_eSPI();

// 1️⃣ Define SPI class for SD
#ifdef VSPI_PIN
SPIClass sdSPI(VSPI);
#else
SPIClass sdSPI(HSPI);
#endif

// ─── Global state ──────────────────────────────────────────────
static PNG png;
static TFT_eSPI* pTFT = nullptr;
static File* pngFileHandle = nullptr;

static float pngScale = 1.0f;
static int16_t pngXOffset = 0;
static int16_t pngYOffset = 0;
static int16_t pngScaledW = 0;
static int16_t pngScaledH = 0;
static int16_t pngImgWidth = 0;
static int16_t pngImgHeight = 0;
static int16_t pngScreenW = 0;
static int16_t pngScreenH = 0;
static uint16_t* pngRowBuffer = nullptr;
static int16_t pngLastDrawnY = -1;  // reset before decode

String* getFileListByExtension(const char* extension, int& fileCount);
bool displayPNG(const char* filename, TFT_eSPI& tft, size_t maxSize);


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
    Serial.println("SD Card initialization failed! Try again...");
    delay(1000);
    if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {
      Serial.println("SD Card initialization failed!");
      tft.println("SD Card init failed!");
      while (1) delay(1000);
    }
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

      if (displayPNG(files[i].c_str(), tft, 100 * 1024)) {
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

void waitForSerial() {

  // Clear the serial buffer so it doesn't instantly loop again
  while (Serial.available())
    Serial.read();
  Serial.println("Image displayed! Press <SEND> (or type a character) to advance");
  // Wait here until a character is received over Serial
  while (!Serial.available())
    ;
}



// ─── PNGdec callbacks ──────────────────────────────────────────

static void* pngOpen(const char* filename, int32_t* pSize) {
  if (pngFileHandle) {
    pngFileHandle->close();
    delete pngFileHandle;
    pngFileHandle = nullptr;
  }
  pngFileHandle = new File(SD.open(filename, "r"));
  if (!*pngFileHandle) {
    Serial.printf("SD.open FAILED: %s\n", filename);
    delete pngFileHandle;
    pngFileHandle = nullptr;
    return nullptr;
  }
  uint8_t sig[8];
  if (pngFileHandle->read(sig, 8) != 8) {
    Serial.printf("Can't read signature from %s\n", filename);
    pngFileHandle->close();
    delete pngFileHandle;
    pngFileHandle = nullptr;
    return nullptr;
  }
  const uint8_t pngSig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
  if (memcmp(sig, pngSig, 8) != 0) {
    Serial.printf("Invalid PNG signature in %s\n", filename);
    pngFileHandle->close();
    delete pngFileHandle;
    pngFileHandle = nullptr;
    return nullptr;
  }
  pngFileHandle->seek(0);
  *pSize = pngFileHandle->size();
  return (void*)pngFileHandle;
}

static void pngClose(void* pHandle) {
  if (pngFileHandle) {
    pngFileHandle->close();
    delete pngFileHandle;
    pngFileHandle = nullptr;
  }
}

static int32_t pngRead(PNGFILE* pFile, uint8_t* pBuf, int32_t iLen) {
  if (!pngFileHandle) return -1;
  return pngFileHandle->read(pBuf, iLen);
}

static int32_t pngSeek(PNGFILE* pFile, int32_t iPos) {
  if (!pngFileHandle) return -1;
  return pngFileHandle->seek(iPos) ? iPos : -1;
}

// ─── Draw callback with proper up/down scaling ─────────────────

static int pngDraw(PNGDRAW* pDraw) {
  // Get decoded source row (original width)
  uint16_t lineBuffer[pDraw->iWidth];
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);

  // Vertical mapping range for this source row
  int tgtY_start = (int)(pDraw->y * pngScale + pngYOffset + 0.5f);
  int tgtY_end = (int)((pDraw->y + 1) * pngScale + pngYOffset + 0.5f);
  // For downscaling, ensure at least one row
  if (tgtY_end <= tgtY_start) tgtY_end = tgtY_start + 1;

  // Determine horizontal visible segment (same for all rows in this block)
  int visibleStart = 0, visibleEnd = pngScaledW - 1;
  int screenStartX = pngXOffset;
  int screenEndX = pngXOffset + pngScaledW - 1;
  if (screenEndX < 0 || screenStartX >= pngScreenW) return 1;  // completely off

  if (screenStartX < 0) {
    visibleStart = -pngXOffset;
    screenStartX = 0;
  }
  if (screenEndX >= pngScreenW) {
    visibleEnd = pngScreenW - 1 - pngXOffset;
    screenEndX = pngScreenW - 1;
  }
  int visibleWidth = visibleEnd - visibleStart + 1;
  if (visibleWidth <= 0) return 1;

  // Resize the visible segment horizontally (nearest neighbour)
  for (int x = 0; x < visibleWidth; x++) {
    int srcX = (int)((visibleStart + x - pngXOffset) / pngScale + 0.5f);
    if (srcX < 0) srcX = 0;
    if (srcX >= pngImgWidth) srcX = pngImgWidth - 1;
    pngRowBuffer[x] = lineBuffer[srcX];
  }

  // Draw vertical block: from tgtY_start to tgtY_end - 1
  for (int tgtY = tgtY_start; tgtY < tgtY_end; tgtY++) {
    if (tgtY < 0 || tgtY >= pngScreenH) continue;
    // Avoid duplicate drawing (only needed for downscaling, but harmless)
    if (tgtY == pngLastDrawnY) continue;
    pngLastDrawnY = tgtY;
    pTFT->pushImage(screenStartX, tgtY, visibleWidth, 1, pngRowBuffer);
  }
  return 1;
}

// ─── Cleanup ────────────────────────────────────────────────────

static void pngCleanup() {
  png.close();
  if (pngFileHandle) {
    pngFileHandle->close();
    delete pngFileHandle;
    pngFileHandle = nullptr;
  }
  if (pngRowBuffer) {
    free(pngRowBuffer);
    pngRowBuffer = nullptr;
  }
  delay(10);
}

// ─── Main display function ──────────────────────────────────────
/**
 * Display a PNG image from SD card.
 * @param filename Full path (e.g., "/image.png")
 * @param tft      Reference to your TFT_eSPI object
 * @param maxSize  Not used (kept for compatibility)
 * @return         true on success, false on error
 */

bool displayPNG(const char* filename, TFT_eSPI& tft, size_t maxSize) {
  pngCleanup();

  if (!SD.exists(filename)) {
    Serial.printf("File NOT found: %s\n", filename);
    return false;
  }

  int rc = png.open(filename, pngOpen, pngClose, pngRead, pngSeek, pngDraw);
  if (rc != PNG_SUCCESS) {
    Serial.printf("PNG open failed: %d for %s\n", rc, filename);
    pngCleanup();
    return false;
  }

  pngImgWidth = png.getWidth();
  pngImgHeight = png.getHeight();
  Serial.printf("PNG: %dx%d\n", pngImgWidth, pngImgHeight);

  if (pngImgWidth > pngImgHeight) tft.setRotation(1);
  else tft.setRotation(2);

/**
* User can define target display image dimension by changing pngScreenW and pngScreenH
* Current default to tft width and height
*/

#ifdef SMALLER_IMAGE
  pngScreenW = SCR_WIDTH;
  pngScreenH = SCR_HEIGHT;
#else
  pngScreenW = tft.width();
  pngScreenH = tft.height();
#endif


  float scaleX = (float)pngScreenW / pngImgWidth;
  float scaleY = (float)pngScreenH / pngImgHeight;

#ifdef SMALLER_IMAGE
  pngScale = (scaleX < scaleY) ? scaleX : scaleY;   // if display smaller image, make sure choose the lower scale factor to maximise image
#else
  pngScale = (scaleX < scaleY) ? scaleX : scaleY;   // default
#endif  

  pngScaledW = (int)(pngImgWidth * pngScale + 0.5f);
  pngScaledH = (int)(pngImgHeight * pngScale + 0.5f);

  /**
* User can define target display offset pngXOffset and pngYOffset
* Current default to 0, 0 of the screen
*/
#ifdef SMALLER_IMAGE
  pngXOffset = SCR_OFFSET_X;
  pngYOffset = SCR_OFFSET_Y;
#else
  pngXOffset = (pngScreenW - pngScaledW) / 2;
  pngYOffset = (pngScreenH - pngScaledH) / 2;
#endif  



  pngRowBuffer = (uint16_t*)malloc(pngScreenW * sizeof(uint16_t));
  if (!pngRowBuffer) {
    Serial.println("Row buffer allocation failed");
    png.close();
    return false;
  }

  pTFT = &tft;
  pngLastDrawnY = -1;  // reset for this image

  tft.startWrite();
  uint32_t start = millis();
  rc = png.decode(nullptr, 0);
  Serial.printf("Decode time: %d ms\n", millis() - start);
  tft.endWrite();

  pngCleanup();

  if (rc != PNG_SUCCESS) {
    Serial.printf("PNG decode failed: %d\n", rc);
    return false;
  }
  return true;
}