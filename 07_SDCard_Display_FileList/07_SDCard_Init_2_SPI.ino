#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

// Define the SD Chip Select pin (must match wiring)
#define VSPI_PIN   // SD card is using VSPI and TFT is using HSPI

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
#define SD_FREQUENCY 16000000  // 16MHz or 4MHz

TFT_eSPI tft = TFT_eSPI();

#ifdef VSPI_PIN
SPIClass sdSPI(VSPI);
#else
SPIClass sdSPI(HSPI);
#endif


void setup() {
  Serial.begin(115200);
  delay(1000);

/*
  // CRITICAL: Initialize SD card FIRST
  // Ensure TFT CS is HIGH (disabled) before SD init
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  delay(100);
*/

// Initialize TFT Display
  Serial.println("Initializing TFT...");
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  Serial.println("TFT ready...");
  tft.println("TFT ready...");

  // Explicitly bind SPI to your SPI pins BEFORE SD.begin()
  // SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN);
  sdSPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN);  // SCK, MISO, MOSI

  Serial.println("Initializing SD card...");
  //if (!SD.begin(SD_CS_PIN, SPI, SD_FREQUENCY)) {
  if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {    
    Serial.println("SD Card initialization failed!");
    while (1) delay(1000);  // Halt
  }
  
  Serial.println("SD Card initialized successfully.");
  tft.println("SD Card OK");

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  tft.printf("Size: %lluMB", cardSize);

  listRootdir();
  Serial.println("File List Successfully.");
  tft.println("File List Successfully.");
}

void loop() {
    // ----
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