#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 5
#define SS_PIN 10

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

void setup() {
  Serial.begin(115200);
  SPI.begin(12, 13, 11, 10);
  mfrc522.PCD_Init();

  // Set the key to all 0xFF to match your Key B
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println("Ready to write. Scan card...");
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // --- CONFIGURATION ---
  byte targetSector = 1;                       // 0 to 15
  byte targetBlock = 1;                        // 0 to 3
  byte dataBlock[16] = { "Hello ESP32-S3!" };  // Must be 16 bytes
  // ---------------------

  writeToBlock(targetSector, targetBlock, dataBlock);

  // Halt communication so it doesn't keep writing
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void writeToBlock(byte sector, byte block, byte data[]) {
  // 1. SAFETY MEASURES
  if (sector == 0 && block == 0) {
    Serial.println("ERROR: Cannot write to Sector 0, Block 0 (Manufacturer Data).");
    return;
  }
  if (block == 3) {
    Serial.println("ERROR: Cannot write to Block 3 (Sector Trailer). You might lock your card!");
    return;
  }

  // Calculate the absolute block number (e.g., Sector 1, Block 1 is absolute block 5)
  byte absoluteBlock = (sector * 4) + block;

  // 2. AUTHENTICATION. Using Key B 
  MFRC522::StatusCode status;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, absoluteBlock, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
    Serial.print("Auth failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // 3. WRITE DATA
  status = mfrc522.MIFARE_Write(absoluteBlock, data, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Write failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
  } else {
    Serial.print("Success! Data written to Sector ");
    Serial.print(sector);
    Serial.print(", Block ");
    Serial.println(block);
  }
}
