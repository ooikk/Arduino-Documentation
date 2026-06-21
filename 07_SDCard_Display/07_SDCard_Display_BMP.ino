#include <SPI.h>
#include <SD.h>
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
#define SD_FREQUENCY 16000000  // 24000000 16000000 4000000

#define LIST_FILES
#define FILE_EXT "bmp"

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

#pragma pack(push, 1)
typedef struct {
  uint16_t bfType;
  uint32_t bfSize;
  uint16_t bfReserved1;
  uint16_t bfReserved2;
  uint32_t bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
  uint32_t biSize;
  int32_t biWidth;
  int32_t biHeight;  // positive = bottom-up, negative = top-down
  uint16_t biPlanes;
  uint16_t biBitCount;
  uint32_t biCompression;
  uint32_t biSizeImage;
  int32_t biXPelsPerMeter;
  int32_t biYPelsPerMeter;
  uint32_t biClrUsed;
  uint32_t biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)



String* getFileListByExtension(const char* extension, int& fileCount);
bool displayBMP(const char* filename, TFT_eSPI& tft, size_t maxSize);


void setup() {
  Serial.begin(115200);
  delay(500);

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

      if (displayBMP(files[i].c_str(), tft, 100 * 1024)) {
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

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

// Read a contiguous block of pixels from a BMP row.
// Returns true on success.
// rowIndex: 0 = top row (for both orientations), startCol, numCols.
static bool readBMPRowSegment(File& file, int rowIndex, int startCol, int numCols,
                              uint8_t* buffer, int imgWidth, int imgHeight,
                              int rowSize, int dataOffset, int bytesPerPixel,
                              bool isBottomUp) {
  if (numCols <= 0) return true;

  int actualRow;
  if (isBottomUp) {
    // Bottom-up: row 0 is the bottom row of the image.
    actualRow = imgHeight - 1 - rowIndex;
  } else {
    // Top-down: row 0 is the top row.
    actualRow = rowIndex;
  }

  uint32_t rowOffset = dataOffset + actualRow * rowSize;
  uint32_t seekPos = rowOffset + startCol * bytesPerPixel;
  if (!file.seek(seekPos)) return false;

  size_t bytesToRead = numCols * bytesPerPixel;
  if (file.read(buffer, bytesToRead) != bytesToRead) return false;

  return true;
}

/*
 * @param filename   Full path to the .bmp file (e.g., "/image.bmp")
 * @param tft        Reference to your TFT object
 * @param maxSize Max memory usage 
 * @return           true on success, false on error
 */

bool displayBMP(const char* filename, TFT_eSPI& tft, size_t maxSize) {
  File file = SD.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open BMP file");
    return false;
  }
  tft.setSwapBytes(true);  // <-- FIXES the random dots (byte order)
  BITMAPFILEHEADER fileHeader;
  if (file.read((uint8_t*)&fileHeader, sizeof(fileHeader)) != sizeof(fileHeader)) {
    file.close();
    return false;
  }
  if (fileHeader.bfType != 0x4D42) {
    Serial.println("Not a BMP file");
    file.close();
    return false;
  }

  BITMAPINFOHEADER infoHeader;
  if (file.read((uint8_t*)&infoHeader, sizeof(infoHeader)) != sizeof(infoHeader)) {
    file.close();
    return false;
  }

  if (infoHeader.biBitCount != 24 || infoHeader.biCompression != 0) {
    Serial.println("Only uncompressed 24-bit BMP supported");
    file.close();
    return false;
  }

  int imgWidth = abs(infoHeader.biWidth);
  int imgHeight = abs(infoHeader.biHeight);
  bool isBottomUp = (infoHeader.biHeight > 0);  // true = standard Windows BMP

  int bytesPerPixel = 3;
  int rowSize = ((imgWidth * bytesPerPixel + 3) & ~3);
  uint32_t dataOffset = fileHeader.bfOffBits;

  // Debug info
  Serial.printf("BMP: %dx%d, %s\n", imgWidth, imgHeight, isBottomUp ? "bottom-up" : "top-down");

  // Auto-rotate: align longer side with screen longer side
  if (imgWidth > imgHeight) {
    tft.setRotation(1);  // Landscape
  } else {
    tft.setRotation(2);  // Portrait
  }

  /**
* User can define target display image dimension by changing pngScreenW and pngScreenH
* Current default to tft width and height
*/

#ifdef SMALLER_IMAGE
  int screenW = SCR_WIDTH;
  int screenH = SCR_HEIGHT;
#else
  int screenW = tft.width();
  int screenH = tft.height();
#endif

  // Compute scaling and offsets (fill screen, crop edges)
  float scaleX = (float)screenW / imgWidth;
  float scaleY = (float)screenH / imgHeight;
  
#ifdef SMALLER_IMAGE  
  float scale = (scaleX > scaleY) ? scaleX : scaleY;  // fill
#else
  float scale = (scaleX > scaleY) ? scaleX : scaleY;  // fill
#endif

  int newW = (int)(imgWidth * scale + 0.5f);
  int newH = (int)(imgHeight * scale + 0.5f);

/**
* User can define target display offset pngXOffset and pngYOffset
* Current default to 0, 0 of the screen
*/
#ifdef SMALLER_IMAGE
  int offsetX = SCR_OFFSET_X;
  int offsetY = SCR_OFFSET_Y;
#else
  int offsetX = (screenW - newW) / 2;
  int offsetY = (screenH - newH) / 2;
#endif


  Serial.printf("Scale: %.2f, display size: %dx%d, offset: %d,%d\n", scale, newW, newH, offsetX, offsetY);

  // Decide interpolation: bilinear only when upscaling (smooth)
  bool useBilinear = (scale > 1.0f);

  // Precompute mapping constants (using double for accuracy)
  double invScaleX = 1.0 / scale;
  double invScaleY = 1.0 / scale;

  //double srcX0 = -offsetX * invScaleX;  // source column at screen x=0
  //double srcY0 = -offsetY * invScaleY;  // source row at screen y=0
  double srcX0 = 0;  // source column at screen x=0
  double srcY0 = 0;  // source row at screen y=0

  // Determine the range of source columns we need to read.
  int srcColStart = (int)floor(srcX0);
  int srcColEnd = (int)floor(srcX0 + (screenW - 1) * invScaleX);
  int minCol = (srcColStart < 0) ? 0 : srcColStart;
  int maxCol = (srcColEnd >= imgWidth) ? imgWidth - 1 : srcColEnd;
  // For bilinear we need one extra column for interpolation (if available)
  if (useBilinear && maxCol + 1 < imgWidth) maxCol++;
  int numColsToRead = maxCol - minCol + 1;

  size_t rowBufferSize = numColsToRead * bytesPerPixel;
  size_t totalBufferSize = useBilinear ? 2 * rowBufferSize : rowBufferSize;
  if (totalBufferSize > maxSize) {
    // Fallback to nearest-neighbor if bilinear would exceed memory
    useBilinear = false;
    totalBufferSize = rowBufferSize;
    if (totalBufferSize > maxSize) {
      Serial.println("BMP row segment too large for memory; increase maxSize or process in strips.");
      file.close();
      return false;
    }
  }

  uint8_t* rowBuf0 = (uint8_t*)malloc(rowBufferSize);
  if (!rowBuf0) {
    file.close();
    return false;
  }
  uint8_t* rowBuf1 = nullptr;
  if (useBilinear) {
    rowBuf1 = (uint8_t*)malloc(rowBufferSize);
    if (!rowBuf1) {
      free(rowBuf0);
      file.close();
      return false;
    }
  }

  uint16_t* screenRow = (uint16_t*)malloc(screenW * sizeof(uint16_t));
  if (!screenRow) {
    free(rowBuf0);
    if (rowBuf1) free(rowBuf1);
    file.close();
    return false;
  }

  // Helper: get a packed 24-bit pixel from the buffer (0x00RRGGBB)
  auto getPixel = [&](uint8_t* buf, int col) -> uint32_t {
    int idx = (col - minCol) * bytesPerPixel;
    uint8_t b = buf[idx];
    uint8_t g = buf[idx + 1];
    uint8_t r = buf[idx + 2];
    return (uint32_t)((r << 16) | (g << 8) | b);
  };

  // For each screen row
  for (int y = 0; y < screenH; y++) {
    double srcY_f = srcY0 + y * invScaleY;
    int srcRow = (int)floor(srcY_f);
    double fracY = srcY_f - srcRow;

    // Clamp source row to valid range
    if (srcRow < 0) {
      srcRow = 0;
      fracY = 0;
    }
    if (srcRow >= imgHeight) {
      srcRow = imgHeight - 1;
      fracY = 0;
    }

    int srcRowNext = srcRow + 1;
    if (srcRowNext >= imgHeight) srcRowNext = imgHeight - 1;

    // Read the two row segments
    if (!readBMPRowSegment(file, srcRow, minCol, numColsToRead, rowBuf0,
                           imgWidth, imgHeight, rowSize, dataOffset, bytesPerPixel, isBottomUp)) {
      free(screenRow);
      free(rowBuf0);
      if (rowBuf1) free(rowBuf1);
      file.close();
      return false;
    }
    if (useBilinear && (srcRow != srcRowNext || fracY != 0)) {
      if (!readBMPRowSegment(file, srcRowNext, minCol, numColsToRead, rowBuf1,
                             imgWidth, imgHeight, rowSize, dataOffset, bytesPerPixel, isBottomUp)) {
        free(screenRow);
        free(rowBuf0);
        if (rowBuf1) free(rowBuf1);
        file.close();
        return false;
      }
    } else if (useBilinear) {
      // Same row, copy
      memcpy(rowBuf1, rowBuf0, rowBufferSize);
    }

    // For each screen column
    for (int x = 0; x < screenW; x++) {
      double srcX_f = srcX0 + x * invScaleX;
      int srcCol = (int)floor(srcX_f);
      double fracX = srcX_f - srcCol;

      // Clamp to image boundaries
      if (srcCol < 0) {
        srcCol = 0;
        fracX = 0;
      }
      if (srcCol >= imgWidth) {
        srcCol = imgWidth - 1;
        fracX = 0;
      }
      int srcColNext = srcCol + 1;
      if (srcColNext >= imgWidth) srcColNext = imgWidth - 1;

      // **Safety clamp to the actual buffer range** to avoid reading garbage
      if (srcCol < minCol) srcCol = minCol;
      if (srcCol > maxCol) srcCol = maxCol;
      if (srcColNext < minCol) srcColNext = minCol;
      if (srcColNext > maxCol) srcColNext = maxCol;

      uint32_t c00, c01, c10, c11;
      if (useBilinear) {
        c00 = getPixel(rowBuf0, srcCol);
        c01 = getPixel(rowBuf0, srcColNext);
        c10 = getPixel(rowBuf1, srcCol);
        c11 = getPixel(rowBuf1, srcColNext);
      } else {
        // Nearest neighbor
        uint32_t c = getPixel(rowBuf0, srcCol);
        uint8_t r = (c >> 16) & 0xFF;
        uint8_t g = (c >> 8) & 0xFF;
        uint8_t b = c & 0xFF;
        screenRow[x] = rgb565(r, g, b);
        continue;
      }

      // Bilinear interpolation
      uint8_t r00 = (c00 >> 16) & 0xFF;
      uint8_t g00 = (c00 >> 8) & 0xFF;
      uint8_t b00 = c00 & 0xFF;
      uint8_t r01 = (c01 >> 16) & 0xFF;
      uint8_t g01 = (c01 >> 8) & 0xFF;
      uint8_t b01 = c01 & 0xFF;
      uint8_t r10 = (c10 >> 16) & 0xFF;
      uint8_t g10 = (c10 >> 8) & 0xFF;
      uint8_t b10 = c10 & 0xFF;
      uint8_t r11 = (c11 >> 16) & 0xFF;
      uint8_t g11 = (c11 >> 8) & 0xFF;
      uint8_t b11 = c11 & 0xFF;

      float fx = fracX, fy = fracY;
      float r = r00 * (1 - fx) * (1 - fy) + r01 * fx * (1 - fy) + r10 * (1 - fx) * fy + r11 * fx * fy;
      float g = g00 * (1 - fx) * (1 - fy) + g01 * fx * (1 - fy) + g10 * (1 - fx) * fy + g11 * fx * fy;
      float b = b00 * (1 - fx) * (1 - fy) + b01 * fx * (1 - fy) + b10 * (1 - fx) * fy + b11 * fx * fy;

      screenRow[x] = rgb565((uint8_t)r, (uint8_t)g, (uint8_t)b);
    }

    //tft.pushImage(0, y, screenW, 1, screenRow);
    tft.pushImage(offsetX, offsetY + y, screenW, 1, screenRow);
  }

  free(screenRow);
  free(rowBuf0);
  if (rowBuf1) free(rowBuf1);
  file.close();
  return true;
}