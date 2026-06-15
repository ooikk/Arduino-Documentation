#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

// MISO, MOSI and CLK are declared under Setup_user.h in TFT_eSPI
#define SD_CS_PIN 7
#define SD_FREQUENCY 16000000

TFT_eSPI tft = TFT_eSPI();

int16_t xStart, yStart;
int32_t screenWidth, screenHeight;

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
 //    (HSPI or VSPI, because of USE_HSPI_PORT or USE_FSPI_PORT in User_Setup.h)
  Serial.println("Initializing TFT...");
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.println("TFT ready...");
  tft.println("TFT ready...");

  // 3️⃣ Get the SPI bus instance that the TFT is using
  SPIClass& sdSPI = tft.getSPIinstance();

  // 4️⃣ Now initialize the SD card on the SAME SPI bus
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {
    Serial.println("SD Card initialization failed!");
    tft.println("SD Card init failed!");
    while (1) delay(1000);
  }
  Serial.println("SD Card initialized successfully.");
  tft.println("SD Card OK");

  readCardType();


  xStart = tft.getCursorX();
  yStart = tft.getCursorY();
  screenWidth = tft.width();
  screenHeight = tft.height();
  // tft.printf("Cursor: x=%d y=%d Screen: W=%d H=%d\n", xStart,yStart,screenWidth,screenHeight);

}



void loop() {
  //tft.fillScreen(TFT_BLACK);

  tft.fillRect(0, yStart, screenWidth, (screenHeight - yStart), TFT_BLACK);
  tft.setCursor(0, yStart);
  // Display Sub-directory 1 level down
  listDir(SD, "/", 1);

  listRootdir();

  Serial.println("Done. Press Enter to continue...");

  //tft.fillScreen(TFT_BLACK);
  //  tft.setCursor(10, 10);
  tft.println("Done. Press Enter to continue...");

  waitForSerial();

  //delay(5000);

}

void waitForSerial() {

  // Clear the serial buffer so it doesn't instantly loop again
  while (Serial.available())
    Serial.read();
  Serial.println("Press <SEND> (or type a character) to advance...");
  // Wait here until a character is received over Serial
  while (!Serial.available())
    ;
}

void readCardType() {

  // Optional: print card info
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  tft.printf("Size: %lluMB", cardSize);

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No physical SD card detected in slot.");
    tft.println("No physical SD card detected in slot.");
    return;
  }

  Serial.print("SD Card Type Detected: ");
  tft.println("SD Card Type Detected: ");
  switch (cardType) {
    case CARD_MMC:
      {
        Serial.println("MMC");
        tft.println("MMC");
        break;
      }
    case CARD_SD:
      {
        Serial.println("SDSC");
        tft.println("SDSC");
        break;
      }
    case CARD_SDHC:
      {
        Serial.println("SDHC");
        tft.println("SDHC");
        break;
      }
    default:
      {
        Serial.println("UNKNOWN");
        tft.println("UNKNOWN");
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

}



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
      tft.print("DIR: ");
      tft.println(file.name());
      y += 20;

      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());

      tft.print("FILE: ");
      tft.print(file.name());
      tft.print(" ");
      tft.print(file.size());
      tft.println("B");
      y += 20;

      // Prevent printing off the screen
      /*
      if (y > 300) {
        tft.println("... (Screen full)");
        break;
      }
      */
    }
    file = root.openNextFile();
  }
}

void listRootdir() {
  File root = SD.open("/");
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
}

