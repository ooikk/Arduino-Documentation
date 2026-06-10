# SD Card ILI9488

## Activating the SD Card   

In your Arduino IDE code, you can initialize the SD card reader simultaneously alongside your display. Ensure you pass your custom SD_CS pin 7 to the initialization function:   

```
#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>
#define SD_CS_PIN 7

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  
  // Initialize the TFT display
  tft.init();
  tft.setRotation(1);
  
  // Initialize the SD Card using the shared SPI bus
  if (!SD.begin(SD_CS_PIN)) { 
    Serial.println("SD Card initialization failed!");
  } else {
    Serial.println("SD Card ready!");
  }
}

void loop() {}

```
## Combined Test Sketch (Display + Touch + SD)   

This sketch initializes the display, checks if the SD card is present, and prints a basic touch prompt.    
***Note: You will need a microSD card formatted as FAT32 inserted into the module for the SD test to pass.***

```
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>

TFT_eSPI tft = TFT_eSPI();

// Define the SD Chip Select pin (must match wiring)
#define SD_CS_PIN 7
#define SD_MISO_PIN 13
#define SD_MOSI_PIN 11
#define SD_SCLK_PIN 12

void setup() {
  Serial.begin(115200);
  
  // 1. Initialize Display
  tft.init();
  tft.setRotation(1); // Landscape
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 20);
  tft.println("Initializing...");

  // 2. Initialize SD Card
  tft.setCursor(20, 60);
  tft.print("SD Card: ");
  SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN);  // SCK, MISO, MOSI 
  // SPI transactions ensure the SD library doesn't conflict with TFT_eSPI
  if (!SD.begin(SD_CS_PIN, SPI, 27000000)) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("FAILED");
    Serial.println("SD Card initialization failed!");
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("OK");
    Serial.println("SD Card initialized successfully.");
    
    // Optional: Print SD card capacity
    uint8_t cardType = SD.cardType();
    if(cardType == CARD_NONE){
        tft.println("No SD card attached");
    } else {
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        tft.print("Size: ");
        tft.print(cardSize);
        tft.println(" MB");
    }
  }

  // 3. Touch Prompt
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(20, 120);
  tft.println("Touch screen to");
  tft.println("test coordinates.");
}

void loop() {
  // Check for touch
  uint16_t x = 0, y = 0;
  bool touched = tft.getTouch(&x, &y);

  if (touched) {
    Serial.printf("Touch detected: X = %d, Y = %d\n", x, y);
    
    // Clear a small area and print the coordinates on screen
    tft.fillRect(20, 160, 200, 30, TFT_BLACK);
    tft.setCursor(20, 160);
    tft.printf("X: %d  Y: %d", x, y);
    
    delay(100); // Debounce
  }
}
```
