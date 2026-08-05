# SD Card ILI9488

## Initialize the SD Card to use separate SPI bus from TFT Display

Below is the general setup to initialize both SD card and ILI9488 TFT Display at separate SPI buses:
- SD Card -> VSPI
- TFT Display -> HSPI (defined in User_setup.h)

### Wiring Diagram - separate SPI

<img width="1283" height="683" alt="image" src="https://github.com/user-attachments/assets/9295b346-a905-4f83-9c77-06c00559bef4" />

Here is the general code setup:

```cpp
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

## Initialize the SD Card to use same SPI bus with TFT Display

On the ESP32‑S3, two different SPIClass objects cannot manage the same hardware SPI peripheral concurrently – this leads to a lock‑up when the TFT library tries to take control. To avoid ESP32-S3 hangs during SD card initialization when sharing the SPI bus with an ILI9488 TFT display:
```User_Setup.h``` is configured to use HSPI (USE_HSPI_PORT) or VSPI (USE_FSPI_PORT). The TFT_eSPI library will initialise that bus automatically inside ```tft.init()```. You must not create any separate SPIClass object for the SD card. Instead:
1. Get the SPI bus instance from the TFT library using ```tft.getSPIinstance()```.
2. Use that instance when initializing the SD card with ```SD.begin()```.
3. Pull the CS pins of all share SPI devices HIGH at the very top of your ```setup()``` function before doing anything else to avoid unassigned state.

### Wiring Diagram - share SPI

<img width="1236" height="706" alt="image" src="https://github.com/user-attachments/assets/57c1138f-699c-4697-a05b-56a6171a618c" />

Here is the general code example:

```cpp
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

**Note:**    

Sometime having problem with SD Card initialization on first power up after code download, you may implement a retry code.

```cpp
  if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {
    Serial.println("SD Card initialization failed! Try again...");
    delay(1000);
    if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {
      Serial.println("SD Card initialization failed!");
      tft.println("SD Card init failed!");
      while (1) delay(1000);
    }
  }
```

## WARNING: Sharing SPI bus expose the risk of SD card corruption

SD card corruption after running ESP32 code strongly suggests a hardware or software conflict – especially since you are sharing the SPI bus between the TFT and the SD card. This is a common cause of file system corruption.

### 1. Missing SPI transaction management – Critical!

When two devices share the same SPI bus, you must use ```SPI.beginTransaction()``` / ```SPI.endTransaction()``` to switch between them. Without this, the TFT and SD card may try to talk at the same time, causing garbage data and corrupting the SD card’s file system.

What your code should do:

```cpp
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

### 2. TFT library may not release the SPI bus

TFT_eSPI uses SPI transactions by default, but if you call SD functions without endTransaction(), the TFT might still hold the bus. Always ensure the TFT is not selected before using the SD card.

### 3. Power brown‑out – ESP32 + TFT + SD can draw >300mA peak

If your USB power is weak, a sudden current drop can cause the SD card to reset or write garbage. Even reads can corrupt the card if power glitches during SPI.

### 4. SD card not closed properly before reset

If your ESP32 resets (watchdog, crash, or power loss) while the SD card is still selected (CS low) or a transaction is open, the card’s internal state may be corrupted.

### How to fix – Step by step

#### Step 1: Ensure proper SPI transaction around SD access

Wrap every SD card operation (including ```SD.begin()```) with transactions:

```cpp
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

#### Step 2: In your file access function, wrap each file operation

The ```SD.open``` and ```file.read``` already use transactions internally if the SD library is compiled with ```USE_SPI_TRANSACTIONS```. But to be safe, you can add explicit transactions around the file open/read loop.

However, the simplest fix is to ensure that no SPI device is selected while the other is active. TFT_eSPI automatically de‑selects the TFT after each drawing command. But if you call ```tft.pushImage``` repeatedly, the TFT stays selected? No – ```pushImage``` ends with de‑select. So that’s likely fine.

#### Step 3: Lower SD card SPI speed

Many SD cards are unstable at 40 MHz. Force a lower speed:

```cpp
SD.begin(SD_CS, SPI, 4000000);  // 4 MHz
```

Add this after your transaction block.

#### Step 4: Add delays and flush after each file access

Example, after displaying an image, add a small delay to let the SD card finish any internal cleanup:

```cpp
displayBinary656(...);
delay(50);
```

Also, after closing the file, you can force a flush of the SD library cache (if any) by calling ```SD.end()``` before power down – but that’s only if you are about to sleep or reset.

#### Step 5: Use SdFat library

It allows you to explicitly pass a pointer to the SPI instance. When initializing your SD card, define the config like this:

```cpp
// Assuming you have a TFT_eSPI object named 'tft'
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(16), tft.getSPIinstance())
```

This uses the ```getSPIinstance()``` method to fetch a pointer to the exact SPI object TFT_eSPI is using, guaranteeing they will cooperate correctly.  
Library ```SdFat.h``` by Bill Greiman  
https://github.com/greiman/SdFat  

Check here for detail about [SdFat](https://github.com/ooikk/Arduino-Documentation/blob/main/07_SDCard_Display/SDCard_SdFat.md)

#### Step 6: Check for accidental writes

Ensure you never open the file with ```FILE_WRITE```. Use ```FILE_READ``` only.

```cpp
File file = SD.open(filename, FILE_READ);
```

OR Use hardware "Lock" your SD Card.

### Advanced: Use separate SPI buses (recommended for stability)

ESP32 has two hardware SPI buses: VSPI (default) and HSPI. You can put the TFT on one and the SD card on the other.

## Recover Corrupted SD Card

If you have accidentally corrupted or damaged your SD Card, follow this link to try restore back your SD Card [SD Card Recovery](SDCard%20Corruption.md)

## Combined Test Sketch (Display + Touch + SD) with separate SPI bus

This sketch initializes the display, checks if the SD card is present, and prints a basic touch prompt.  
***Note: You will need a microSD card formatted as FAT32 inserted into the module for the SD test to pass.***

```cpp
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

| Category | Function | Example | Description |
| --- | --- | --- | --- |
| Initialization | ```begin(csPin)``` | ```if (!SD.begin(4)) { Serial.println("Init failed!"); }``` | Initializes the SD card library. Must be called first, before any other SD card functions. Returns true on success. |
| File/Directory Info | ```exists(path)``` | ```if (SD.exists("/data.txt")) { Serial.println("File found"); }``` | Checks if a file or directory exists on the SD card. |
| File/Directory Info | ```open(path, mode)``` | ```File dataFile = SD.open("/data.txt", FILE_WRITE);``` | Opens a file for reading/writing. Must be called before you can read (```FILE_READ```) or write (```FILE_WRITE```) to a file. Returns a File object or false on failure. |
| File/Directory Info | ```remove(path)``` | ```f (SD.exists("/unwanted.txt")) { SD.remove("/unwanted.txt"); }``` | Deletes a file from the SD card. It's recommended to check if the file exists first using ```SD.exists()```. |
| File/Directory Info | ```mkdir(path)``` | ```SD.mkdir("/newFolder/subFolder");``` | Creates a directory. Can also create intermediate directories if they don't exist. |
| File/Directory Info | ```rmdir(path)``` | ```SD.rmdir("/emptyFolder");``` | Removes an empty directory. The directory must be empty before calling this. |
| Control | ```close()``` | ```dataFile.close();``` | Closes a file and ensures any data written to it is saved to the SD card. It's good practice to close files when you're done with them. |
| Control | ```flush()``` | ```dataFile.flush();``` | Forces any buffered data to be written to the SD card. This is done automatically when you close the file. |
| State Information | ```available()``` | ```while (dataFile.available()) { char c = dataFile.read(); }``` | Checks if there are any bytes available to be read from the file. |
| State Information | ```position()``` | ```uint32_t pos = dataFile.position();``` | Returns the current position (in bytes) within a file, where the next read or write will happen. |
| State Information | ```seek(pos)``` | ```dataFile.seek(0); // Jump to the start of the file```| Moves the read/write position to a specific byte in a file. |
| State Information | ```size()``` | ```uint32_t fileSize = dataFile.size();``` | Returns the total size of a file in bytes. |
| State Information | ```isDirectory()``` | ```if (root.isDirectory()) { Serial.println("It's a directory!"); }``` | Checks if the current File object represents a directory (folder). |
| Reading & Writing | ```read()``` | ```char ch = dataFile.read();``` | Reads a single byte from a file. Returns -1 if no bytes are available. |
| Reading & Writing | ```write(data)``` | ```dataFile.write("Hello, world!");``` | Writes a single byte or a buffer of data to a file. |
| Reading & Writing | ```print(data)``` | ```dataFile.print("Sensor Value: "); dataFile.print(sensorValue);``` | Writes text to a file. Numbers are automatically converted to their ASCII string representation. |
| Reading & Writing | ```println(data)``` | ```dataFile.println("--- End of Data ---");``` | Like ```print()```, but adds a newline (```\r\n```) at the end. |
| Reading & Writing | ```peek()``` | ```char nextChar = dataFile.peek();``` | Reads a byte from a file without moving the read pointer, so a subsequent ```read()``` will return the same byte. |
| Directory Navigation | ```openNextFile()``` | ```File entry = dir.openNextFile();``` | Opens the next file or folder in a directory. Used in a loop to list all contents of a directory. |
| Directory Navigation | ```rewindDirectory()``` | ```dir.rewindDirectory();``` | Resets the file pointer back to the first file in a directory, allowing you to iterate over it again. |
| Information | ```name()``` | ```Serial.println(entry.name());``` | Returns the name of a file or directory as a character array (string). |

Refer to SD library for more details:  
https://docs.arduino.cc/libraries/sd/

### write() and print() functions

In the context of Arduino and the ```SD.h``` library (and serial communication), the difference between ```write()``` and ```print()``` comes down to Raw Binary Data vs. Human-Readable Text.

1. ```write()``` (Raw Bytes)
```write()``` takes the exact binary representation of the data and dumps it directly into the file. It does not format or convert the data into text.
- Best for: Binary files, saving memory, writing arrays of bytes, sensor data, images, or audio.
- How it works: If you tell it to write the number 255, it writes exactly 1 byte (0xFF in hex). If you write an integer 1000, it writes 2 bytes (or 4, depending on the board).
- Syntax:
  - ```file.write(myByte);``` (Writes a single byte)
  - ```file.write(myArray, arrayLength);``` (Writes an array of bytes)

2. ```print()``` (Formatted Text / ASCII)
```print()``` takes your data, converts it into human-readable ASCII characters (text), and then writes those characters to the file.
- Best for: Text files, CSV logs, configuration files, data that needs to be read by a human or imported into Excel.
- How it works: If you tell it to print the number 255, it converts it to the text characters '2', '5', and '5'. This takes 3 bytes of space, not 1. If you print 1000, it takes 4 bytes.
- Syntax:
  - ```file.print(255);``` (Writes the text "255")
  - ```file.print(255, HEX);``` (Writes the text "FF")
  - ```file.print(3.14159, 2);``` (Writes the text "3.14" - limits to 2 decimal places)

Visual Comparison:  
Imagine you have an integer variable: ```int myValue = 12345;```

| Method | Code | What is actually saved in the file | File Size Increase | How it looks if you open it in Notepad |
| --- | --- | --- | --- | --- |
| ```write()``` | ```file.write((uint8_t*)&myValue, 2);``` | 0x39 0x30 (Raw binary) | 2 Bytes | Gibberish symbols (e.g., 90) |
| ```print()``` | ```file.print(myValue);``` | 0x31 0x32 0x33 0x34 0x35 (ASCII for "12345") | 5 Bytes | 12345 |

## Sample code to access the SD Card

### Get SD Card information

```cpp
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

### Writing a text file to SD Card

Below is a simple test code to write a text to SD Card.

```cpp
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

### Reading a text file from SD Card

```cpp
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

### More Examples

#### Write to a file

```cpp
/**
 * Writes an ASCII/Text file to the SD card.
 * 
 * @param filename The name/path of the file (e.g., "data.txt")
 * @param textData The text data to write (passed by reference to save memory)
 * @param append   If true, adds data to the end. If false, overwrites the file.
 * @return true if successful, false otherwise.
 */
bool writeTextFile(const String& filename, const String& textData, bool append) {

/*  
  if (!SD.begin(SD_CS_PIN)) {  // Replace 4 with your CS pin
    Serial.println("Error: SD card not initialized.");
    return false;
  }
  */

  // If overwrite is requested, delete the existing file first
  if (!append) {
    if (SD.exists(filename)) {
      if (!SD.remove(filename)) {
        Serial.println("Error: Failed to delete existing file.");
        return false;
      }
    }
  }

  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Error: Could not create/open text file.");
    return false;
  }

  size_t written = file.print(textData);
  file.close();

  if (written != textData.length()) {
    Serial.println("Error: Partial write detected. Card may be full.");
    return false;
  }

  return true;
}

/**
 * Writes a Binary file to the SD card.
 * 
 * @param filename The name/path of the file (e.g., "data.bin")
 * @param binData  Pointer to the byte array containing binary data
 * @param dataSize The number of bytes to write from the array
 * @param append   If true, adds data to the end. If false, overwrites the file.
 * @return true if successful, false otherwise.
 */
bool writeBinFile(const String& filename, const uint8_t* binData, size_t dataSize, bool append) {
/*
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Error: SD card not initialized.");
    return false;
  }
  */
  // If overwrite is requested, delete the existing file first
  if (!append) {
    if (SD.exists(filename)) {
      if (!SD.remove(filename)) {
        Serial.println("Error: Failed to delete existing file.");
        return false;
      }
    }
  }

  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Error: Could not create/open binary file.");
    return false;
  }

  size_t written = file.write(binData, dataSize);
  file.close();

  if (written != dataSize) {
    Serial.println("Error: Partial write detected. Card may be full.");
    return false;
  }

  return true;
}

  // ==========================================
  // 1. Testing Text File (Write, Append, Overwrite)
  // ==========================================
  Serial.println("\n--- Testing Text File ---");
  
  // Create new file and write first line
  writeTextFile("/log.txt", "Line 1: Hello World!\n", false);
  Serial.println("Wrote initial text.");

  // Append to the file
  writeTextFile("/log.txt", "Line 2: Appended data.\n", true);
  Serial.println("Appended text.");

  // Overwrite the file (This will erase Line 1 and Line 2)
  writeTextFile("/log.txt", "Line 1: File was overwritten!\n", false);
  Serial.println("Overwrote text file.");

  // ==========================================
  // 2. Testing Binary File (Write and Append)
  // ==========================================
  Serial.println("\n--- Testing Binary File ---");
  
  // Create some dummy binary data
  uint8_t dataBlock1[] = {0xDE, 0xAD, 0xBE, 0xEF};
  uint8_t dataBlock2[] = {0xCA, 0xFE, 0xBA, 0xBE};

  // Write first block (overwrite mode, creates new file)
  if (writeBinFile("/sensor.bin", dataBlock1, sizeof(dataBlock1), false)) {
    Serial.println("Wrote initial binary block.");
  }

  // Append second block
  if (writeBinFile("/sensor.bin", dataBlock2, sizeof(dataBlock2), true)) {
    Serial.println("Appended second binary block.");
  }
  
  // Overwrite the binary file with just the second block
  if (writeBinFile("/sensor.bin", dataBlock2, sizeof(dataBlock2), false)) {
    Serial.println("Overwrote binary file.");
  }
```

⚠️ Important Note on Overwriting in Arduino:  
In the standard Arduino ```SD.h``` library, the ```FILE_WRITE``` mode is hardcoded to append data. To overwrite a file, the most reliable method is to delete the existing file first using ```SD.remove()``` before opening it in write mode.

Key Design Choices:  
1. Return Type (```bool```): File operations can fail (e.g., SD card full, bad sector, write error). Returning a boolean allows your main code to check if the write was actually successful.
2. Pass by Reference (```const String&```): For the text function, passing the string by const reference prevents the Arduino from creating an unnecessary copy of the string in memory, which is crucial for preventing memory fragmentation on low-RAM boards.
3. Verification: The functions check the return value of ```file.print()``` and ```file.write()```. If the SD card becomes full halfway through writing, the function will catch it and return false.

#### Read from a file

```cpp
/**
 * Reads an ASCII/Text file from the SD card.
 * 
 * @param filename The name/path of the file (e.g., "data.txt")
 * @return A String containing the text data.
 */
String readTextFile(String filename) {
  String content = "";

  // Open the file for reading
  File file = SD.open(filename, FILE_READ);

  if (file) {
    // Read the file character by character
    while (file.available()) {
      content += (char)file.read();
    }
    file.close();
  } else {
    Serial.print("Error: Could not open text file ");
    Serial.println(filename);
  }

  return content;
}

/**
 * Reads a Binary file from the SD card.
 * 
 * @param filename The name/path of the file (e.g., "data.bin")
 * @param outSize Reference variable to store the size of the returned array.
 * @return A dynamically allocated uint8_t array. 
 *         IMPORTANT: You must call free() on this array when done to prevent memory leaks!
 */
uint8_t* readBinFile(String filename, size_t& outSize) {
  outSize = 0;

  // Open the file for reading
  File file = SD.open(filename, FILE_READ);

  if (file) {
    // Get the total size of the file
    size_t fileSize = file.size();

    // Dynamically allocate memory for the binary data
    uint8_t* buffer = (uint8_t*)malloc(fileSize);

    if (buffer != NULL) {
      // Read the entire file into the buffer
      size_t bytesRead = file.read(buffer, fileSize);

      if (bytesRead == fileSize) {
        outSize = fileSize;  // Update the size reference
      } else {
        Serial.println("Error: Could not read the entire binary file.");
        free(buffer);
        buffer = NULL;
      }
    } else {
      Serial.println("Error: Memory allocation failed for binary file.");
    }

    file.close();
    return buffer;

  } else {
    Serial.print("Error: Could not open binary file ");
    Serial.println(filename);
    return NULL;
  }
}


// --- 1. Test Reading Text File ---
  Serial.println("\n--- Reading Text File ---");
  String textData = readTextFile("/example.txt");
  
  if (textData.length() > 0) {
    Serial.println("Text Content:");
    Serial.println(textData);
  }

  // --- 2. Test Reading Binary File ---
  Serial.println("\n--- Reading Binary File ---");
  size_t binSize = 0;
  uint8_t* binData = readBinFile("/example.bin", binSize);
  
  if (binData != NULL && binSize > 0) {
    Serial.print("Binary file size: ");
    Serial.print(binSize);
    Serial.println(" bytes");
    
    // Print the first 10 bytes as an example
    Serial.print("First bytes: ");
    for (size_t i = 0; i < min((size_t)10, binSize); i++) {
      Serial.print(binData[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    // CRITICAL: Free the dynamically allocated memory!
    free(binData); 
    binData = NULL;
  }
```

### ⚠️ Important Memory Notes for Arduino:    
1. Binary File Memory Management: Because ```readBinFile``` uses ```malloc()``` to create the array, the memory is allocated on the heap. You must call ```free(binData)``` when you are completely done using the array, otherwise, your Arduino will run out of memory (Memory Leak).  
2. Text File Memory Fragmentation: The ```String``` class in Arduino can cause memory fragmentation if used heavily or with very large files. If you are reading a massive text file on a memory-constrained board (like an Arduino Uno), it is better to process the text line-by-line inside the while(```file.available()```) loop rather than appending it all to a single ```String```.
3. File Paths: Ensure your filenames include the extension (e.g., "log.txt", "sensor_data.bin"). If your files are inside a folder, use the full path (e.g., "folder/data.txt"). Note that the standard Arduino SD.h library requires folder names to be 8.3 format (max 8 characters for the name, max 3 for the extension).  

## Display RGB565 Image

Below is the 565RGB text file structure modified from generated online by [565RGB Convertor](https://mischianti.org/rgb-image-to-byte-array-converter-for-arduino-tft-displays/). The code in **07_SDCard_Display_565.ino** will load this file and send the pixel color to ```pushImage()``` function. The format is:  
  *Height,Width,{16-bit_pixels,16-bit_pixels....};*  
Picture height is 320 and width is 480. Use ".565" as the image file extention.

```
480,320,{0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
:
0x736d, 0x4a48
};
```

There could be memory limitaton to display 320×480 image (153,600 pixels → 307,200 bytes), ESP32 may still run out of RAM and enter continue reboot cycles.  
getFreeHeap command showed ESP32-S3 has about 320kB of RAM just before image display function start.  
- Free heap before open: 320020  

```cpp
Serial.printf("Free heap before open: %u\n", ESP.getFreeHeap());
```

There are a few methods to over come the memory limitation.

### 1. Process the file in chunks – avoid storing the whole array

If you only need the pixel data for streaming (e.g., to a display), you can parse the text file incrementally without storing all pixels at once. For example, read one line of hex values at a time and send them to the display or process them on the fly.  
Example in ```display565FileDirect()``` or ```displayBinary565()``` in sketch **07_SDCard_Display_565.ino**.  

### 2. Use a raw binary format instead of text

Text hex representation takes ~5 characters per pixel (e.g., 0xABCD,), so the file size is ~5× larger than binary. Parsing it also requires mor
