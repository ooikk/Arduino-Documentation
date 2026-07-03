// This is the rotate function using offline scaled image size

// Define the SD Chip Select pin (must match wiring)
#define VSPI_PIN  // SD card is using VSPI and TFT is using HSPI

#include <SPI.h>
#include <SD.h>
//#include "SdFat.h"
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
#define SD_FREQUENCY 16000000  // 16MHz or 4MHz

TFT_eSPI tft = TFT_eSPI();

#ifdef VSPI_PIN
SPIClass sdSPI(VSPI);
#else
SPIClass sdSPI(HSPI);
#endif

// Clock hand 565RGB image
#include <scaled_clockhand.h>
// Create an instance of the sprite class for flickering-free smooth rotation
TFT_eSprite needle(&tft);
TFT_eSprite meterArea(&tft);


int angle = 0;

// ---------- User Settings ----------

#define NEEDLE_CENTERX 7   // Original pivot X (from top-left)
#define NEEDLE_CENTERY 86  // Original pivot Y (from top-left)

// Display pivot (where the needle rotates around on screen)
#define SCREEN_PIVOT_X 120  // Example: center of a 320x240 screen
#define SCREEN_PIVOT_Y 140

#define METER_X 50
#define METER_Y 50


void setup() {
  Serial.begin(115200);
  delay(1000);



  // Initialize TFT Display
  Serial.println("Initializing TFT...");
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);

  Serial.println("TFT ready...");
  // tft.println("TFT ready...");

  // Explicitly bind SPI to your SPI pins BEFORE SD.begin()
  // SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN);
  sdSPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN);  // SCK, MISO, MOSI

  Serial.println("Initializing SD card...");
  //if (!SD.begin(SD_CS_PIN, SPI, SD_FREQUENCY)) {
  if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {
    Serial.println("SD Card initialization failed! Try again...");
    if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {
      while (1) delay(1000);  // Halt
    }
  }

  Serial.println("SD Card initialized successfully.");

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  needle.setSwapBytes(true);
  needle.createSprite(CLOCKHAND_WIDTH, CLOCKHAND_HEIGHT);
  needle.pushImage(0, 0, CLOCKHAND_WIDTH, CLOCKHAND_HEIGHT, clockhand);
  needle.setPivot(NEEDLE_CENTERX, NEEDLE_CENTERY);

  meterArea.createSprite(CLOCKHAND_HEIGHT * 2, CLOCKHAND_HEIGHT * 2);
  meterArea.fillSprite(TFT_BLACK);
  meterArea.setPivot(CLOCKHAND_HEIGHT, CLOCKHAND_HEIGHT);

  // Set the screen pivot (where the needle will rotate)
  //tft.setPivot(SCREEN_PIVOT_X, SCREEN_PIVOT_Y);

  // Draw static text (will be redrawn each loop)
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Rotating Needle", 10, 10);
}

void loop() {

  // fill in the area with background color
  // Method 1: push to the sprite
  //needle.fillSprite(TFT_BLACK);
  //needle.pushRotated(angle_old, TFT_WHITE);

  // Method 2: just fill circle to tft directly
  // tft.fillCircle(SCREEN_PIVOT_X,SCREEN_PIVOT_Y,CLOCKHAND_HEIGHT,TFT_BLACK);
  meterArea.fillSprite(TFT_BLACK);
  meterArea.drawCircle(CLOCKHAND_HEIGHT, CLOCKHAND_HEIGHT, CLOCKHAND_HEIGHT, TFT_YELLOW);
  meterArea.drawCircle(CLOCKHAND_HEIGHT, CLOCKHAND_HEIGHT, CLOCKHAND_HEIGHT - 1, TFT_YELLOW);
  needle.pushImage(0, 0, CLOCKHAND_WIDTH, CLOCKHAND_HEIGHT, clockhand);
  needle.pushRotated(&meterArea, angle, TFT_BLACK);

  meterArea.pushSprite(METER_X, METER_Y);

  // 4. Update angle for next frame
  angle = (angle + 1) % 360;

  delay(10);  // Adjust speed
}
