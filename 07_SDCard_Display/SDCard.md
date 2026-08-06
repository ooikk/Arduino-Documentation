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

### ✅ How to fix – Step by step

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
480,320,{0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
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

Text hex representation takes ~5 characters per pixel (e.g., 0xABCD,), so the file size is ~5× larger than binary. Parsing it also requires more temporary memory. An 320×480 image in ASCII 565RGB take about 1.17MB of file size, the file size reduced to 300kB in binary format. 

  

### Convert images to raw binary format offline with Python script:     

Use little‑endian storage – it matches ESP32’s native byte order, giving the fastest possible read performance. Then enable byte swapping on the TFT:    
```
[2 bytes: height little-endian]
[2 bytes: width  little-endian]
[height × width × 2 bytes: pixel data (16‑bit RGB565, little‑endian)]
```
### Store in little‑endian (recommended)     
- File contents: [height_L, height_H, width_L, width_H, pixel0_L, pixel0_H, pixel1_L, pixel1_H, ...] (low byte first, high byte second)
- Reader code: Directly read into ```uint16_t``` variables. Because ESP32 is little‑endian, the in‑memory value will be correct (e.g., bytes 0x15, 0x6C become 0x6C15).
- TFT display: Call ```tft.setSwapBytes(true);``` once. The library will reverse the byte order when sending to the display (converting to big‑endian). This is very fast (done in DMA or SPI transfer).     

**Advantages:**     
- No conversion needed when reading from SD.
- Uses the native endianness of the ESP32 – fastest.
- ```tft.setSwapBytes(true)``` is a single instruction.

Below is the Python script to convert ASCII RGB565 images file to binary RGB565 file. 

*Refer to **1.8 TFT SPI Display** section for online tool to convert images to RGB565 ASCII format.*     
- [1.8" TFT Display](https://github.com/ooikk/Arduino-Documentation/blob/main/05_1.8_TFT_SPI_Display/1.8_TFT_Display.md)

```python
#!/usr/bin/env python3
"""
Convert a C header with PROGMEM array (e.g. clockhand.h) to a binary .b565 file.
Usage:
    python 565_to_bin.py input.h output.b565

The input file must contain:
    #define SOMETHING_WIDTH  <number>
    #define SOMETHING_HEIGHT <number>
and a 'static const uint16_t name[] PROGMEM = { ... }' with hex values.
The output is a raw binary file with:
    height (uint16_t, little‑endian),
    width  (uint16_t, little‑endian),
    pixel data (uint16_t, little‑endian) in row‑major order.
"""

import sys
import re
import struct

def parse_defines(text):
    """Extract width/height from any #define ending with WIDTH/HEIGHT."""
    width = height = None
    w_name = h_name = None
    for match in re.finditer(r'#define\s+(\w+)\s+(\d+)', text):
        name, val = match.group(1), int(match.group(2))
        if name.upper().endswith('WIDTH') and width is None:
            width, w_name = val, name
        elif name.upper().endswith('HEIGHT') and height is None:
            height, h_name = val, name
        if width is not None and height is not None:
            break
    return width, height, w_name, h_name

def parse_hex_numbers(text):
    """Extract all 0xXXXX tokens and return list of ints."""
    return [int(m, 16) for m in re.findall(r'0x[0-9A-Fa-f]{1,4}', text)]

def main():
    if len(sys.argv) != 3:
        print("Usage: python 565_to_bin.py input.h output.b565")
        sys.exit(1)

    input_file, output_file = sys.argv[1], sys.argv[2]

    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()

    # 1. Extract dimensions
    width, height, w_name, h_name = parse_defines(content)
    if width is None or height is None:
        print("Error: Could not find WIDTH/HEIGHT defines in input.", file=sys.stderr)
        sys.exit(1)
    print(f"Detected dimensions: {width}x{height} (from {w_name}, {h_name})", file=sys.stderr)

    # 2. Extract pixel data
    pixels = parse_hex_numbers(content)
    expected = width * height
    if len(pixels) != expected:
        print(f"Warning: Found {len(pixels)} pixels, expected {expected}.", file=sys.stderr)
        if len(pixels) < expected:
            print("Padding missing pixels with 0x0000", file=sys.stderr)
            pixels += [0] * (expected - len(pixels))
        else:
            print("Truncating to expected count", file=sys.stderr)
            pixels = pixels[:expected]

    # 3. Write binary file
    with open(output_file, 'wb') as out:
        # height, width as little-endian uint16_t
        out.write(struct.pack('<HH', height, width))
        # pixel data as little-endian uint16_t
        for p in pixels:
            out.write(struct.pack('<H', p))

    print(f"Converted {input_file} -> {output_file}: {height}x{width}, {len(pixels)} pixels", file=sys.stderr)

if __name__ == "__main__":
    main()
```

### Other Pythons Tools for 565RGB     
1. Convert background color, use text editor to replace background color to any color of your choice.       
   Example search 0xffff (white) and replace with 0xf800 (RED).      
2. Replace pixels color to any color, change the desire color in the script: ```replace_color.py```. Usage:        
   - ```REPLACE_MATCH = 1``` → replaces matching pixels (equal to ```SEARCH_COLOR```) with ```REPLACE_COLOR```.
   - ```REPLACE_MATCH = 0``` → replaces non‑matching pixels (not equal to ```SEARCH_COLOR```) with ```REPLACE_COLOR```.
   - Under Windows PowerShell, go to 565RGB file directory and type:       
     ```python replace_color.py input-black.h output-red.h```       
3. Convert 565RGB image edge color, use this script: ```draw_edge_line.py```, it uses background color as baseline to check for edge. You can change the ```BG_COLOR``` and ```EDGE_COLOR``` in the script. Usage under Windows PowerShell:       
   ```python draw_edge_line.py input-black.h output-red.h```        
4. Scaled the image to any size, ```scale_image.py```. Usage (For scale factor of 0.5):          
   ```python scale_image.py 0.5 clockhand.h scaled_clockhand.h```      

**File Format**: File has to use the following format as generated from [Online Convertor](https://mischianti.org/rgb-image-to-byte-array-converter-for-arduino-tft-displays/)      

   ```h
   #define CLOCKHAND_WIDTH 28
   #define CLOCKHAND_HEIGHT 185
   // array size is 10360
   static const uint16_t clockhand[] PROGMEM = {
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
   0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
   :
   0x0000, 0x0000
   };
   ```     

## Display JPEG images    

### Library     
TJpg_Decoder by Bodmer     
https://github.com/Bodmer/TJpg_Decoder

### Sample Code    
```cpp
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


### Key Features Explained     
|Feature | How It's Implemented|
|--- | ---|
|Auto‑rotation	| Rotate ( 1 or 2) the image to match the orientations (landscape vs. portrait).|
|Scaling	| The function selects the smallest power‑of‑two scale (1, 2, 4, or 8) that either covers the screen (cropping edges) if possible, or fits entirely (letterbox) if the image is smaller.|
|Memory usage	| ```TJpg_Decoder``` uses a fixed ~3.5KB workspace – the maxSize parameter is kept for compatibility but is not needed.|
|Centering	| The image is always drawn at the centre of the screen (the destX/destY arguments are ignored).|
|Edge cropping	| When covering the screen, the image is cropped symmetrically on the shorter side, preserving the aspect ratio.|
|Callback‑based rendering	| The ```tft_output``` function is called for each MCU block, streaming the image directly to the TFT without a full‑frame buffer.|

### Important Notes  
1. JPEG format – The library only supports 24‑bit JPEGs (not 8‑bit) and does not support progressive JPEGs.
2. SD Card initialisation – Make sure ```SD.begin()``` is called before any ```drawSdJpg()``` calls.
3. Scaling limits – The library only supports scaling factors of 1, 2, 4, or 8. The function automatically picks the best one.
4. Callback clipping – The tft_output callback already clips at screen edges, so negative x/y positions are handled automatically.
5. Restoring rotation – The original screen rotation is restored after drawing, so the rest of your UI remains unaffected.

## Display BMP images       

### Library     

None     

### Sample Code     

Below is a complete implementation of ```displayBMP()``` that reads a **24‑bit BMP** file from an SD card, automatically rotates the TFT to match the image orientation, and scales/crops the image to fill the screen while preserving aspect ratio. The image is centered, and only the necessary portion of the BMP is loaded into memory, respecting the maxSize parameter.    

```cpp
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
  //double srcX0 = -offsetX * invScaleX;  // source column at screen x=0
  //double srcY0 = -offsetY * invScaleY;  // source row at screen y=0
  double srcX0 = 0;  // source column at screen x=0
  double srcY0 = 0;  // source row at screen y=0

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

### Usage Example:    
```cpp
    // Display a BMP image with a memory limit of 4096 bytes per row
    if (!displayBMP("/image.bmp", tft, 4096)) {
        Serial.println("Failed to display BMP");
    }
```

### Key Features
- Auto‑rotation: The TFT is rotated so that the image’s longer side aligns with the screen’s longer side (```setRotation(1)``` for landscape, 2 for portrait).
- Scaling & Cropping: If the image is larger than the screen, it is scaled to fill the entire display; excess edge pixels are discarded to preserve aspect ratio. If the image is smaller, the code expands the image to fill the entire screen even if the original image is smaller than the TFT.
- Memory‑aware: Only the required portion of each BMP row is read into a buffer; the buffer size is checked against maxSize. (For very wide images, you may need to extend the implementation with horizontal strip processing.)
- Centering: The image is always centered on the screen, whether scaled or not.
- Supported format: Uncompressed 24‑bit BMP (the most common format).

### Notes     
- The function assumes the SD library provides a File object with ```seek()```, ```read()```, ```write()```, etc. Adjust the ```SD.open``` path prefix as needed for your filesystem (e.g., SPIFFS or LittleFS).
- If your BMP has negative height (top‑down orientation), it is handled correctly.
- For extremely wide images that exceed maxSize after cropping, you can implement horizontal strip processing by splitting the screen into vertical bands and calling pushImage for each band. The provided code returns false in that case for simplicity.


## Display PNG images       

The PNG function support standard non‑interlaced 24‑bit PNG, if unable to display the image, convert the PNG file with the following settings:     
- Interlaced = OFF (not checked)
- Bit depth = 24‑bit (or "Truecolor")
- No alpha channel if possible (or it may still work)

### Note:     
Transparency – The PNG decoder handles alpha channels; this code uses ```0xffffffff``` as the background colour for blending (fully opaque). If you want transparency to show the TFT background, you can modify the callback to blend pixels.    

### Online Tools          

- To convert PNG to 24-bit PNG: https://elysiatools.com/en/tools/png-to-24bit-png
- To resize or crop image: https://imageresizer.com/crop-image


<img alt="image" style="width: 50%; height: auto;" src="https://github.com/user-attachments/assets/957670a6-59bf-41d9-af09-d6f7dcf1be1a" />



### Library      

PNGdec by Larry Bank          
https://github.com/bitbank2/PNGdec     


### Sample Code     

```cpp
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

### Usage Example:    

```cpp
if (!displayPNG("/image.png", tft, 4096)) {
    Serial.println("Display failed");
}
```

### Key Features   
- Auto‑rotation: The TFT is rotated so that the image’s longer side aligns with the screen’s longer side (```setRotation(1)``` for landscape, 2 for portrait).
- Scaling & Cropping: If the image is larger than the screen, it is scaled to fill the entire display; excess edge pixels are discarded to preserve aspect ratio. If the image is smaller, the code expands the image to fill the entire screen even if the original image is smaller than the TFT.
- Memory‑aware: Only the required portion of each PNG row is read into a buffer; the buffer size is checked against maxSize. (For very wide images, you may need to extend the implementation with horizontal strip processing.)
- Centering: The image is always centered on the screen, whether scaled or not.
- Supported format: Standard non‑interlaced 24‑bit PNG.

## TFT_eSprite     

TFT_eSprite is a class in the TFT_eSPI library that creates an off-screen memory buffer (a canvas). It is highly useful for eliminating screen flicker, building complex UI components, and rendering smooth animations on your ESP32 display.     

### Sprite Declarations     
The two sprite declarations look similar but have important differences in C++:      
```cpp
TFT_eSprite needle(&tft);                      // Direct initialization
TFT_eSprite needle = TFT_eSprite(&tft);        // Copy-initialization
```
Detailed Breakdown:     

|Declaration|What it does|
| - | - |
|```TFT_eSprite needle(&tft);```|Direct initialization – calls the constructor ```TFT_eSprite(TFT_eSPI*)``` directly with ```&tft```. No temporary object is created; needle is constructed in place. This is the standard, most efficient way.|
|```TFT_eSprite needle = TFT_eSprite(&tft);```| Copy-initialization – creates a temporary ```TFT_eSprite``` object using the same constructor, then copy‑constructs (or move‑constructs) needle from that temporary.|

**⚠️ The Critical Issue: Copy Constructor**     
The TFT_eSprite class (from the TFT_eSPI library) does not provide a copy constructor – in fact, it is almost certainly deleted (or implicitly deleted) because the sprite owns a dynamically allocated frame buffer. Copying a sprite would cause two objects to point to the same buffer, leading to double‑free errors.    

If the copy constructor is deleted, the second line:     
```cpp
TFT_eSprite needle = TFT_eSprite(&tft);
```
will fail to compile with an error like:     
```
error: use of deleted function ‘TFT_eSprite::TFT_eSprite(const TFT_eSprite&)’   
```
Even if the copy constructor exists (unlikely), the second form is less efficient because it creates a temporary, copies it, and then discards the temporary.     


**✅ Recommended Way**     
Always use direct initialization:
```cpp
TFT_eSprite needle(&tft);
```
This avoids unnecessary temporaries and works because it doesn't require a copy constructor.     
**📝 Additional Note**      
If you need to declare a sprite pointer (e.g., for dynamic allocation), you would do:       
```cpp
TFT_eSprite* needle = new TFT_eSprite(&tft);
```
But the direct object declaration is simpler and safer.      
**📌 Summary**       
| |TFT_eSprite needle(&tft);	|TFT_eSprite needle = TFT_eSprite(&tft);|
| - | - | - |
|How it works|	Direct construction|	Copy‑initialization (temporary + copy)|
|Requires copy ctor?|	No	|Yes – and it's usually deleted|
Compiles?|	✅ Yes|	❌ No (in most TFT_eSPI versions)|
Efficiency|	Best|	Worse (temporary overhead)|

So always use the first form.     

### Core Workflow     
To use sprites effectively, follow this standard sequence in your code:     
1. **Instantiation**: Create an instance of ```TFT_eSprite``` linked to your main ```TFT_eSPI``` object.
2. **Allocation**: Define the size of your sprite in memory using ```createSprite()```.
3. **Drawing**: Use sprite equivalents of standard commands (e.g., ```drawString()```, ```fillRect()```) to draw to the buffer.
4. **Rendering**: Push the completed sprite to the screen using ```pushSprite()```.
5. **Memory Management**: Free up ESP32 RAM by using ```deleteSprite()``` when the sprite is no longer needed.     

**NOTE:** When you draw inside a Sprite, the coordinates are relative to the Sprite itself (where 0,0 is the top-left corner of the sprite), not the main screen.

### Memory & Color Settings     
#### Color Depths           
Sprites support different color depths to help balance rendering speed against RAM usage on the ESP32:      
- **8-bit (Color)**: Uses 1 byte per pixel. Requires less RAM.
- **16-bit (RGB565)**: Uses 2 bytes per pixel. Default setting for high-quality color.
- **1-bit (Palette/Monochrome)**: Uses 1 bit per pixel. Extremely memory-efficient; ideal for e-paper displays or compressing static bitmaps. Set using ```spr.setColorDepth(1)```;.

#### Color Depth Syntax     
To declare a custom color depth, insert the function immediately after creation:      
```cpp
spr.createSprite(100, 100);
spr.setColorDepth(8); // Switch to 8-bit color depth
```

### pushSprite()      
**1. Full Sprite Pushing**      
```pushSprite(x, y)``` is a fast, direct pixel-transfer function. It copies a source Sprite and pastes it onto a destination (the TFT screen or another Sprite) at exact (x, y) coordinates. It always aligns the top-left corner of the source sprite to the target location. Because it involves no complex math, it is extremely fast and efficient.                  
```cpp
spr.pushSprite(x, y);
```

**2. Full Sprite Pushing with Transparency**   
   Renders the entire sprite while ignoring a specific background color, allowing graphics to appear transparent.
```cpp
spr.pushSprite(x, y, transparent_color);
```
**3. Partial Sprite Pushing**     
   Copies a specific sub-rectangle of the sprite onto the screen.     
```cpp
spr.pushSprite(tft_x, tft_y, sprite_x, sprite_y, width, height);
```       


#### Best Practices     
- **Avoid Full-Screen Sprites**: ESP32 RAM is limited. Allocating a full-screen sprite (e.g., 320 × 240 × 2 bytes) can quickly cause a memory crash (OutOfMemory). Use smaller sprites for text, gauges, or small icons instead.
- **Use Transparency**: To overlay sprites without overwriting the background, use ```pushSprite(x, y, transparent_color)```. For example, if your background is black, use ```spr.pushSprite(x, y, TFT_BLACK);```.

For a full guide on installing and setting up the base TFT_eSPI library for ESP32 boards, you can refer to the [TFT_eSPI Getting Started Docs](https://doc-tft-espi.readthedocs.io/graphics/)) or follow this setup guide:


#### Code Examples     

Scrolling Text:     
```cpp
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();           // Main TFT object
TFT_eSprite spr = TFT_eSprite(&tft); // Sprite object linked to TFT

int x_pos = 0;
int x_dir = 2;

void setup() {
  tft.init();
  tft.setRotation(1);
  
  // Create a sprite canvas (120 pixels wide by 50 pixels high)
  spr.createSprite(120, 50);
}

void loop() {
  // 1. Clear the sprite background (instead of the whole screen)
  spr.fillSprite(TFT_BLACK);
  
  // 2. Set font and colors within the sprite
  spr.setTextColor(TFT_GREEN, TFT_BLACK); // Text and background color
  spr.drawString("ESP32", 10, 15, 4);      // String, x, y, font index

  // 3. Push the sprite to the screen
  spr.pushSprite(x_pos, 50); // Push to X position, Y position

  // 4. Update position for movement
  x_pos += x_dir;
  if (x_pos > 200 || x_pos < 0) {
    x_dir = -x_dir; // Bounce
  }
  
  delay(10);
}
```
Boucing Ball:      
```cpp
#include <TFT_eSPI.h>
TFT_eSprite ball = TFT_eSprite(&tft);  // Sprite object linked to TFT
int x_pos_ball = 320 / 2;
int y_pos_ball = 480 / 2;
int x_dir_ball = 2;
int y_dir_ball = 4;
#define R_BALL 35
// 1. MAKE THE SPRITE LARGER: Add padding so the sprite box covers its own trail.
// Since your max speeds are x_dir=2 and y_dir=4, adding 10-15 pixels of padding is perfect.
#define PADDING 10
#define SPRITE_SIZE ((R_BALL * 2) + PADDING)

void setup() {
  tft.init();
  tft.setRotation(1);
  
  // Create a sprite canvas
    ball.createSprite(SPRITE_SIZE, SPRITE_SIZE);
}

void loop() {
  // 2. Clear the internal Sprite canvas
  ball.fillSprite(TFT_BLACK);

  // 3. Draw the ball in the CENTER of the Sprite (Local coordinates)
  ball.fillCircle((SPRITE_SIZE / 2), (SPRITE_SIZE / 2), R_BALL, TFT_ORANGE);

  // 4. Push the Sprite to the TFT Screen (Global coordinates)
  // We subtract SPRITE_SIZE/2 so x_pos_ball and y_pos_ball represent the center of the ball
  ball.pushSprite((x_pos_ball - (SPRITE_SIZE / 2)), (y_pos_ball - (SPRITE_SIZE / 2)));


  // Update the new coordinate after clearing the old sprite location with tft.fillRect, avoid using tft.fillScreen(TFT_BLACK);
  // 5. Move the ball
  x_pos_ball += x_dir_ball;
  y_pos_ball += y_dir_ball;

  // 6. Bounce Logic (accounting for radius so it bounces off edges properly)
  if (x_pos_ball > (tft.width() - R_BALL) || x_pos_ball < R_BALL) {
    x_dir_ball = -x_dir_ball;  // Fixed: Uncommented bounce
    x_pos_ball += x_dir_ball;
  }

  if (y_pos_ball > (tft.height() - R_BALL) || y_pos_ball < R_BALL) {
    y_dir_ball = -y_dir_ball;  // Bounce
    y_pos_ball += y_dir_ball;
  }

  delay(10);

}
```     

### pushRotated      
```pushRotated(angle)``` is a geometric transformation function. It takes a source Sprite, rotates its pixels by a specified angle, and pastes it onto a destination. Instead of using (x, y) coordinates, it aligns the pivot point of the source sprite with the pivot point of the destination. Because it requires trigonometric calculations to map the rotated pixels, it is computationally heavier, but it is essential for drawing dynamic, rotating elements like gauge needles or clock hands.     
Examples:      
```cpp

         // Push a rotated copy of Sprite to TFT with optional transparent colour
bool     pushRotated(int16_t angle, uint32_t transp = 0x00FFFFFF);
         // Push a rotated copy of Sprite to another different Sprite with optional transparent colour
bool     pushRotated(TFT_eSprite *spr, int16_t angle, uint32_t transp = 0x00FFFFFF);

needle.pushRotated(angle, TFT_WHITE);               // rotate at angle and ignore background color TFT_WHITE
needle.pushRotated(&meterArea, angle, TFT_BLACK);   // push to another sprite meterArea
```      

### setPivot       
#### 1. pushRotated to TFT screen direatly      
To draw a needle directly to the TFT screen at any arbitrary location, you use the version of ```pushRotated()``` that does not take a sprite pointer.
When pushing directly to the TFT, the library aligns the Source Sprite's Pivot with the TFT's Pivot.
Here is exactly how to do it:
```cpp
// --- INITIALIZATION ---
// 1. Create and draw the needle
needle.createSprite(CLOCKHAND_WIDTH, CLOCKHAND_HEIGHT);
needle.setSwapBytes(true); // Fix the red/blue color swap
needle.pushImage(0, 0, CLOCKHAND_WIDTH, CLOCKHAND_HEIGHT, clockhand);

// 2. Set the needle's pivot (where the physical pin is inside the image)
needle.setPivot(NEEDLE_CENTERX, NEEDLE_CENTERY);

// --- DRAWING FUNCTION ---
void drawNeedleOnScreen(float angle, int screenX, int screenY) {
  // 1. Tell the TFT where the center of rotation is on the physical screen
  tft.setPivot(screenX, screenY);
  
  // 2. Push directly to the TFT. 
  // The library automatically aligns needle.setPivot() with tft.setPivot()
  needle.pushRotated((int16_t)angle, TFT_BLACK);
}
```      

**How it works:**     
- ```tft.setPivot(screenX, screenY)``` places the "nail" on your physical screen at (screenX, screenY).
- ```needle.setPivot(...)``` defines the "hole" in your needle image.
- ```needle.pushRotated(angle)``` rotates the needle around its hole, and then docks that hole directly onto the TFT's nail.     

**⚠️ CRITICAL WARNING: The "Smearing" Problem**       
Because you are drawing directly to the TFT without a destination sprite to hold the background, the TFT does not automatically erase the old needle.     
If you call ```drawNeedleOnScreen()``` in a loop to animate the gauge, the new needle will draw on top of the old needle, creating a massive smeared mess.
How to fix the smearing:      
Since you don't have a destination sprite to ```fillSprite()``` and redraw, you must manually erase the old needle before drawing the new one.
The Solution: Create a bigger sprite area     
Before pushing the new angle, ```fillSprite()``` with background color and redraw needle before ```pushRotated()```.      

```cpp
// in setup():
  needle.createSprite(CLOCKHAND_HEIGHT * 2, CLOCKHAND_HEIGHT * 2);    // ensure the area cover the needle length
  needle.fillSprite(TFT_BLACK);                                       // background color
// now the sprite area is larger, need to move the needle to the center of the sprite and anchor the rotating pivot
  needle.pushImage(CLOCKHAND_HEIGHT-NEEDLE_CENTERX, CLOCKHAND_HEIGHT-NEEDLE_CENTERY, CLOCKHAND_WIDTH, CLOCKHAND_HEIGHT, clockhand);
  needle.setPivot(CLOCKHAND_HEIGHT, CLOCKHAND_HEIGHT);                // set the rotating pivot at center of the sprite
  tft.setPivot(METER_X+CLOCKHAND_HEIGHT, METER_Y+CLOCKHAND_HEIGHT);   // pivot point to lay the image on tft screen

// in loop():
  needle.fillSprite(TFT_BLACK);
  needle.pushImage(CLOCKHAND_HEIGHT-NEEDLE_CENTERX, CLOCKHAND_HEIGHT-NEEDLE_CENTERY, CLOCKHAND_WIDTH, CLOCKHAND_HEIGHT, clockhand);
  needle.pushRotated(angle, TFT_BLACK);  // needle with black background color TFT_BLACK 
```      

#### 2. pushRotated to another sprite          
When pushing a rotated needle into a bigger sprite, you only need to think about two pivot points, and they work together like a hinge and a nail on a wall.     
The Mental Model. imagine:        
- The destination sprite is a wall.
- The source sprite (needle) is a clock hand.
- The destination pivot is the nail you hammer into the wall.
- The source pivot is the hole at the base of the clock hand.
- pushRotated() hangs the hole onto the nail and spins it.

**1. Source Pivot (Needle) → "Where is the hole?"**
This is the coordinate inside the needle image where the physical pin/hinge is.     
```
Needle Image (e.g., 20 x 100 pixels)
  ┌───────┐
  │   ▲   │  (0,0) is top-left
  │   │   │
  │   │   │  ← needle body
  │   │   │
  │   ●   │  ← The physical pin is HERE
  └───────┘
     (10, 90)  ← So setPivot(10, 90)

needle.setPivot(10, 90);  // The "hole" in the needle
```      
**2. Destination Pivot (Gauge) → "Where is the nail?"**       
This is the coordinate inside the gauge sprite where the center of the dial is drawn.    
```
Gauge Sprite (e.g., 240 x 240 pixels)

      (120, 120)
          ●  ← Center of dial = "the nail"
         /|\
        / | \

gaugeSprite.setPivot(120, 120);  // The "nail" on the wall
```    
*Important*: This is relative to the gauge sprite's own (0,0) top-left corner, not the physical screen.      
What Happens During pushRotated        
```cpp
needle.pushRotated(&gaugeSprite, 45, TFT_MAGENTA);
```
The library does this internally:
- Takes the needle image
- Rotates it 45° around its pivot (10, 90)
- Moves the entire rotated image so that point (10, 90) lands exactly on (120, 120) of the gauge sprite
```
Before rotation:                After rotation & docking:

  Needle pivot (10,90)          Gauge pivot (120,120)
       ●                              ●
       |                             /
       |                            /  ← needle rotated 45°
       |                           /
      tip                        tip
```          
**⚠️ Critical: Destination Sprite Must Be Big Enough**        
When a sprite rotates, its bounding box grows. A 20×100 needle rotated 45° needs roughly 85×85 pixels of space.      
If your gauge sprite is too small, the rotated needle gets clipped at the edges:     
```
┌─────────────────┐
│    ╱  ← clipped!│
│   ╱             │
│  ●              │  ← gauge sprite boundary
│                 │
└─────────────────┘
```
*Rule of thumb*: The destination sprite should be at least as large as 2 times of source sprite length     

Complete Working Example:      
```cpp
// --- INIT ---
// Needle: 20 wide, 100 tall, pin at bottom-center
needle.createSprite(20, 100);
needle.setSwapBytes(true);
needle.pushImage(0, 0, 20, 100, clockhand);
needle.setPivot(10, 95);         // Hole: bottom-center of needle

// Gauge: 240x240, dial center at (120, 120)
gaugeSprite.createSprite(240, 240);
gaugeSprite.setPivot(120, 120);  // Nail: center of dial

// --- LOOP ---
void drawNeedle(float angle) {
  // 1. Redraw gauge background (erase old needle)
  gaugeSprite.fillSprite(COLOR_BG);
  drawDialFace();  // your dial drawing function

  // 2. Prepare needle with transparent background
  needle.fillSprite(TFT_MAGENTA);
  needle.pushImage(0, 0, 20, 100, clockhand);

  // 3. Dock the hole onto the nail and rotate
  needle.pushRotated(&gaugeSprite, (int16_t)angle, TFT_MAGENTA);

  // 4. Push combined result to screen
  gaugeSprite.pushSprite(0, 0);       // or define X and Y on screen
}
```


### Sprite deletion      
To release a sprite generated with the TFT_eSPI library, you call the ```deleteSprite()``` method on the sprite object. This frees the internal frame buffer that was allocated by ```createSprite()```.

**✅ Basic Usage**     
```cpp
TFT_eSprite mySprite = TFT_eSprite(&tft);  // create the object
mySprite.createSprite(100, 100);            // allocate memory
// ... use the sprite ...
mySprite.deleteSprite();                    // release memory
```

**🔄 Reusing a Sprite Object**       
You can delete a sprite and then recreate it with different dimensions using the same object:     
```cpp
mySprite.deleteSprite();          // free old buffer
mySprite.createSprite(200, 50);   // allocate new buffer
```
**🧠 Important: Sprites Created with new**     
If you allocated the sprite on the heap using new (like ```TFT_eSprite sprite = new TFT_eSprite(&tft);```), you must:    
1. Call ```sprite->deleteSprite();``` to free its internal buffer.
2. Call ```delete sprite;``` to free the object itself.

```cpp
TFT_eSprite* sprite = new TFT_eSprite(&tft);
sprite->createSprite(32, 32);
// ...
sprite->deleteSprite();   // free buffer
delete sprite;            // free object
```

**⚠️ Watch Out for Memory Fragmentation**          
Frequently creating and deleting sprites of different sizes can cause heap fragmentation – especially on memory‑constrained microcontrollers (ESP8266, Arduino Uno). To avoid this, it's often better to:

Create all sprites once during ```setup()``` and keep them alive.     

Reuse the same sprite (delete and recreate) only when absolutely necessary.    

**🔍 Verifying Memory Release**      
You can check free heap memory before and after to confirm deletion:
```cpp
#ifdef ESP32
  Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
#endif
mySprite.deleteSprite();
#ifdef ESP32
  Serial.printf("After delete: %u\n", ESP.getFreeHeap());
#endif
```

**📌 Summary**     
|Action|	Method|
|-|-|
|Release sprite buffer	|```sprite.deleteSprite()```|
|Release object (if new used)	|```delete sprite```|
|Recreate sprite	|```sprite.createSprite(w, h)``` after deleting|

Always call ```deleteSprite()``` when you're done with a sprite to keep your RAM usage under control.     

Refer to this for more details about:
- [Sprite.h](https://github.com/Bodmer/TFT_eSPI/blob/master/Extensions/Sprite.h)
- [TFT_eSPI.h](https://github.com/Bodmer/TFT_eSPI/blob/master/TFT_eSPI.h)
