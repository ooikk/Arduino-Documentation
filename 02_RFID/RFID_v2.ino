#include <SPI.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h> // Required for the pin wrapper
#include <MFRC522Debug.h>

// 1. Define the pins as MFRC522DriverPin objects
MFRC522DriverPinSimple ss_pin{10}; 
MFRC522DriverPinSimple rst_pin{5}; 

// 2. Pass the pins, the SPI instance, and settings to the driver
// Syntax: Driver(CS_Pin, SPI_Class, SPI_Settings)
MFRC522DriverSPI driver{ss_pin, SPI, SPISettings(4000000, MSBFIRST, SPI_MODE0)}; 

// 3. Create the MFRC522 instance
MFRC522 mfrc522{driver}; 

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Initialize the S3 SPI pins (SCK, MISO, MOSI, SS)
  SPI.begin(12, 13, 11, 10); 
  
  mfrc522.PCD_Init();
  Serial.println("V2 Library initialized with specific pin objects.");
}

void loop() {
  if ( ! mfrc522.PICC_IsNewCardPresent()) return;
  if ( ! mfrc522.PICC_ReadCardSerial()) return;

  MFRC522Debug::PICC_DumpToSerial(mfrc522, Serial, &(mfrc522.uid));
  
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
