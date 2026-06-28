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
  tft.println("SD Card init completed!");

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

  tft.fillScreen(TFT_BLACK);

  //waitForSerial();
}

void loop() {

}


void waitForSerial() {

  // Clear the serial buffer so it doesn't instantly loop again
  while (Serial.available())
    Serial.read();
  Serial.println("Press <SEND> (or type a character) to advance");
  // Wait here until a character is received over Serial
  while (!Serial.available())
    ;
}

