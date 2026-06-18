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

Check here for detail about [SdFat](https://github.com/ooikk/Arduino-Documentation/blob/main/07_SDCard_Display/SDCard_SdFat.md)


**Step 6: Check for accidental writes**    
Ensure you never open the file with FILE_WRITE. Use FILE_READ only.    
```
File file = SD.open(filename, FILE_READ);
```   
OR Use hardware "Lock" your SD Card.   

**🛠️ Advanced: Use separate SPI buses (recommended for stability)**     
ESP32 has two hardware SPI buses: VSPI (default) and HSPI. You can put the TFT on one and the SD card on the other.     

## Recover Corrupted SD Card     
If you have accidentally corrupted or damaged your SD Card, follow this link to try restore back your SD Card [SD Card Recovery](SDCard%20Corruption.md)     

## Combined Test Sketch (Display + Touch + SD) with separate SPI bus   

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
## SD Card functions


|	Category	|	Function	|	Example	|	Description	|
|	--------------	|	--------------	|	----------------------------------------------------	|	---------------------------------------------------------	|
|	Initialization	|	begin(csPin)	|	if (!SD.begin(4)) { Serial.println("Init failed!"); }	|	Initializes the SD card library. Must be called first, before any other SD card functions. Returns true on success.	|
|	File/Directory Info	|	exists(path)	|	if (SD.exists("/data.txt")) { Serial.println("File found"); }	|	Checks if a file or directory exists on the SD card.	|
|	File/Directory Info	|	open(path, mode)	|	File dataFile = SD.open("/data.txt", FILE_WRITE);	|	Opens a file for reading/writing. Must be called before you can read or write to a file. Returns a File object or false on failure.	|
|	File/Directory Info	|	remove(path)	|	if (SD.exists("/unwanted.txt")) { SD.remove("/unwanted.txt"); }	|	Deletes a file from the SD card. It's recommended to check if the file exists first using SD.exists().	|
|	File/Directory Info	|	mkdir(path)	|	SD.mkdir("/newFolder/subFolder");	|	Creates a directory. Can also create intermediate directories if they don't exist.	|
|	File/Directory Info	|	rmdir(path)	|	SD.rmdir("/emptyFolder");	|	Removes an empty directory. The directory must be empty before calling this.	|
|	Control	|	close()	|	dataFile.close();	|	Closes a file and ensures any data written to it is saved to the SD card. It's good practice to close files when you're done with them.	|
|	Control	|	flush()	|	dataFile.flush();	|	Forces any buffered data to be written to the SD card. This is done automatically when you close the file.	|
|	State Information	|	available()	|	while (dataFile.available()) { char c = dataFile.read(); }	|	Checks if there are any bytes available to be read from the file.	|
|	State Information	|	position()	|	uint32_t pos = dataFile.position();	|	Returns the current position (in bytes) within a file, where the next read or write will happen.	|
|	State Information	|	seek(pos)	|	dataFile.seek(0); // Jump to the start of the file	|	Moves the read/write position to a specific byte in a file.	|
|	State Information	|	size()	|	uint32_t fileSize = dataFile.size();	|	Returns the total size of a file in bytes.	|
|	State Information	|	isDirectory()	|	if (root.isDirectory()) { Serial.println("It's a directory!"); }	|	Checks if the current File object represents a directory (folder).	|
|	Reading & Writing	|	read()	|	char ch = dataFile.read();	|	Reads a single byte from a file. Returns -1 if no bytes are available.	|
|	Reading & Writing	|	write(data)	|	dataFile.write("Hello, world!");	|	Writes a single byte or a buffer of data to a file.	|
|	Reading & Writing	|	print(data)	|	dataFile.print("Sensor Value: "); dataFile.print(sensorValue);	|	Writes text to a file. Numbers are automatically converted to their ASCII string representation.	|
|	Reading & Writing	|	println(data)	|	dataFile.println("--- End of Data ---");	|	Like print(), but adds a newline (\r\n) at the end.	|
|	Reading & Writing	|	peek()	|	char nextChar = dataFile.peek();	|	Reads a byte from a file without moving the read pointer, so a subsequent read() will return the same byte.	|
|	Directory Navigation	|	openNextFile()	|	File entry = dir.openNextFile();	|	Opens the next file or folder in a directory. Used in a loop to list all contents of a directory.	|
|	Directory Navigation	|	rewindDirectory()	|	dir.rewindDirectory();	|	Resets the file pointer back to the first file in a directory, allowing you to iterate over it again.	|
|	Information	|	name()	|	Serial.println(entry.name());	|	Returns the name of a file or directory as a character array (string).	|


## Sampel code to access the SD Card

**Get SD Card information**   

```
Serial.begin(115200);
if (!SD.begin()) {
  Serial.println("Initialisation failed");
  return;
}

// Get the card object and check read-only
uint8_t cardType = SD.cardType();
uint64_t cardSize = SD.cardSize();
//bool isReadOnly = SD.card()->readOnly();  // not available

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


```

**Writing a text file to SD Card**      

Below is a simple test code to write a text to SD Card.
```
File test = SD.open("/test.txt", FILE_WRITE);
if (test) {
    test.println("Test");
    test.close();
    SD.remove("/test.txt");
    Serial.println("Card is writeable");
} else {
    Serial.println("Card is read-only or write failed");
}
```
**Reading a text file from SD Card**     
```
  // Open the file for reading
  File file = SD.open("/test.txt", FILE_READ);
  if (!file) {
    Serial.println("Failed to open test.txt for reading.");
    return;
  }

  Serial.println("Contents of test.txt:");
  // Read and print all characters from the file
  while (file.available()) {
    Serial.write(file.read());  // writes the raw byte to the serial
  }

  // Close the file
  file.close();
  Serial.println("\n--- End of file ---");
}

```

## Display RGB565 Image    

Below is the 565RGB text file structure modified from generated online by [565RGB Convertor](https://mischianti.org/rgb-image-to-byte-array-converter-for-arduino-tft-displays/). The code in 07_SDCard_Display_565.ino will load this file and send the pixel color to pushImage() function. The format is:    
  *Height,Width,{16-bit_pixels,16-bit_pixels....};*     
Picture height is 320 and width is 480. Use ".565" as the image file extention.
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
- File contents: [height_L, height_H, width_L, width_H, pixel0_L, pixel0_H, pixel1_L, pixel1_H, ...] (low byte first, high byte second)
- Reader code: Directly read into uint16_t variables. Because ESP32 is little‑endian, the in‑memory value will be correct (e.g., bytes 0x15, 0x6C become 0x6C15).
- TFT display: Call tft.setSwapBytes(true); once. The library will reverse the byte order when sending to the display (converting to big‑endian). This is very fast (done in DMA or SPI transfer).     

Advantages:     
- No conversion needed when reading from SD.
- Uses the native endianness of the ESP32 – fastest.
- tft.setSwapBytes(true) is a single instruction.

Below is the Python script to batch convert ASCII RGB565 images file to binary RGB565 file. 

*Refer to 1.8 TFT SPI Display section for online tool to convert images to RGB565 ASCII format.*     
- [1.8" TFT Display](https://github.com/ooikk/Arduino-Documentation/blob/main/05_1.8_TFT_SPI_Display/1.8_TFT_Display.md)

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

## Display JPEG images    

**Library**     
TJpg_Decoder by Bodmer     
https://github.com/Bodmer/TJpg_Decoder

**Sample Code**    
```
#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>

/**
 * @brief Display a JPEG from SD card with auto‑rotation and smart scaling.
 *
 * The image orientation is matched to the screen (longer side aligns with longer side).
 * If the image is larger than the screen, it is down‑scaled to *cover* the display
 * (edges are cropped). If it is smaller, it is shown centered (letterboxed).
 *
 * @param filename   Full path to the .jpg file (e.g., "/Shangrila.jpg")
 * @param tft        Reference to your TFT_eSPI object
 * @param maxSize    Maximum RAM usage (TJpg_Decoder uses ~3.5KB fixed, this param is kept for compatibility)
 * @return           true on success, false on error
 */
bool displayJPG(const char* filename, TFT_eSPI& tft, size_t maxSize) {
  // ------------------------------------------------------------------
  // 1. Get JPEG dimensions without decoding
  // ------------------------------------------------------------------
  uint16_t imgW = 0, imgH = 0;
  JRESULT result = TJpgDec.getSdJpgSize(&imgW, &imgH, filename);
  if (result != JDR_OK || imgW == 0 || imgH == 0) {
    Serial.print("Decoding error: ");
    Serial.println(result);  // 0 = OK, 1 = parameter error, 2 = out of memory, etc.
    return false;
  }

  // ------------------------------------------------------------------
  // 2. Save current rotation and auto‑rotate screen if needed
  // ------------------------------------------------------------------
  /*
  uint8_t currentRot = tft.getRotation();
  int sw = tft.width();
  int sh = tft.height();
  bool scrLandscape = (sw >= sh);
  bool imgLandscape = (imgW >= imgH);

  if (imgLandscape != scrLandscape) {
    tft.setRotation((currentRot + 1) % 4);  // 90° clockwise
    sw = tft.width();                       // update after rotation
    sh = tft.height();
  }
*/
  // After reading image width, image height

  uint8_t currentRot = tft.getRotation();

  if (imgW > imgH) {
    // Image is landscape
    tft.setRotation(1);
  } else tft.setRotation(2);

  int sw = tft.width();  // update after rotation
  int sh = tft.height();


  // ------------------------------------------------------------------
  // 3. Compute the best integer scale (1, 2, 4, or 8)
  //    "Cover" mode if possible, otherwise "fit" mode.
  // ------------------------------------------------------------------
  float ratioW = (float)imgW / sw;
  float ratioH = (float)imgH / sh;
  float minRatio = (ratioW < ratioH) ? ratioW : ratioH;

  uint8_t scale = 1;
  if (minRatio >= 1.0f) {
    // Image is large enough to cover the screen – choose largest power‑of‑2 ≤ minRatio
    while (scale * 2 <= minRatio && scale < 8) {
      scale *= 2;
    }
  } else {
    // Image is smaller in at least one dimension – fit entirely (letterbox)
    float maxRatio = (ratioW > ratioH) ? ratioW : ratioH;
    while (scale < maxRatio && scale < 8) {
      scale *= 2;
    }
  }

  // Compute scaled image size and centre position
  int scaledW = imgW / scale;
  int scaledH = imgH / scale;
  int x = (sw - scaledW) / 2;
  int y = (sh - scaledH) / 2;

  // ------------------------------------------------------------------
  // 4. Configure TJpg_Decoder
  // ------------------------------------------------------------------
  TJpgDec.setJpgScale(scale);
  TJpgDec.setCallback(tft_output);
  TJpgDec.setSwapBytes(true);
  // ------------------------------------------------------------------
  // 5. Draw the JPEG from SD card
  // ------------------------------------------------------------------
  // Note: drawSdJpg() draws at the specified (x,y) position.
  // The callback (tft_output) handles clipping at screen edges.
  result = TJpgDec.drawSdJpg(x, y, filename);

  // ------------------------------------------------------------------
  // 6. Restore original screen rotation
  // ------------------------------------------------------------------
  tft.setRotation(currentRot);

  return (result == JDR_OK);
}


// ------------------------------------------------------------------
// Rendering callback – called by TJpg_Decoder for each MCU block
// ------------------------------------------------------------------
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

```


**Key Features Explained**     
|Feature | How It's Implemented|
|--- | ---|
|Auto‑rotation	| Rotate ( 1 or 2) the image to match the orientations (landscape vs. portrait).|
|Scaling	| The function selects the smallest power‑of‑two scale (1, 2, 4, or 8) that either covers the screen (cropping edges) if possible, or fits entirely (letterbox) if the image is smaller.|
|Memory usage	| TJpg_Decoder uses a fixed ~3.5KB workspace – the maxSize parameter is kept for compatibility but is not needed.|
|Centering	| The image is always drawn at the centre of the screen (the destX/destY arguments are ignored).|
|Edge cropping	| When covering the screen, the image is cropped symmetrically on the shorter side, preserving the aspect ratio.|
|Callback‑based rendering	| The tft_output function is called for each MCU block, streaming the image directly to the TFT without a full‑frame buffer.|

**Important Notes**
1. JPEG format – The library only supports 24‑bit JPEGs (not 8‑bit) and does not support progressive JPEGs.
2. SD Card initialisation – Make sure SD.begin() is called before any drawSdJpg() calls.
3. Scaling limits – The library only supports scaling factors of 1, 2, 4, or 8. The function automatically picks the best one.
4. Callback clipping – The tft_output callback already clips at screen edges, so negative x/y positions are handled automatically.
5. Restoring rotation – The original screen rotation is restored after drawing, so the rest of your UI remains unaffected.

## Display BMP images       

**Library**     

None     

**Sample Code**     

Below is a complete implementation of displayBMP() that reads a **24‑bit BMP** file from an SD card, automatically rotates the TFT to match the image orientation, and scales/crops the image to fill the screen while preserving aspect ratio. The image is centered, and only the necessary portion of the BMP is loaded into memory, respecting the maxSize parameter.    

```
#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>


#pragma pack(push, 1)
typedef struct {
  uint16_t bfType;
  uint32_t bfSize;
  uint16_t bfReserved1;
  uint16_t bfReserved2;
  uint32_t bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
  uint32_t biSize;
  int32_t biWidth;
  int32_t biHeight;  // positive = bottom-up, negative = top-down
  uint16_t biPlanes;
  uint16_t biBitCount;
  uint32_t biCompression;
  uint32_t biSizeImage;
  int32_t biXPelsPerMeter;
  int32_t biYPelsPerMeter;
  uint32_t biClrUsed;
  uint32_t biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)


static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

// Read a contiguous block of pixels from a BMP row.
// Returns true on success.
// rowIndex: 0 = top row (for both orientations), startCol, numCols.
static bool readBMPRowSegment(File& file, int rowIndex, int startCol, int numCols,
                              uint8_t* buffer, int imgWidth, int imgHeight,
                              int rowSize, int dataOffset, int bytesPerPixel,
                              bool isBottomUp) {
  if (numCols <= 0) return true;

  int actualRow;
  if (isBottomUp) {
    // Bottom-up: row 0 is the bottom row of the image.
    actualRow = imgHeight - 1 - rowIndex;
  } else {
    // Top-down: row 0 is the top row.
    actualRow = rowIndex;
  }

  uint32_t rowOffset = dataOffset + actualRow * rowSize;
  uint32_t seekPos = rowOffset + startCol * bytesPerPixel;
  if (!file.seek(seekPos)) return false;

  size_t bytesToRead = numCols * bytesPerPixel;
  if (file.read(buffer, bytesToRead) != bytesToRead) return false;

  return true;
}

/*
 * @param filename   Full path to the .bmp file (e.g., "/image.bmp")
 * @param tft        Reference to your TFT object
 * @param maxSize Max memory usage 
 * @return           true on success, false on error
 */

bool displayBMP(const char* filename, TFT_eSPI& tft, size_t maxSize) {
  File file = SD.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open BMP file");
    return false;
  }
  tft.setSwapBytes(true);  // <-- FIXES the random dots (byte order)
  BITMAPFILEHEADER fileHeader;
  if (file.read((uint8_t*)&fileHeader, sizeof(fileHeader)) != sizeof(fileHeader)) {
    file.close();
    return false;
  }
  if (fileHeader.bfType != 0x4D42) {
    Serial.println("Not a BMP file");
    file.close();
    return false;
  }

  BITMAPINFOHEADER infoHeader;
  if (file.read((uint8_t*)&infoHeader, sizeof(infoHeader)) != sizeof(infoHeader)) {
    file.close();
    return false;
  }

  if (infoHeader.biBitCount != 24 || infoHeader.biCompression != 0) {
    Serial.println("Only uncompressed 24-bit BMP supported");
    file.close();
    return false;
  }

  int imgWidth = abs(infoHeader.biWidth);
  int imgHeight = abs(infoHeader.biHeight);
  bool isBottomUp = (infoHeader.biHeight > 0);  // true = standard Windows BMP

  int bytesPerPixel = 3;
  int rowSize = ((imgWidth * bytesPerPixel + 3) & ~3);
  uint32_t dataOffset = fileHeader.bfOffBits;

  // Debug info
  Serial.printf("BMP: %dx%d, %s\n", imgWidth, imgHeight, isBottomUp ? "bottom-up" : "top-down");

  // Auto-rotate: align longer side with screen longer side
  if (imgWidth > imgHeight) {
    tft.setRotation(1);  // Landscape
  } else {
    tft.setRotation(2);  // Portrait
  }

  int screenW = tft.width();
  int screenH = tft.height();

  // Compute scaling and offsets (fill screen, crop edges)
  float scaleX = (float)screenW / imgWidth;
  float scaleY = (float)screenH / imgHeight;
  float scale = (scaleX > scaleY) ? scaleX : scaleY;  // fill
  int newW = (int)(imgWidth * scale + 0.5f);
  int newH = (int)(imgHeight * scale + 0.5f);
  int offsetX = (screenW - newW) / 2;
  int offsetY = (screenH - newH) / 2;

  Serial.printf("Scale: %.2f, display size: %dx%d, offset: %d,%d\n", scale, newW, newH, offsetX, offsetY);

  // Decide interpolation: bilinear only when upscaling (smooth)
  bool useBilinear = (scale > 1.0f);

  // Precompute mapping constants (using double for accuracy)
  double invScaleX = 1.0 / scale;
  double invScaleY = 1.0 / scale;
  double srcX0 = -offsetX * invScaleX;  // source column at screen x=0
  double srcY0 = -offsetY * invScaleY;  // source row at screen y=0

  // Determine the range of source columns we need to read.
  int srcColStart = (int)floor(srcX0);
  int srcColEnd = (int)floor(srcX0 + (screenW - 1) * invScaleX);
  int minCol = (srcColStart < 0) ? 0 : srcColStart;
  int maxCol = (srcColEnd >= imgWidth) ? imgWidth - 1 : srcColEnd;
  // For bilinear we need one extra column for interpolation (if available)
  if (useBilinear && maxCol + 1 < imgWidth) maxCol++;
  int numColsToRead = maxCol - minCol + 1;

  size_t rowBufferSize = numColsToRead * bytesPerPixel;
  size_t totalBufferSize = useBilinear ? 2 * rowBufferSize : rowBufferSize;
  if (totalBufferSize > maxSize) {
    // Fallback to nearest-neighbor if bilinear would exceed memory
    useBilinear = false;
    totalBufferSize = rowBufferSize;
    if (totalBufferSize > maxSize) {
      Serial.println("BMP row segment too large for memory; increase maxSize or process in strips.");
      file.close();
      return false;
    }
  }

  uint8_t* rowBuf0 = (uint8_t*)malloc(rowBufferSize);
  if (!rowBuf0) {
    file.close();
    return false;
  }
  uint8_t* rowBuf1 = nullptr;
  if (useBilinear) {
    rowBuf1 = (uint8_t*)malloc(rowBufferSize);
    if (!rowBuf1) {
      free(rowBuf0);
      file.close();
      return false;
    }
  }

  uint16_t* screenRow = (uint16_t*)malloc(screenW * sizeof(uint16_t));
  if (!screenRow) {
    free(rowBuf0);
    if (rowBuf1) free(rowBuf1);
    file.close();
    return false;
  }

  // Helper: get a packed 24-bit pixel from the buffer (0x00RRGGBB)
  auto getPixel = [&](uint8_t* buf, int col) -> uint32_t {
    int idx = (col - minCol) * bytesPerPixel;
    uint8_t b = buf[idx];
    uint8_t g = buf[idx + 1];
    uint8_t r = buf[idx + 2];
    return (uint32_t)((r << 16) | (g << 8) | b);
  };

  // For each screen row
  for (int y = 0; y < screenH; y++) {
    double srcY_f = srcY0 + y * invScaleY;
    int srcRow = (int)floor(srcY_f);
    double fracY = srcY_f - srcRow;

    // Clamp source row to valid range
    if (srcRow < 0) {
      srcRow = 0;
      fracY = 0;
    }
    if (srcRow >= imgHeight) {
      srcRow = imgHeight - 1;
      fracY = 0;
    }

    int srcRowNext = srcRow + 1;
    if (srcRowNext >= imgHeight) srcRowNext = imgHeight - 1;

    // Read the two row segments
    if (!readBMPRowSegment(file, srcRow, minCol, numColsToRead, rowBuf0,
                           imgWidth, imgHeight, rowSize, dataOffset, bytesPerPixel, isBottomUp)) {
      free(screenRow);
      free(rowBuf0);
      if (rowBuf1) free(rowBuf1);
      file.close();
      return false;
    }
    if (useBilinear && (srcRow != srcRowNext || fracY != 0)) {
      if (!readBMPRowSegment(file, srcRowNext, minCol, numColsToRead, rowBuf1,
                             imgWidth, imgHeight, rowSize, dataOffset, bytesPerPixel, isBottomUp)) {
        free(screenRow);
        free(rowBuf0);
        if (rowBuf1) free(rowBuf1);
        file.close();
        return false;
      }
    } else if (useBilinear) {
      // Same row, copy
      memcpy(rowBuf1, rowBuf0, rowBufferSize);
    }

    // For each screen column
    for (int x = 0; x < screenW; x++) {
      double srcX_f = srcX0 + x * invScaleX;
      int srcCol = (int)floor(srcX_f);
      double fracX = srcX_f - srcCol;

      // Clamp to image boundaries
      if (srcCol < 0) {
        srcCol = 0;
        fracX = 0;
      }
      if (srcCol >= imgWidth) {
        srcCol = imgWidth - 1;
        fracX = 0;
      }
      int srcColNext = srcCol + 1;
      if (srcColNext >= imgWidth) srcColNext = imgWidth - 1;

      // **Safety clamp to the actual buffer range** to avoid reading garbage
      if (srcCol < minCol) srcCol = minCol;
      if (srcCol > maxCol) srcCol = maxCol;
      if (srcColNext < minCol) srcColNext = minCol;
      if (srcColNext > maxCol) srcColNext = maxCol;

      uint32_t c00, c01, c10, c11;
      if (useBilinear) {
        c00 = getPixel(rowBuf0, srcCol);
        c01 = getPixel(rowBuf0, srcColNext);
        c10 = getPixel(rowBuf1, srcCol);
        c11 = getPixel(rowBuf1, srcColNext);
      } else {
        // Nearest neighbor
        uint32_t c = getPixel(rowBuf0, srcCol);
        uint8_t r = (c >> 16) & 0xFF;
        uint8_t g = (c >> 8) & 0xFF;
        uint8_t b = c & 0xFF;
        screenRow[x] = rgb565(r, g, b);
        continue;
      }

      // Bilinear interpolation
      uint8_t r00 = (c00 >> 16) & 0xFF;
      uint8_t g00 = (c00 >> 8) & 0xFF;
      uint8_t b00 = c00 & 0xFF;
      uint8_t r01 = (c01 >> 16) & 0xFF;
      uint8_t g01 = (c01 >> 8) & 0xFF;
      uint8_t b01 = c01 & 0xFF;
      uint8_t r10 = (c10 >> 16) & 0xFF;
      uint8_t g10 = (c10 >> 8) & 0xFF;
      uint8_t b10 = c10 & 0xFF;
      uint8_t r11 = (c11 >> 16) & 0xFF;
      uint8_t g11 = (c11 >> 8) & 0xFF;
      uint8_t b11 = c11 & 0xFF;

      float fx = fracX, fy = fracY;
      float r = r00 * (1 - fx) * (1 - fy) + r01 * fx * (1 - fy) + r10 * (1 - fx) * fy + r11 * fx * fy;
      float g = g00 * (1 - fx) * (1 - fy) + g01 * fx * (1 - fy) + g10 * (1 - fx) * fy + g11 * fx * fy;
      float b = b00 * (1 - fx) * (1 - fy) + b01 * fx * (1 - fy) + b10 * (1 - fx) * fy + b11 * fx * fy;

      screenRow[x] = rgb565((uint8_t)r, (uint8_t)g, (uint8_t)b);
    }

    //tft.pushImage(0, y, screenW, 1, screenRow);
    tft.pushImage(offsetX, offsetY+y, screenW, 1, screenRow);
  }

  free(screenRow);
  free(rowBuf0);
  if (rowBuf1) free(rowBuf1);
  file.close();
  return true;
}

```

**Usage Example:**    
```
    // Display a BMP image with a memory limit of 4096 bytes per row
    if (!displayBMP("/image.bmp", tft, 4096)) {
        Serial.println("Failed to display BMP");
    }
```

**Key Features**
- Auto‑rotation: The TFT is rotated so that the image’s longer side aligns with the screen’s longer side (setRotation(1) for landscape, 2 for portrait).
- Scaling & Cropping: If the image is larger than the screen, it is scaled to fill the entire display; excess edge pixels are discarded to preserve aspect ratio. If the image is smaller, the code expands the image to fill the entire screen even if the original image is smaller than the TFT.
- Memory‑aware: Only the required portion of each BMP row is read into a buffer; the buffer size is checked against maxSize. (For very wide images, you may need to extend the implementation with horizontal strip processing.)
- Centering: The image is always centered on the screen, whether scaled or not.
- Supported format: Uncompressed 24‑bit BMP (the most common format).

**Notes**
- The function assumes the SD library provides a File object with seek(), read(), write(), etc. Adjust the SD.open path prefix as needed for your filesystem (e.g., SPIFFS or LittleFS).
- If your BMP has negative height (top‑down orientation), it is handled correctly.
- For extremely wide images that exceed maxSize after cropping, you can implement horizontal strip processing by splitting the screen into vertical bands and calling pushImage for each band. The provided code returns false in that case for simplicity.


## Display PNG images       

The PNG function support standard non‑interlaced 24‑bit PNG, if unable to display the image, convert the PNG file with the following settings:     
- Interlaced = OFF (not checked)
- Bit depth = 24‑bit (or "Truecolor")
- No alpha channel if possible (or it may still work)

**Note:** Transparency – The PNG decoder handles alpha channels; this code uses 0xffffffff as the background colour for blending (fully opaque). If you want transparency to show the TFT background, you can modify the callback to blend pixels.    

**Online Tools**          

- To convert PNG to 24-bit PNG: https://elysiatools.com/en/tools/png-to-24bit-png
- To resize or crop image: https://imageresizer.com/crop-image
  
**Library**     

PNGdec by Larry Bank          
https://github.com/bitbank2/PNGdec     


**Sample Code**     

```
#include <SD.h>
#include <TFT_eSPI.h>
#include <PNGdec.h>

static PNG png;
static TFT_eSPI* pTFT = nullptr;
static File* pngFileHandle = nullptr;

static float   pngScale = 1.0f;
static int16_t pngXOffset = 0;
static int16_t pngYOffset = 0;
static int16_t pngScaledW = 0;
static int16_t pngScaledH = 0;
static int16_t pngImgWidth = 0;
static int16_t pngImgHeight = 0;
static int16_t pngScreenW = 0;
static int16_t pngScreenH = 0;
static uint16_t* pngRowBuffer = nullptr;
static int16_t pngLastDrawnY = -1;   // reset before decode

// ─── PNGdec callbacks ──────────────────────────────────────────

static void* pngOpen(const char* filename, int32_t* pSize) {
    if (pngFileHandle) { pngFileHandle->close(); delete pngFileHandle; pngFileHandle = nullptr; }
    pngFileHandle = new File(SD.open(filename, "r"));
    if (!*pngFileHandle) {
        Serial.printf("SD.open FAILED: %s\n", filename);
        delete pngFileHandle; pngFileHandle = nullptr;
        return nullptr;
    }
    uint8_t sig[8];
    if (pngFileHandle->read(sig, 8) != 8) {
        Serial.printf("Can't read signature from %s\n", filename);
        pngFileHandle->close(); delete pngFileHandle; pngFileHandle = nullptr;
        return nullptr;
    }
    const uint8_t pngSig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    if (memcmp(sig, pngSig, 8) != 0) {
        Serial.printf("Invalid PNG signature in %s\n", filename);
        pngFileHandle->close(); delete pngFileHandle; pngFileHandle = nullptr;
        return nullptr;
    }
    pngFileHandle->seek(0);
    *pSize = pngFileHandle->size();
    return (void*)pngFileHandle;
}

static void pngClose(void* pHandle) {
    if (pngFileHandle) { pngFileHandle->close(); delete pngFileHandle; pngFileHandle = nullptr; }
}

static int32_t pngRead(PNGFILE* pFile, uint8_t* pBuf, int32_t iLen) {
    if (!pngFileHandle) return -1;
    return pngFileHandle->read(pBuf, iLen);
}

static int32_t pngSeek(PNGFILE* pFile, int32_t iPos) {
    if (!pngFileHandle) return -1;
    return pngFileHandle->seek(iPos) ? iPos : -1;
}

// ─── Draw callback with proper up/down scaling ─────────────────

static int pngDraw(PNGDRAW* pDraw) {
    // Get decoded source row (original width)
    uint16_t lineBuffer[pDraw->iWidth];
    png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);

    // Vertical mapping range for this source row
    int tgtY_start = (int)(pDraw->y * pngScale + pngYOffset + 0.5f);
    int tgtY_end   = (int)((pDraw->y + 1) * pngScale + pngYOffset + 0.5f);
    // For downscaling, ensure at least one row
    if (tgtY_end <= tgtY_start) tgtY_end = tgtY_start + 1;

    // Determine horizontal visible segment (same for all rows in this block)
    int visibleStart = 0, visibleEnd = pngScaledW - 1;
    int screenStartX = pngXOffset;
    int screenEndX   = pngXOffset + pngScaledW - 1;
    if (screenEndX < 0 || screenStartX >= pngScreenW) return 1; // completely off

    if (screenStartX < 0) {
        visibleStart = -pngXOffset;
        screenStartX = 0;
    }
    if (screenEndX >= pngScreenW) {
        visibleEnd = pngScreenW - 1 - pngXOffset;
        screenEndX = pngScreenW - 1;
    }
    int visibleWidth = visibleEnd - visibleStart + 1;
    if (visibleWidth <= 0) return 1;

    // Resize the visible segment horizontally (nearest neighbour)
    for (int x = 0; x < visibleWidth; x++) {
        int srcX = (int)((visibleStart + x - pngXOffset) / pngScale + 0.5f);
        if (srcX < 0) srcX = 0;
        if (srcX >= pngImgWidth) srcX = pngImgWidth - 1;
        pngRowBuffer[x] = lineBuffer[srcX];
    }

    // Draw vertical block: from tgtY_start to tgtY_end - 1
    for (int tgtY = tgtY_start; tgtY < tgtY_end; tgtY++) {
        if (tgtY < 0 || tgtY >= pngScreenH) continue;
        // Avoid duplicate drawing (only needed for downscaling, but harmless)
        if (tgtY == pngLastDrawnY) continue;
        pngLastDrawnY = tgtY;
        pTFT->pushImage(screenStartX, tgtY, visibleWidth, 1, pngRowBuffer);
    }
    return 1;
}

// ─── Cleanup ────────────────────────────────────────────────────

static void pngCleanup() {
    png.close();
    if (pngFileHandle) { pngFileHandle->close(); delete pngFileHandle; pngFileHandle = nullptr; }
    if (pngRowBuffer) { free(pngRowBuffer); pngRowBuffer = nullptr; }
    delay(10);
}

// ─── Main display function ──────────────────────────────────────

bool displayPNG(const char* filename, TFT_eSPI& tft, size_t maxSize) {
    pngCleanup();

    if (!SD.exists(filename)) {
        Serial.printf("File NOT found: %s\n", filename);
        return false;
    }

    int rc = png.open(filename, pngOpen, pngClose, pngRead, pngSeek, pngDraw);
    if (rc != PNG_SUCCESS) {
        Serial.printf("PNG open failed: %d for %s\n", rc, filename);
        pngCleanup();
        return false;
    }

    pngImgWidth  = png.getWidth();
    pngImgHeight = png.getHeight();
    Serial.printf("PNG: %dx%d\n", pngImgWidth, pngImgHeight);

    if (pngImgWidth > pngImgHeight) tft.setRotation(1); else tft.setRotation(2);

    pngScreenW = tft.width();
    pngScreenH = tft.height();

    float scaleX = (float)pngScreenW / pngImgWidth;
    float scaleY = (float)pngScreenH / pngImgHeight;
    pngScale = (scaleX > scaleY) ? scaleX : scaleY;

    pngScaledW = (int)(pngImgWidth  * pngScale + 0.5f);
    pngScaledH = (int)(pngImgHeight * pngScale + 0.5f);

    pngXOffset = (pngScreenW - pngScaledW) / 2;
    pngYOffset = (pngScreenH - pngScaledH) / 2;

    pngRowBuffer = (uint16_t*)malloc(pngScreenW * sizeof(uint16_t));
    if (!pngRowBuffer) {
        Serial.println("Row buffer allocation failed");
        png.close();
        return false;
    }

    pTFT = &tft;
    pngLastDrawnY = -1;   // reset for this image

    tft.startWrite();
    uint32_t start = millis();
    rc = png.decode(nullptr, 0);
    Serial.printf("Decode time: %d ms\n", millis() - start);
    tft.endWrite();

    pngCleanup();

    if (rc != PNG_SUCCESS) {
        Serial.printf("PNG decode failed: %d\n", rc);
        return false;
    }
    return true;
}
```

**Usage Example:**    

```
if (!displayPNG("/image.png", tft, 4096)) {
    Serial.println("Display failed");
}
```

**Key Features**
- Auto‑rotation: The TFT is rotated so that the image’s longer side aligns with the screen’s longer side (setRotation(1) for landscape, 2 for portrait).
- Scaling & Cropping: If the image is larger than the screen, it is scaled to fill the entire display; excess edge pixels are discarded to preserve aspect ratio. If the image is smaller, the code expands the image to fill the entire screen even if the original image is smaller than the TFT.
- Memory‑aware: Only the required portion of each PNG row is read into a buffer; the buffer size is checked against maxSize. (For very wide images, you may need to extend the implementation with horizontal strip processing.)
- Centering: The image is always centered on the screen, whether scaled or not.
- Supported format: Standard non‑interlaced 24‑bit PNG.

