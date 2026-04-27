#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         5
#define SS_PIN          10

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// Global or static pointer to store the result
byte* readData; 

void setup() {
  Serial.begin(115200);
  SPI.begin(12, 13, 11, 10);
  mfrc522.PCD_Init();

  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // Now you can assign the result to a variable
  readData = readBlock(1, 1); 

  if (readData != nullptr) {
    Serial.print("Data Returned: ");
    for (byte i = 0; i < 16; i++) {
      Serial.print(readData[i] < 0x10 ? " 0" : " ");
      Serial.print(readData[i], HEX);
    }
    Serial.println();
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

// Function modified to return a byte pointer
byte* readBlock(byte sector, byte block) {
  static byte buffer[18]; // 'static' ensures the data stays in memory after function ends
  byte size = sizeof(buffer);
  byte absoluteBlock = (sector * 4) + block;

  // 1. Authenticate
  MFRC522::StatusCode status;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, absoluteBlock, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
    Serial.print("Auth failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return nullptr; // Return null if authentication fails
  }

  // 2. Read
  status = mfrc522.MIFARE_Read(absoluteBlock, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Read failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return nullptr; // Return null if read fails
  }

  return buffer; // Return the pointer to the first byte of data
}
