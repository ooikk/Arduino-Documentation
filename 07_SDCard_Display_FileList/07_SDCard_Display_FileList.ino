#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>

#define TFT_DISPLAY_ON

TFT_eSPI tft = TFT_eSPI();
// --- SD Card Pins ---
#define SD_SCLK_PIN 4
#define SD_MISO_PIN 5  // Map to your un-shared SD_MISO line
#define SD_MOSI_PIN 6
#define SD_CS_PIN 7

// STEP 1: Instantiate a brand new SPI object explicitly assigned to the VSPI hardware block
SPIClass sdSPI(VSPI);

int16_t xStart, yStart;
int32_t screenWidth, screenHeight;

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  int y = 20;  // Y position for TFT printing

  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
#ifdef TFT_DISPLAY_ON
      tft.print("DIR: ");
      tft.println(file.name());
#endif
      y += 20;

      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
#ifdef TFT_DISPLAY_ON
      tft.print("FILE: ");
      tft.print(file.name());
      tft.print(" ");
      tft.print(file.size());
      tft.println("B");
#endif
      y += 20;
#ifdef TFT_DISPLAY_ON
      // Prevent printing off the screen
      if (y > 300) {
        tft.println("... (Screen full)");
        break;
      }
#endif
    }
    file = root.openNextFile();
  }
}

void listRootdir() {
  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open root directory");
#ifdef TFT_DISPLAY_ON
    tft.println("Failed to open root directory");
#endif
    return;
  }
  File file = root.openNextFile();
  while (file) {
    String fileName = String(file.name());

    String fullPath = "/" + fileName;
    Serial.println("Displaying: " + fullPath);
#ifdef TFT_DISPLAY_ON
    tft.printf("Displaying: /%s\n",fileName.c_str());
    //tft.println("Displaying: " + fullPath);
#endif
    file.close();
    file = root.openNextFile();
  }
  root.close();
}


void setup() {
  Serial.begin(115200);

  // STEP 2: Initialize your TFT display normally.
  // TFT_eSPI automatically configures its own HSPI bus using pins 11, 12, 13 [User_Setup.h]
  tft.init();
  tft.setRotation(1);  // Landscape 480x320
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.println("Initializing SD...");

  // STEP 3: Explicitly start the custom SD SPI bus with your chosen pins
  // Order: sclk, miso, mosi, ss
  Serial.println("Initializing Separate SPI Bus for SD...");
  sdSPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

  // STEP 4: Tell the SD library to hook into your custom 'sdSPI' instance instead of 'SPI'
  // Pass a safe clock frequency (e.g., 16MHz or 4MHz) for the separate bus
  Serial.println("Initializing SD Card...");
  if (!SD.begin(SD_CS_PIN, sdSPI, 16000000)) {
    Serial.println("SD Card Mount Failed!");
#ifdef TFT_DISPLAY_ON
    tft.println("SD Failed");
#endif
    return;
  }
  Serial.println("SD Card Mounted Successfully!");
#ifdef TFT_DISPLAY_ON
  tft.println("SD Card Mounted Successfully!");
#endif

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No physical SD card detected in slot.");
#ifdef TFT_DISPLAY_ON
    tft.println("No physical SD card detected in slot.");
#endif
    return;
  }

  Serial.print("SD Card Type Detected: ");
#ifdef TFT_DISPLAY_ON
  tft.println("SD Card Type Detected: ");
#endif

  switch (cardType) {
    case CARD_MMC:
      {
        Serial.println("MMC");
#ifdef TFT_DISPLAY_ON
        tft.println("MMC");
#endif
        break;
      }
    case CARD_SD:
      {
        Serial.println("SDSC");
#ifdef TFT_DISPLAY_ON
        tft.println("SDSC");
#endif
        break;
      }
    case CARD_SDHC:
      {
        Serial.println("SDHC");
#ifdef TFT_DISPLAY_ON
        tft.println("SDHC");
#endif
        break;
      }
    default:
      {
        Serial.println("UNKNOWN");
#ifdef TFT_DISPLAY_ON
        tft.println("UNKNOWN");
#endif
        break;
      }
  }
  
/*
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No physical SD card detected in slot.");
    return;
  }

  Serial.print("SD Card Type Detected: ");
  if(cardType == CARD_MMC)  Serial.println("MMC");
  else if(cardType == CARD_SD)   Serial.println("SDSC");
  else if(cardType == CARD_SDHC) Serial.println("SDHC");
  else                           Serial.println("UNKNOWN");

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  */

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
#ifdef TFT_DISPLAY_ON
  tft.printf("SD Card Size: %lluMB\n", cardSize);
#endif

  xStart = tft.getCursorX();
  yStart = tft.getCursorY();
  screenWidth = tft.width();
  screenHeight = tft.height();
}



void loop() {

#ifdef TFT_DISPLAY_ON
  //tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, yStart, screenWidth, (screenHeight - yStart), TFT_BLACK);
  tft.setCursor(0, yStart);
#endif

  listDir(SD, "/", 0);

  listRootdir();

  Serial.println("Finished scanning. Restarting in 5 seconds...");


#ifdef TFT_DISPLAY_ON
  //tft.fillScreen(TFT_BLACK);
  //  tft.setCursor(10, 10);
  tft.println("Done. Restarting in 5 seconds...");
#endif
  delay(5000);
}
