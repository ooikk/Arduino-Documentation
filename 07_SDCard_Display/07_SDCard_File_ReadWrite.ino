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

bool writeTextFile(const String& filename, const String& textData, bool append);
bool writeBinFile(const String& filename, const uint8_t* binData, size_t dataSize, bool append);
String readTextFile(String filename);
uint8_t* readBinFile(String filename, size_t& outSize);


void waitForSerial() {

  // Clear the serial buffer so it doesn't instantly loop again
  while (Serial.available())
    Serial.read();
  Serial.println("Press <SEND> (or type a character) to advance...");
  // Wait here until a character is received over Serial
  while (!Serial.available())
    ;
}

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
    Serial.println("SD Card initialization failed! Try again...");
    if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {
      while (1) delay(1000);  // Halt
    }
  }

  Serial.println("SD Card initialized successfully.");
  tft.println("SD Card OK");

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  tft.printf("Size: %lluMB", cardSize);

  listRootdir();
  Serial.println("File List Successfully.");
  tft.println("File List Successfully.");


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

  waitForSerial();
  // ==========================================
  // 2. Testing Binary File (Write and Append)
  // ==========================================
  Serial.println("\n--- Testing Binary File ---");

  // Create some dummy binary data
  uint8_t dataBlock1[] = { 0xDE, 0xAD, 0xBE, 0xEF };
  uint8_t dataBlock2[] = { 0xCA, 0xFE, 0xBA, 0xBE };

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

  waitForSerial();

  // --- 3. Test Reading Text File ---
  Serial.println("\n--- Reading Text File ---");
  String textData = readTextFile("/log.txt");

  if (textData.length() > 0) {
    Serial.println("Text Content:");
    Serial.println(textData);
    Serial.printf("File size: %d bytes\n", textData.length());
  }

  waitForSerial();

  // --- 4. Test Reading Binary File ---
  Serial.println("\n--- Reading Binary File ---");
  size_t binSize = 0;
  uint8_t* binData = readBinFile("/sensor.bin", binSize);

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



  Serial.println("Completed file access testing.");
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


/***************************************************************
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

***************************************************************/

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

/***************************************************************
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
***************************************************************/

/*
⚠️ Important Note on Overwriting in Arduino: 
In the standard Arduino SD.h library, the FILE_WRITE mode is hardcoded to append data. To overwrite a file,
the most reliable method is to delete the existing file first using SD.remove() before opening it in write mode.

Key Design Choices:
1. Return Type (bool): File operations can fail (e.g., SD card full, bad sector, write error). 
Returning a boolean allows your main code to check if the write was actually successful.

2. Pass by Reference (const String&): For the text function, passing the string by const reference prevents the Arduino from 
creating an unnecessary copy of the string in memory, which is crucial for preventing memory fragmentation on low-RAM boards.
3. Verification: The functions check the return value of file.print() and file.write(). If the SD card becomes full halfway 
through writing, the function will catch it and return false.

*/



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
/***************************************************************
// --- 1. Test Reading Text File ---
  Serial.println("\n--- Reading Text File ---");
  String textData = readTextFile("/example.txt");
  
  if (textData.length() > 0) {
    Serial.println("Text Content:");
    Serial.println(textData);
  }
***************************************************************/


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

/***************************************************************
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
 ***************************************************************/

/*
⚠️ Important Memory Notes for Arduino:

1. Binary File Memory Management: Because readBinFile uses malloc() to create the array, the memory is allocated on the heap. 
You must call free(binData) when you are completely done using the array, otherwise, your Arduino will run out of memory (Memory Leak).

2. Text File Memory Fragmentation: The String class in Arduino can cause memory fragmentation if used heavily or with very large files. 
If you are reading a massive text file on a memory-constrained board (like an Arduino Uno), it is better to process the text line-by-line 
inside the while(file.available()) loop rather than appending it all to a single String.

3. File Paths: Ensure your filenames include the extension (e.g., "log.txt", "sensor_data.bin"). If your files are inside a folder, 
use the full path (e.g., "folder/data.txt"). Note that the standard Arduino SD.h library requires folder names to be 8.3 format 
(max 8 characters for the name, max 3 for the extension).

*/