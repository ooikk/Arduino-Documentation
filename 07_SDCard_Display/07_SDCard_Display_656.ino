// Define the SD Chip Select pin (must match wiring)
#define VSPI_PIN   // SD card is using VSPI and TFT is using HSPI

#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

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
#define SD_FREQUENCY 16000000

#define LIST_FILES_ONLY
#define STREAM_RGB656

TFT_eSPI tft = TFT_eSPI();

// 1️⃣ Define SPI class for SD
#ifdef VSPI_PIN
SPIClass sdSPI(VSPI);
#else
SPIClass sdSPI(HSPI);
#endif

uint16_t height, width;
uint16_t* pixels = nullptr;


bool displayBinary656(const char* filename, TFT_eSPI& tft, int destX = -1, int destY = -1) {
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    Serial.println("Cannot open file");
    return false;
  }


  if (file.read((uint8_t*)&height, 2) != 2) {
    file.close();
    return false;
  }
  if (file.read((uint8_t*)&width, 2) != 2) {
    file.close();
    return false;
  }

  Serial.printf("Image: %d x %d\n", height, width);

  // After reading image width, image height
  if (width > height) {
    // Image is landscape
    tft.setRotation(1);
  } else  tft.setRotation(2);

  // Center on screen
  if (destX == -1) destX = (tft.width() - width) / 2;
  if (destY == -1) destY = (tft.height() - height) / 2;
  if (destX < 0) destX = 0;
  if (destY < 0) destY = 0;

  uint16_t* rowBuffer = new (std::nothrow) uint16_t[width];
  if (!rowBuffer) {
    file.close();
    return false;
  }

  tft.setSwapBytes(true);  // because file is little‑endian, TFT expects big‑endian

  for (uint16_t y = 0; y < height; y++) {
    size_t readBytes = file.read((uint8_t*)rowBuffer, width * 2);
    if (readBytes != width * 2) {
      Serial.printf("Error reading row %d\n", y);
      delete[] rowBuffer;
      file.close();
      return false;
    }
    tft.pushImage(destX, destY + y, width, 1, rowBuffer);
  }

  delete[] rowBuffer;
  file.close();
  return true;
}




/**
 * Reads a .656 text file directly from SD and displays it on the TFT,
 * row by row, without storing the whole image in RAM.
 * 
 * @param filename   Full path to the .656 file (e.g., "/Shangrila.656")
 * @param tft        Reference to your TFT object
 * @param destX      Top‑left X coordinate on TFT (default = center of screen)
 * @param destY      Top‑left Y coordinate on TFT (default = center)
 * @return           true on success, false on error
 */

bool display656FileDirect(const char* filename, TFT_eSPI& tft, int destX = -1, int destY = -1) {
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    Serial.println("Cannot open file");
    return false;
  }

  // ----- Helper: skip whitespace (space, tab, newline, carriage return) -----
  auto skipWhitespace = [&]() {
    while (file.available()) {
      char c = file.peek();
      if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        file.read();
      else
        break;
    }
  };

  // ----- Helper: read a decimal integer (height or width) -----
  auto readDecimal = [&](uint16_t& value) -> bool {
    value = 0;
    skipWhitespace();
    if (!file.available()) return false;
    char c = file.peek();
    if (c < '0' || c > '9') return false;
    while (file.available()) {
      c = file.peek();
      if (c >= '0' && c <= '9') {
        file.read();
        value = value * 10 + (c - '0');
      } else break;
    }
    return true;
  };

  // ----- Helper: read a hexadecimal number (0xXXXX) -----
  auto readHex = [&](uint16_t& value) -> bool {
    value = 0;
    skipWhitespace();
    if (!file.available()) return false;
    if (file.read() != '0') return false;
    if (!file.available()) return false;
    char c = file.read();
    if (c != 'x' && c != 'X') return false;
    while (file.available()) {
      c = file.peek();
      if (c >= '0' && c <= '9') {
        file.read();
        value = (value << 4) | (c - '0');
      } else if (c >= 'a' && c <= 'f') {
        file.read();
        value = (value << 4) | (c - 'a' + 10);
      } else if (c >= 'A' && c <= 'F') {
        file.read();
        value = (value << 4) | (c - 'A' + 10);
      } else break;
    }
    return true;
  };

  // ----- Parse header: height, width -----
  uint16_t height, width, screenW, screenH;
  if (!readDecimal(height)) {
    file.close();
    return false;
  }
  skipWhitespace();
  if (file.read() != ',') {
    file.close();
    return false;
  }
  if (!readDecimal(width)) {
    file.close();
    return false;
  }

  // ----- Expect comma and opening brace -----
  skipWhitespace();
  if (file.read() != ',') {
    file.close();
    return false;
  }
  skipWhitespace();
  if (file.read() != '{') {
    file.close();
    return false;
  }

  // After reading image width, image height
  if (width > height) {
    // Image is landscape
    tft.setRotation(1);
  } else  tft.setRotation(2);


  // ----- Compute display position (centered if not specified) -----
  if (destX == -1) destX = (tft.width() - width) / 2;
  if (destY == -1) destY = (tft.height() - height) / 2;

  // ----- Allocate a row buffer (width pixels * 2 bytes each) -----
  // Use std::nothrow to avoid crash if allocation fails (e.g., width too large)
  uint16_t* rowBuffer = new (std::nothrow) uint16_t[width];
  if (rowBuffer == nullptr) {
    Serial.println("Not enough memory for a single row buffer");
    file.close();
    return false;
  }

  // ----- Read and display row by row -----
  bool success = true;
  tft.setSwapBytes(true);  // // Swap the byte order for pushImage() and pushPixels() - corrects endianness
  for (uint16_t y = 0; y < height; y++) {
    // Read one entire row (width hex values)
    for (uint16_t x = 0; x < width; x++) {
      skipWhitespace();
      // If we encounter '}' before finishing the row, file is malformed
      if (file.peek() == '}') {
        success = false;
        break;
      }
      if (!readHex(rowBuffer[x])) {
        success = false;
        break;
      }
      skipWhitespace();
      // After a hex value, there must be a comma or the closing brace
      if (x < width - 1) {
        if (file.peek() != ',') {
          success = false;
          break;
        }
        file.read();  // consume the comma
      }
    }
    if (!success) break;

    // Push the row to the TFT
    // Using pushImage with height = 1 draws a single row efficiently
    tft.pushImage(destX, destY + y, width, 1, rowBuffer);

    // After each row, we may have a newline or comma – skip whitespace
    skipWhitespace();
    // If this was the last row, we expect a closing brace
    if (y == height - 1) {
      if (file.peek() != '}') success = false;
      else file.read();  // consume '}'
    } else {
      // Otherwise, there should be a comma between rows
      if (file.peek() == ',') file.read();
      // (the file could also have a newline – skipWhitespace handles it)
    }
    if (!success) break;
  }

  // ----- Clean up -----
  delete[] rowBuffer;
  file.close();

  // Optional: skip trailing semicolon if present
  if (success && file.available()) {
    skipWhitespace();
    if (file.peek() == ';') file.read();
  }

  return success;
}

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

#ifdef LIST_FILES_ONLY
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
#else

#ifdef STREAM_RGB656

  int fileCount = 0;
  String* files = getFileListByExtension("b656", fileCount);

  if (files && fileCount > 0) {
    for (int i = 0; i < fileCount; i++) {
      Serial.print("Displaying: ");
      Serial.println(files[i]);
      tft.fillScreen(TFT_BLACK);
      // Display the file – centered automatically

      if (displayBinary656(files[i].c_str(), tft)) {
        //if (display656FileDirect(files[i].c_str(), tft)) {
        Serial.println("  OK");
      } else {
        Serial.println("  Failed to display");
      }

      // Wait a moment before next image
      //      delay(3000);
      Serial.println("Continue next image...");
      waitForSerial();
    }
    delete[] files;
  } else {
    Serial.println("No .656 files found");
  }

#else
  /**
  Copy RGB656 file contains from SD card and than display to TFT
  Required huge memory which limit it's application to small image < 200x200
**/
  int fileCount = 0;
  String* files = getFileListByExtension("656", fileCount);  // get all .656 files

  if (files && fileCount > 0) {
    for (int i = 0; i < fileCount; i++) {
      Serial.print("Reading: ");
      Serial.println(files[i]);

      uint16_t height, width;
      uint16_t* pixels = nullptr;

      if (read656FileText(files[i].c_str(), height, width, pixels)) {
        showimage16(width, height, pixels);
        delete[] pixels;  // free memory after use
      } else {
        Serial.println("  Failed to parse file!");
      }
      Serial.println("Continue next image...");
      waitForSerial();
    }
    delete[] files;  // free the file name array
  } else {
    Serial.println("No .656 files found or SD error.");
  }
#endif
#endif
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


/**
 * Reads a .656 file from the SD card and extracts the image dimensions
 * and pixel data.
 *
 * @param filename  Full path to the .656 file (e.g., "/images/img.656").
 * @param height    Reference to store the image height (first integer).
 * @param width     Reference to store the image width (second integer).
 * @param data      Reference to a pointer that will point to a newly allocated
 *                  array of uint16_t containing the pixel values.
 *                  The caller is responsible for deleting[] this array.
 * @return          true on success, false on error (file missing, parse error,
 *                  size mismatch, etc.).
 */
bool read656File(const char* filename, uint16_t& height, uint16_t& width, uint16_t*& data) {
  // Build full path if needed
  String fullPath;
  if (filename[0] != '/') fullPath = "/" + String(filename);
  else fullPath = filename;

  Serial.println("Opening: " + fullPath);
  File file = SD.open(fullPath.c_str(), FILE_READ);
  if (!file) {
    Serial.println("  Open failed");
    return false;
  }
  Serial.println("  File opened, size: " + String(file.size()));

  // Read height
  uint8_t low = file.read();
  uint8_t high = file.read();
  height = (high << 8) | low;
  Serial.println("  Height = " + String(height));

  // Read width
  low = file.read();
  high = file.read();
  width = (high << 8) | low;
  Serial.println("  Width  = " + String(width));

  uint32_t pixelCount = (uint32_t)height * (uint32_t)width;
  Serial.println("  Pixel count = " + String(pixelCount));

  // Sanity check: reject obviously wrong dimensions
  if (height == 0 || width == 0 || pixelCount > 320 * 480) {  // adjust max for your board
    Serial.println("  Invalid dimensions");
    file.close();
    return false;
  }

  // Check file size matches expected
  uint32_t expected = 4 + pixelCount * 2;
  if (file.size() < expected) {
    Serial.println("  File too small");
    file.close();
    return false;
  }

  // Allocate with nothrow to avoid abort
  Serial.println("  Allocating memory...");
  data = new (std::nothrow) uint16_t[pixelCount];
  if (data == nullptr) {
    Serial.println("  Allocation failed - out of memory!");
    file.close();
    return false;
  }
  Serial.println("  Allocation succeeded");

  // Read pixel data
  Serial.println("  Reading pixels...");
  for (uint32_t i = 0; i < pixelCount; i++) {
    low = file.read();
    high = file.read();
    data[i] = (high << 8) | low;
    // Optional: print progress every 10000 pixels
    if (i % 10000 == 0) Serial.print(".");
  }
  Serial.println("\n  Done.");

  file.close();
  return true;
}

/**
 * Reads a .656 file in text format:
 *   height,width,{ 0xXXXX, 0xXXXX, ... };
 *
 * @param filename  Full path (e.g., "/Shangrila.656")
 * @param height    Output height
 * @param width     Output width
 * @param data      Output pointer to allocated uint16_t array (caller must delete[])
 * @return          true on success, false on error (including insufficient memory)
 */
bool read656FileText(const char* filename, uint16_t& height, uint16_t& width, uint16_t*& data) {
  File file = SD.open(filename, FILE_READ);
  if (!file) return false;

  // Helper: skip whitespace (space, tab, newline, carriage return)
  auto skipWhitespace = [&]() {
    while (file.available()) {
      char c = file.peek();
      if (c == ' ' || c == '\t' || c == '\n' || c == '\r') file.read();
      else break;
    }
  };

  // Helper: read a decimal integer (height or width)
  auto readDecimal = [&](uint16_t& value) -> bool {
    value = 0;
    skipWhitespace();
    if (!file.available()) return false;
    char c = file.peek();
    if (c < '0' || c > '9') return false;
    while (file.available()) {
      c = file.peek();
      if (c >= '0' && c <= '9') {
        file.read();
        value = value * 10 + (c - '0');
      } else break;
    }
    return true;
  };

  // Helper: read a hex number with 0x prefix
  auto readHex = [&](uint16_t& value) -> bool {
    value = 0;
    skipWhitespace();
    if (!file.available()) return false;
    if (file.read() != '0') return false;
    if (!file.available()) return false;
    char c = file.read();
    if (c != 'x' && c != 'X') return false;
    while (file.available()) {
      c = file.peek();
      if (c >= '0' && c <= '9') {
        file.read();
        value = (value << 4) | (c - '0');
      } else if (c >= 'a' && c <= 'f') {
        file.read();
        value = (value << 4) | (c - 'a' + 10);
      } else if (c >= 'A' && c <= 'F') {
        file.read();
        value = (value << 4) | (c - 'A' + 10);
      } else break;
    }
    return true;
  };

  // --- Parse height ---
  if (!readDecimal(height)) {
    file.close();
    return false;
  }

  // --- Expect comma ---
  skipWhitespace();
  if (!file.available() || file.read() != ',') {
    file.close();
    return false;
  }

  // --- Parse width ---
  if (!readDecimal(width)) {
    file.close();
    return false;
  }

  // --- Expect comma then '{' ---
  skipWhitespace();
  if (!file.available() || file.read() != ',') {
    file.close();
    return false;
  }
  skipWhitespace();
  if (!file.available() || file.read() != '{') {
    file.close();
    return false;
  }

  uint32_t expectedPixels = (uint32_t)height * (uint32_t)width;
  // Optional: reject images that are too large for your board
  if (expectedPixels > 320 * 240) {  // adjust limit as needed
    file.close();
    return false;
  }

  // Allocate memory with nothrow to avoid abort on failure
  data = new (std::nothrow) uint16_t[expectedPixels];
  if (data == nullptr) {
    file.close();
    return false;
  }

  // Read all hex values
  uint32_t count = 0;
  while (count < expectedPixels && file.available()) {
    skipWhitespace();
    char c = file.peek();
    if (c == '}') {
      file.read();  // consume '}'
      break;
    }
    uint16_t val;
    if (!readHex(val)) {
      delete[] data;
      file.close();
      return false;
    }
    data[count++] = val;

    skipWhitespace();
    if (!file.available()) break;
    c = file.peek();
    if (c == ',') {
      file.read();  // consume comma, continue
    } else if (c == '}') {
      // will be handled at next loop start
    } else {
      delete[] data;
      file.close();
      return false;
    }
  }

  // Optional: skip trailing ';' if present
  skipWhitespace();
  if (file.peek() == ';') file.read();

  file.close();

  if (count != expectedPixels) {
    delete[] data;
    return false;
  }
  return true;
}




void showimage16(int W, int H, const uint16_t* p) {

  tft.setSwapBytes(true);  // // Swap the byte order for pushImage() and pushPixels() - corrects endianness

  int16_t x = (tft.width() / 2) - (W / 2);   // Center horizontally
  int16_t y = (tft.height() / 2) - (H / 2);  // Center vertically

  tft.pushImage(x, y, W, H, p);  // Draw a small bitmap
}

void showimage(int W, int H, const uint8_t* p) {
  // Calculate center position for a 40x40 image on a 160x128 screen
  //  tft.setSwapBytes(true);
  int x = (tft.width() / 2) - (W / 2);
  int y = (tft.height() / 2) - (H / 2);

  // tft.pushImage(x, y, W, H, (const uint16_t *)p);
  tft.pushImage(x, y, W, H, (const uint8_t*)p, true);  // 8-bit image
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