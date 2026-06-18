#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <TJpg_Decoder.h>
//#include <PNGdec.h>


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
//PNG png;


// 1️⃣ Define SPI class for SD
#ifdef VSPI_PIN
SPIClass sdSPI(VSPI);
#else
SPIClass sdSPI(HSPI);
#endif


// --- Callback for TJpg_Decoder (JPEG) ---
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
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

  if (clipW <= 0 || clipH <= 0) return 1;

  uint16_t *clipBuffer = new uint16_t[clipW * clipH];
  for (int16_t row = 0; row < clipH; row++) {
    memcpy(&clipBuffer[row * clipW], &bitmap[(row + (startY - y)) * w + (startX - x)], clipW * 2);
  }

  tft.pushImage(startX, startY, clipW, clipH, clipBuffer);
  delete[] clipBuffer;
  return 1;
}

/*
// --- Callback for PNGdec (PNG) ---
int pngDraw(PNGDRAW *pDraw) {
  int16_t x = pDraw->x;
  int16_t y = pDraw->y;
  uint16_t w = pDraw->iWidth;
  uint16_t h = 1;

  if (y >= 320) return 0;
  if (x >= 480) return 1;
  if (x + w <= 0) return 1;
  if (y + h <= 0) return 1;

  int16_t startX = max((int16_t)0, x);
  int16_t startY = max((int16_t)0, y);
  int16_t endX = min(480, x + w);
  int16_t endY = min(320, y + h);

  uint16_t clipW = endX - startX;
  uint16_t clipH = endY - startY;

  if (clipW <= 0 || clipH <= 0) return 1;

  uint16_t *rgb565 = new uint16_t[w];
  png.getLineAsRGB565(pDraw, rgb565, 0xFF, 0x000000);

  uint16_t *clipBuffer = new uint16_t[clipW * clipH];
  for (int16_t row = 0; row < clipH; row++) {
    memcpy(&clipBuffer[row * clipW], &rgb565[(row + (startY - y)) * w + (startX - x)], clipW * 2);
  }

  tft.pushImage(startX, startY, clipW, clipH, clipBuffer);
  delete[] rgb565;
  delete[] clipBuffer;
  
  return 1; // Important: Must return 1 to continue decoding
}


// --- PNG File IO Callbacks ---
void *myOpen(const char *filename, int32_t *size) {
  File f = SD.open(filename);
  *size = f.size();
  return new File(f);
}
void myClose(void *handle) {
  if (handle) delete (File*)handle;
}
int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length) {
  if (!handle) return -1;
  return ((File*)handle)->read(buffer, length);
}
int32_t mySeek(PNGFILE *handle, int32_t position) {
  if (!handle) return -1;
  return ((File*)handle)->seek(position);
}
*/

// --- Custom BMP Decoder (24-bit only) ---
void drawBMP(const char *filename, int16_t x, int16_t y) {
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
  uint8_t *rowBuffer = new uint8_t[rowSize];
  uint16_t *lineBuffer = new uint16_t[bmpWidth];

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
  tft.setSwapBytes(false);
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
      } else if (fileName.endsWith(".png")) {
        /* 
        png.open(fullPath.c_str(), myOpen, myClose, myRead, mySeek, pngDraw);
        png.decode(NULL, 0);
        png.close();
        */
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