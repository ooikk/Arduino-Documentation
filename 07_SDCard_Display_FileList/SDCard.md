# SD Card ILI9488

## Initialize the SD Card to use separate SPI bus from TFT Display   

Below is the general setup to initialize both SD card and ILI9488 TFT Display at separate SPI buses:  
- SD Card -> VSPI
- TFT Display -> HSPI (defined in User_setup.h)    

```
#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

// Comment out below for SD card -> HSPI and TFT -> VSPI
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

// STEP 1: Instantiate a brand new SPI object explicitly assigned to the SPI hardware block
#ifdef VSPI_PIN
SPIClass sdSPI(VSPI);
#else
SPIClass sdSPI(HSPI);
#endif

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
  if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {
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

```
##  Initialize the SD Card to use same SPI bus with TFT Display   

To avoid ESP32-S3 hangs during SD card initialization when sharing the SPI bus with an ILI9488 TFT display:   
1. The initialization order is very critical.   
According to Espressif's official documentation on sharing SPI buses with SD cards, the SD card MUST be initialized BEFORE any other SPI device communicates on the bus:    
*"This step will put the SD card into the SPI mode, which SHOULD be done before all other SPI communications on the same bus. Otherwise the card will stay in the SD mode, in which mode it may randomly respond to any SPI communications on the bus, even when its CS line is not addressed."*    
2. Floating TFT Chip Select (CS) During SD Card Init
   When the ESP32-S3 executes SD.begin(SD_CS_PIN, SPI, SD_FREQUENCY), it floods the SPI bus with high-frequency commands to configure the display registers.
- If your TFT's Chip Select pin is floating or defaults to an unassigned input state when the microcontroller powers on, the TFT will assume it is being spoken to.
- The TFT tries to parse the SD initialization sequence, reads it as completely corrupted garbage data, and experiences an internal state-machine crash. By the time the code reaches tft.init(), the TFT's internal controller is completely locked up and will not respond, causing the ESP32-S3 to hang while waiting for a handshake token.

In your Arduino IDE code, you can initialize the SD card reader simultaneously alongside your display. Ensure you pass your custom SD_CS pin 7 to the initialization function:   

```
#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

// Define the SD Chip Select pin (must match wiring)
// Use VSPI pin to aviod conflict with HSPI
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
// Use VSPI pin to aviod conflict with HSPI
#define SD_SCLK_PIN 4
#define SD_MISO_PIN 5  
#define SD_MOSI_PIN 6
#define SD_CS_PIN 7

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
