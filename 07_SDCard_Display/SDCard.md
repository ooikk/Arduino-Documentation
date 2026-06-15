# SD Card ILI9488

## Initialize the SD Card to use separate SPI bus from TFT Display   

Below is the general setup to initialize both SD card and ILI9488 TFT Display at separate SPI buses:  
- SD Card -> VSPI
- TFT Display -> HSPI (defined in User_setup.h)    

**Wiring Diagram - separate SPI**    

<img width="1207" height="679" alt="image" src="https://github.com/user-attachments/assets/42b4d4a6-25fb-4d4c-b98c-553a4afc759b" />

Here is the general code setup:    

```
#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

// 1️⃣ Define SPI pins for SD

#define VSPI_PIN   // SD card is using VSPI and TFT is using HSPI (defined in User_Setup.h)

#ifdef VSPI_PIN
#define SD_SCLK_PIN 4
#define SD_MISO_PIN 5
#define SD_MOSI_PIN 6
#else
#define SD_SCLK_PIN 12
#define SD_MISO_PIN 13
#define SD_MOSI_PIN 11
#endif

// Define the SD Chip Select pin (must match wiring)
#define SD_CS_PIN 7
#define SD_FREQUENCY 16000000  // 16MHz or 4MHz

TFT_eSPI tft = TFT_eSPI();

// 2️⃣ Define SPI class for SD
#ifdef VSPI_PIN
SPIClass sdSPI(VSPI);
#else
SPIClass sdSPI(HSPI);
#endif


void setup() {
  Serial.begin(115200);
  delay(1000);

/* Optional for separate SPI bus
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

// 3️⃣ Initialize the TFT. This also sets up the SPI bus
  Serial.println("Initializing TFT...");
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  Serial.println("TFT ready...");
  tft.println("TFT ready...");

// 4️⃣ Explicitly bind SPI to your SPI pins BEFORE SD.begin()
  sdSPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN);  // SCK, MISO, MOSI

  Serial.println("Initializing SD card...");
// 5️⃣ Now initialize the SD card on the second SPI bus
  if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {    
    Serial.println("SD Card initialization failed!");
    while (1) delay(1000);  // Halt
  }
  
  Serial.println("SD Card initialized successfully.");
  tft.println("SD Card OK");

// Your code here

}

void loop() {
    // ----
}
```
##  Initialize the SD Card to use same SPI bus with TFT Display   

On the ESP32‑S3, two different SPIClass objects cannot manage the same hardware SPI peripheral concurrently – this leads to a lock‑up when the TFT library tries to take control. To avoid ESP32-S3 hangs during SD card initialization when sharing the SPI bus with an ILI9488 TFT display:     
User_Setup.h is configured to use HSPI (USE_HSPI_PORT) or VSPI (USE_FSPI_PORT). The TFT_eSPI library will initialise that bus automatically inside tft.init(). You must not create any separate SPIClass object for the SD card. Instead:     
1. Get the SPI bus instance from the TFT library using tft.getSPIinstance().
2. Use that instance when initializing the SD card with SD.begin().
3. Pull the CS pins of all share SPI devices HIGH at the very top of your setup() function before doing anything else to avoid unassigned state.


**Wiring Diagram - share SPI**    

<img width="1169" height="694" alt="image" src="https://github.com/user-attachments/assets/fe8bbd4c-8ce9-48c2-a5cc-2865fa349e14" />

Here is the general code example:    
```
#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

#define SD_CS_PIN 7
#define SD_FREQUENCY 16000000   // 16MHz or 4MHz

TFT_eSPI tft = TFT_eSPI();

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
  //  (HSPI or VSPI, because of USE_HSPI_PORT or USE_FSPI_PORT in User_Setup.h)
  Serial.println("Initializing TFT...");
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  Serial.println("TFT ready...");
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

// Comtinue with the rest of the code from here  

}

void loop() {
  // Your code here
}
```


## WARNING: Sharing SPI bus expose the risk of SD card corruption     

SD card corruption after running ESP32 code strongly suggests a hardware or software conflict – especially since you are sharing the SPI bus between the TFT and the SD card. This is a common cause of file system corruption.

**1. Missing SPI transaction management – Critical!**    
When two devices share the same SPI bus, you must use SPI.beginTransaction() / SPI.endTransaction() to switch between them. Without this, the TFT and SD card may try to talk at the same time, causing garbage data and corrupting the SD card’s file system.

What your code should do:    
```
// Before accessing SD card
SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
digitalWrite(SD_CS, LOW);
// ... read from SD ...
digitalWrite(SD_CS, HIGH);
SPI.endTransaction();

// Before accessing TFT (the TFT library usually handles this internally)
// But ensure the TFT library also uses transactions (TFT_eSPI does by default)
```
If you mix libraries that both use SPI without coordination, corruption can occur.    

**2. TFT library may not release the SPI bus**     
TFT_eSPI uses SPI transactions by default, but if you call SD functions without endTransaction(), the TFT might still hold the bus. Always ensure the TFT is not selected before using the SD card.    

**3. Power brown‑out – ESP32 + TFT + SD can draw >300mA peak**     
If your USB power is weak, a sudden current drop can cause the SD card to reset or write garbage. Even reads can corrupt the card if power glitches during SPI.    

**4. SD card not closed properly before reset**   
If your ESP32 resets (watchdog, crash, or power loss) while the SD card is still selected (CS low) or a transaction is open, the card’s internal state may be corrupted.    

**✅ How to fix – Step by step**     

**Step 1: Ensure proper SPI transaction around SD access**   
Wrap every SD card operation (including SD.begin()) with transactions:    

```
#include <SPI.h>

// Define CS pins
#define TFT_CS   9   // example
#define SD_CS    7

void setup() {
    SPI.begin();

    // Initialise SD card with explicit transaction
    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    digitalWrite(SD_CS, LOW);
    if (!SD.begin(SD_CS)) {
        digitalWrite(SD_CS, HIGH);
        SPI.endTransaction();
        Serial.println("SD init failed");
        return;
    }
    digitalWrite(SD_CS, HIGH);
    SPI.endTransaction();

    // Now initialise TFT (TFT_eSPI will manage its own transactions)
    tft.begin();
    tft.setRotation(2);
}
```

**Step 2: In your file access function, wrap each file operation**   
The SD.open and file.read already use transactions internally if the SD library is compiled with USE_SPI_TRANSACTIONS. But to be safe, you can add explicit transactions around the file open/read loop.   

However, the simplest fix is to ensure that no SPI device is selected while the other is active. TFT_eSPI automatically de‑selects the TFT after each drawing command. But if you call tft.pushImage repeatedly, the TFT stays selected? No – pushImage ends with de‑select. So that’s likely fine.    

**Step 3: Lower SD card SPI speed**    

Many SD cards are unstable at 40 MHz. Force a lower speed:    
```
SD.begin(SD_CS, SPI, 4000000);  // 4 MHz
```
Add this after your transaction block.    

**Step 4: Add delays and flush after each file access**    
Example, after displaying an image, add a small delay to let the SD card finish any internal cleanup:    
```
displayBinary656(...);
delay(50);
```
Also, after closing the file, you can force a flush of the SD library cache (if any) by calling SD.end() before power down – but that’s only if you are about to sleep or reset.     

**Step 5: Use SdFat library**    
It allows you to explicitly pass a pointer to the SPI instance. When initializing your SD card, define the config like this:   
```
// Assuming you have a TFT_eSPI object named 'tft'
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(16), tft.getSPIinstance())
```    
This uses the getSPIinstance() method to fetch a pointer to the exact SPI object TFT_eSPI is using, guaranteeing they will cooperate correctly.    
Library SdFat.h by Bill Greiman    
https://github.com/greiman/SdFat

**Step 6: Check for accidental writes**    
Ensure you never open the file with FILE_WRITE. Use FILE_READ only.    
```
File file = SD.open(filename, FILE_READ);
```   
OR Use hardware "Lock" your SD Card.   


**🛠️ Advanced: Use separate SPI buses (recommended for stability)**     
ESP32 has two hardware SPI buses: VSPI (default) and HSPI. You can put the TFT on one and the SD card on the other.    


## Combined Test Sketch (Display + Touch + SD) with shared HSPI SPI bus   

This sketch initializes the display, checks if the SD card is present, and prints a basic touch prompt.    
***Note: You will need a microSD card formatted as FAT32 inserted into the module for the SD test to pass.***

```
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
#define SD_FREQUENCY 16000000

TFT_eSPI tft = TFT_eSPI();

// 1️⃣ Define SPI class for SD
#ifdef VSPI_PIN
SPIClass sdSPI(VSPI);
#else
SPIClass sdSPI(HSPI);
#endif

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 2️⃣ Set all CS pins high to deselect devices before initialization
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  delay(100);

  // 2️⃣ Initialize the TFT first. This also sets up the SPI bus
  //  (HSPI or VSPI, because of USE_HSPI_PORT or USE_FSPI_PORT in User_Setup.h)
  tft.init();
  tft.setRotation(1);  // Landscape
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 20);
  Serial.println("TFT ready...");
  tft.println("TFT ready...");

  // 3️⃣ Get the SPI bus instance that the TFT is using
  // SPIClass& sdSPI = tft.getSPIinstance();
  
  // 4️⃣ Explicitly bind SPI to your SPI pins BEFORE SD.begin()
  sdSPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN);  // SCK, MISO, MOSI

  // 5️⃣ Now initialize the SD card on the SAME SPI bus
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {
    Serial.println("SD Card initialization failed!");
    while (1) delay(1000);  // Halt
  }

  // Try open file
  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open root directory");
    tft.println("Failed to open root directory");
    return;
  }
  root.close();
  Serial.println("File open OK.");
  tft.println("File open OK");

  // run Touch_calibrate.ino from Example to do the calibration
  // Use this calibration code in setup():
  uint16_t calData[5] = { 263, 3627, 233, 3513, 7 };
  tft.setTouch(calData);

  tft.setCursor(20, 60);
  tft.print("SD Card: ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println("OK");

  // 2. Print SD card capacity
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    tft.println("No SD card attached");
  } else {
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    tft.print("Size: ");
    tft.print(cardSize);
    tft.println(" MB");
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

    // Draw a dot where you touch
    tft.fillCircle(x, y, 5, TFT_YELLOW);

    Serial.printf("Touch detected: X = %d, Y = %d\n", x, y);

    // Clear a small area and print the coordinates on screen
    tft.fillRect(20, 160, 200, 30, TFT_BLACK);
    tft.setCursor(20, 160);
    tft.printf("X: %d  Y: %d", x, y);

    delay(100);  // Debounce
    tft.fillCircle(x, y, 5, TFT_BLACK);
  }
}


```
## Display RGB565 Image    

Below is the 565RGB text file structure modified from generated online by (https://mischianti.org/rgb-image-to-byte-array-converter-for-arduino-tft-displays/). The code in 07_SDCard_Display_565.ino will load this file and send the pixel color to pushImage() function. The format is:    
  *Height,Width,{16-bit_pixels,16-bit_pixels....};*     
use 565 as the image file extention.
```
320,480,{
  0x7d1c, 0x7d1c, 0x7d1c,....
...
0x736d, 0x4a48
};
```

There could be memory limitaton to display 320×480 image (153,600 pixels → 307,200 bytes), ESP32 may still run out of RAM and enter continue reboot cycles.       
getFreeHeap command showed ESP32-S3 has about 320kB of RAM just before image display function start.    
- Free heap before open: 320020    
```
Serial.printf("Free heap before open: %u\n", ESP.getFreeHeap());
```
There are a few methods to over come the memory limitation.

**1. Process the file in chunks – avoid storing the whole array**    
If you only need the pixel data for streaming (e.g., to a display), you can parse the text file incrementally without storing all pixels at once. For example, read one line of hex values at a time and send them to the display or process them on the fly.   
Example in display565FileDirect() or displayBinary565() in sketch 07_SDCard_Display_565.ino.    

**2. Use a raw binary format instead of text**    
Text hex representation takes ~5 characters per pixel (e.g., 0xABCD,), so the file size is ~5× larger than binary. Parsing it also requires more temporary memory. An 320×480 image in ASCII 565RGB take about 1.17MB of file size, the file size reduced to 300kB in binary format.       

Convert images to raw binary format offline with Python script:     

Use little‑endian storage – it matches ESP32’s native byte order, giving the fastest possible read performance. Then enable byte swapping on the TFT:    
```
[2 bytes: height little-endian]
[2 bytes: width  little-endian]
[height × width × 2 bytes: pixel data (16‑bit RGB565, little‑endian)]
```
Store in little‑endian (recommended)     
- File contents:
  [height_L, height_H, width_L, width_H, pixel0_L, pixel0_H, pixel1_L, pixel1_H, ...]
  (low byte first, high byte second)
- Reader code:     
  Directly read into uint16_t variables. Because ESP32 is little‑endian, the in‑memory value will be correct (e.g., bytes 0x15, 0x6C become 0x6C15).
- TFT display:
  Call tft.setSwapBytes(true); once. The library will reverse the byte order when sending to the display (converting to big‑endian). This is very fast (done in DMA or SPI transfer).     

Advantages:     
- No conversion needed when reading from SD.
- Uses the native endianness of the ESP32 – fastest.
- tft.setSwapBytes(true) is a single instruction.

Below is the Python script to batch convert ASCII RGB565 images file to binary RGB565 file. 

*Refer to 1.8 TFT SPI Display section for online tool to convert images to RGB565 ASCII format.*     
- https://github.com/ooikk/Arduino-Documentation/blob/main/05_1.8_TFT_SPI_Display/1.8_TFT_Display.md

```
## run below command
## python 565_to_bin_batch.py
## all the file *.565 in current Directory will be converted to *.b565
##

import re
import os
import glob

def convert_text_565_to_binary(input_path, output_path):
    """Convert a single .565 text file to binary format."""
    with open(input_path, 'r') as f:
        content = f.read()
    
    # Remove all whitespace (spaces, newlines, tabs)
    compact = re.sub(r'\s+', '', content)
    
    # Format: height,width,{...};
    match = re.match(r'(\d+),(\d+),\{([^}]+)\};?', compact)
    if not match:
        raise ValueError("Invalid .565 text format: expected height,width,{...}")
    
    height = int(match.group(1))   # first number is ALWAYS height
    width  = int(match.group(2))   # second number is ALWAYS width
    hex_part = match.group(3).strip(',')
    
    # Split and clean
    hex_strings = [hs for hs in hex_part.split(',') if hs and hs.startswith('0x')]
    pixels = [int(hs, 16) for hs in hex_strings]
    
    expected = height * width
    if len(pixels) != expected:
        raise ValueError(f"Pixel count mismatch: {len(pixels)} vs {expected}")
    
    with open(output_path, 'wb') as out:
        out.write(height.to_bytes(2, 'little'))
        out.write(width.to_bytes(2, 'little'))
        for p in pixels:
            out.write(p.to_bytes(2, 'little'))
    
    print(f"✅ {input_path}: {height}x{width}, {len(pixels)} pixels -> {output_path}")

def batch_convert():
    # Find all .565 files in the current directory
    files = glob.glob("*.565")
    if not files:
        print("No .565 files found in current directory.")
        return
    
    print(f"Found {len(files)} .565 file(s). Converting...\n")
    
    for input_file in files:
        # Generate output filename: replace .565 with .b565
        output_file = input_file.replace(".565", ".b565")
        try:
            convert_text_565_to_binary(input_file, output_file)
        except Exception as e:
            print(f"❌ Error converting {input_file}: {e}\n")

if __name__ == "__main__":
    batch_convert()
```
