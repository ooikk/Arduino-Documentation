#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

#define SD_SCLK_PIN 4
#define SD_MISO_PIN 5  // Map to your un-shared SD_MISO line
#define SD_MOSI_PIN 6
#define SD_CS_PIN 7

// STEP 1: Instantiate a brand new SPI object explicitly assigned to the VSPI hardware block
SPIClass sdSPI(VSPI);

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  delay(1000);
  // STEP 2: Initialize your TFT display normally.
  // TFT_eSPI automatically configures its own HSPI bus using pins 11, 12, 13 [User_Setup.h]
  Serial.println("Initializing TFT...");
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.println("TFT Ready!");

  // STEP 3: Explicitly start the custom SD SPI bus with your chosen pins
  // Order: sclk, miso, mosi, ss
  Serial.println("Initializing Separate SPI Bus for SD...");
  sdSPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

  // STEP 4: Tell the SD library to hook into your custom 'sdSPI' instance instead of 'SPI'
  // Pass a safe clock frequency (e.g., 16MHz or 4MHz) for the separate bus
  Serial.println("Initializing SD Card...");
  if (!SD.begin(SD_CS_PIN, sdSPI, 16000000)) {
    Serial.println("SD Card initialization failed!");
    tft.println("SD Card FAILED!");
  } else {
    Serial.println("SD Card ready!");
    tft.println("SD Card is ready!");
    Serial.println("ESP32 is ready!");
  }
}

void loop() {
  // Your code here
}