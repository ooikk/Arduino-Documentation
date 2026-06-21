#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <TJpg_Decoder.h>
#include <PNGdec.h>


// Define the SD Chip Select pin (must match wiring)
#define VSPI_PIN  // SD card is using VSPI and TFT is using HSPI

// --- SD Card Pins ---
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
#define SD_FREQUENCY 4000000  //16000000  // 16MHz or 4MHz

TFT_eSPI tft = TFT_eSPI();

static PNG png;  // PNG decoder instance
static File* pngFileHandle = nullptr;
static TFT_eSPI* pTFT = nullptr;
static int16_t pngXOffset = 0;  // horizontal offset (for centering)
static int16_t pngYOffset = 0;  // vertical offset
static int16_t pngImgWidth = 0;
static int16_t pngImgHeight = 0;
static int16_t pngScreenW = 0;
static int16_t pngScreenH = 0;
static uint16_t* pngRowBuffer = nullptr;  // buffer for one clipped row

// 1️⃣ Define SPI class for SD
#ifdef VSPI_PIN
SPIClass sdSPI(VSPI);
#else
SPIClass sdSPI(HSPI);
#endif


// --- Callback for TJpg_Decoder (JPEG) ---
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= 320) return 0;
  if (x >= 480) return 1;
  if (x + w <= 0) return 1;
  if (y + h <= 0) return 1;

  // Clip to screen boundaries
  int16_t startX = max((int16_t)0, x);
  int16_t startY = max((int16_t)0, y);
  int16_t endX = min(480, x + w);
  int16_t endY = min(320, y + h);

  uint16_t clipW = endX - startX;
  uint16_t clipH = endY - startY;
  
  tft.setSwapBytes(false);

  if (clipW <= 0 || clipH <= 0) return 1;

  uint16_t* clipBuffer = new uint16_t[clipW * clipH];
  for (int16_t row = 0; row < clipH; row++) {
    memcpy(&clipBuffer[row * clipW], &bitmap[(row + (startY - y)) * w + (startX - x)], clipW * 2);
  }

  tft.pushImage(startX, startY, clipW, clipH, clipBuffer);
  delete[] clipBuffer;
  return 1;
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
  // Verify PNG signature
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

static int pngDraw(PNGDRAW* pDraw) {
    int srcY = pDraw->y;
    int imgW = pDraw->iWidth;
    int tgtY = srcY + pngYOffset;

    if (tgtY < 0 || tgtY >= pngScreenH) return 1;

    // Allocate line buffer (safe even for large images)
    uint16_t* lineBuffer = (uint16_t*)malloc(imgW * sizeof(uint16_t));
    if (!lineBuffer) return 0;

    // Use little‑endian (ESP32) and black background for transparency
    png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_LITTLE_ENDIAN, 0x000000);

    // Horizontal clipping
    int srcStart = 0, srcEnd = imgW;
    int tgtStart = pngXOffset;
    if (tgtStart < 0) {
        srcStart = -tgtStart;
        tgtStart = 0;
    }
    if (tgtStart + imgW > pngScreenW) {
        srcEnd = pngScreenW - tgtStart;
    }
    pTFT->setSwapBytes(true);
    int clipW = srcEnd - srcStart;
    if (clipW > 0) {
        // Copy visible slice
        for (int i = 0; i < clipW; i++) {
            pngRowBuffer[i] = lineBuffer[srcStart + i];
        }
        // Push without swapping (since we used LITTLE_ENDIAN)
        pTFT->pushImage(tgtStart, tgtY, clipW, 1, pngRowBuffer);
    }
    free(lineBuffer);
    return 1;
}

bool displayPNG(const char* filename, TFT_eSPI& tft) {
    // Clean up previous session
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

    if (!SD.exists(filename)) {
        Serial.printf("File NOT found: %s\n", filename);
        return false;
    }

    int rc = png.open(filename, pngOpen, pngClose, pngRead, pngSeek, pngDraw);
    if (rc != PNG_SUCCESS) {
        Serial.printf("PNG open failed: %d for %s\n", rc, filename);
        return false;
    }

    pngImgWidth = png.getWidth();
    pngImgHeight = png.getHeight();
    Serial.printf("PNG: %dx%d\n", pngImgWidth, pngImgHeight);

    pngScreenW = tft.width();
    pngScreenH = tft.height();

    // Center if image fits, otherwise crop from (0,0)
    pngXOffset = (pngImgWidth < pngScreenW) ? (pngScreenW - pngImgWidth) / 2 : 0;
    pngYOffset = (pngImgHeight < pngScreenH) ? (pngScreenH - pngImgHeight) / 2 : 0;

    // Allocate a buffer large enough for one screen row (in pixels)
    pngRowBuffer = (uint16_t*)malloc(pngScreenW * sizeof(uint16_t));
    if (!pngRowBuffer) {
        Serial.println("Row buffer allocation failed");
        png.close();
        return false;
    }

    pTFT = &tft;

    tft.startWrite();
    uint32_t start = millis();
    rc = png.decode(nullptr, 0);
    Serial.printf("Decode time: %d ms\n", millis() - start);
    tft.endWrite();

    // Cleanup
    png.close();
    if (pngFileHandle) {
        pngFileHandle->close();
        delete pngFileHandle;
        pngFileHandle = nullptr;
    }
    free(pngRowBuffer);
    pngRowBuffer = nullptr;

    return (rc == PNG_SUCCESS);
}

// --- Custom BMP Decoder (24-bit only) ---
void drawBMP(const char* filename, int16_t x, int16_t y) {
  File bmpFile = SD.open(filename);
  if (!bmpFile) {
    Serial.println("BMP file not found");
    return;
  }

  uint32_t bmpWidth, bmpHeight;
  uint16_t bmpDepth;
  uint32_t bmpImageoffset;
  tft.setSwapBytes(true);
  bmpFile.seek(18);
  bmpWidth = bmpFile.read() | (bmpFile.read() << 8) | (bmpFile.read() << 16) | (bmpFile.read() << 24);
  bmpHeight = bmpFile.read() | (bmpFile.read() << 8) | (bmpFile.read() << 16) | (bmpFile.read() << 24);
  bmpFile.seek(28);
  bmpDepth = bmpFile.read() | (bmpFile.read() << 8);
  bmpFile.seek(10);
  bmpImageoffset = bmpFile.read() | (bmpFile.read() << 8) | (bmpFile.read() << 16) | (bmpFile.read() << 24);

  if (bmpDepth != 24) {
    Serial.println("Only 24-bit BMP supported");
    bmpFile.close();
    return;
  }

  // Center cropping logic
  int16_t startX = x + (480 - bmpWidth) / 2;
  int16_t startY = y + (320 - bmpHeight) / 2;
  if (startX < 0) startX = 0;
  if (startY < 0) startY = 0;

  bmpFile.seek(bmpImageoffset);
  uint32_t rowSize = (bmpWidth * 3 + 3) & ~3;  // 4-byte aligned rows
  uint8_t* rowBuffer = new uint8_t[rowSize];
  uint16_t* lineBuffer = new uint16_t[bmpWidth];

  for (int32_t row = 0; row < bmpHeight; row++) {
    bmpFile.read(rowBuffer, rowSize);
    int16_t drawY = startY + (bmpHeight - 1 - row);  // BMP is bottom-up

    if (drawY >= 0 && drawY < 320) {
      for (int32_t col = 0; col < bmpWidth; col++) {
        int16_t drawX = startX + col;
        if (drawX >= 0 && drawX < 480) {
          uint8_t b = rowBuffer[col * 3];
          uint8_t g = rowBuffer[col * 3 + 1];
          uint8_t r = rowBuffer[col * 3 + 2];
          lineBuffer[col] = tft.color565(r, g, b);
        }
      }
      tft.pushImage(startX, drawY, bmpWidth, 1, lineBuffer);
    }
  }
  delete[] rowBuffer;
  delete[] lineBuffer;
  bmpFile.close();
}

void setup() {
  Serial.begin(115200);

  // 2️⃣ Set all CS pins high to deselect devices before initialization
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  delay(100);

  // 3️⃣ Initialize the TFT first. This also sets up the SPI bus
  tft.init();
  tft.setRotation(1);  // Landscape 480x320
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Initializing SD...");


  // 3️⃣ Get the SPI bus instance that the TFT is using
  // SPIClass& sdSPI = tft.getSPIinstance();
  // 4️⃣ Explicitly bind SPI to your SPI pins BEFORE SD.begin()
  // SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN);
  sdSPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN);  // SCK, MISO, MOSI

  // 5️⃣ Now initialize the SD card on the SPI bus
  if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {
    Serial.println("SD Card Mount Failed! Try again...");
    delay(1000);
    if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {
      Serial.println("SD Card Mount Failed!");
      tft.println("SD Card init failed!");
      while (1) delay(1000);
    }
  }

  Serial.println("SD Card Mounted Successfully!");
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open root directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    String fileName = String(file.name());
    // Check for image extensions
    if (fileName.endsWith(".jpg") || fileName.endsWith(".jpeg") || fileName.endsWith(".png") || fileName.endsWith(".bmp")) {

      String fullPath = "/" + fileName;
      Serial.println("Displaying: " + fullPath);
      tft.fillScreen(TFT_BLACK);

      // Display image based on extension
      if (fileName.endsWith(".jpg") || fileName.endsWith(".jpeg")) {
        TJpgDec.setJpgScale(1);
        TJpgDec.setSwapBytes(true);
        TJpgDec.setCallback(tft_output);
        TJpgDec.drawSdJpg(0, 0, fullPath.c_str());
      } else if (fileName.endsWith(".png") || fileName.endsWith(".PNG")) {
        String fullPath = "/" + fileName;
        tft.fillScreen(TFT_BLACK);
        if (displayPNG(fullPath.c_str(), tft)) {
          Serial.println("PNG OK");
        } else {
          Serial.println("PNG failed");
        }
        delay(2000);


      } else if (fileName.endsWith(".bmp")) {
        drawBMP(fullPath.c_str(), 0, 0);
      }

      delay(2000);  // Wait 2 seconds
    }
    file.close();
    file = root.openNextFile();
  }
  root.close();

  Serial.println("Finished scanning. Restarting in 5 seconds...");
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 10);
  tft.println("Done. Restarting...");
  delay(5000);
}