#include <SPI.h>
#include <MFRC522.h>

// Choose a Touch-enabled pin from your diagram
// GPIO 1 is labeled as TOUCH1
// comment OFF USE_TOUCHPIN to disable Touch pin
//#define USE_TOUCHPIN

#ifdef USE_TOUCHPIN
const int touchPin = 4;
int touchValue = 0;
#define TOUCHTHRESHOLD 50000
#endif

#define RST_PIN 5  // Configurable, see your pinout
#define SS_PIN 10  // Configurable, see your pinout

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

void setup() {
	Serial.begin(115200);  // Initialize serial communications with PC
	while (!Serial)
		;                         // Do nothing if no serial port is opened
	SPI.begin(12, 13, 11, 10);  // Init SPI bus (SCK, MISO, MOSI, SS)

	/* Add 10uF across Vcc and Gnd */
	SPI.setFrequency(400000);  //(4MHz) before PCD_Init() to slow down the clock for the RFID module.
	mfrc522.PCD_Init();        // Init MFRC522
	Serial.println("V1 Library: Scan PICC to see UID, SAK, type, and data blocks...");
}

void loop() {
	// Reset the loop if no new card present on the sensor/reader.
	if (!mfrc522.PICC_IsNewCardPresent()) {
		return;
	}

	// Select one of the cards
	if (!mfrc522.PICC_ReadCardSerial()) {
		return;
	}

	// Dump debug info about the card; PICC_HaltA() is automatically called
	mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
	//mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));


	Serial.print("UID tag :");
	String Mycontent = "";

	//byte letter;
	for (byte i = 0; i < mfrc522.uid.size; i++) {
		Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
		Serial.print(mfrc522.uid.uidByte[i], HEX);
		Mycontent.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
		Mycontent.concat(String(mfrc522.uid.uidByte[i], HEX));
	}
	Serial.println();
	Serial.print("Message : ");
	Mycontent.toUpperCase();

	if (Mycontent.substring(1) == "2E 69 29 07") {
		Serial.println("Access granted");
		Serial.println();
		delay(3000);
	} else {
		Serial.println(" Access denied");
		delay(3000);
	}


#ifdef USE_TOUCHPIN                  // Wait for touch pin to proceed
	touchValue = touchRead(touchPin);  // throw away first read
	Serial.println("Waiting for touch pin to continue...");
	do {
		touchValue = touchRead(touchPin);
		delay(500);
	} while (touchValue < TOUCHTHRESHOLD);

	Serial.println("Touch detected...");
#endif
}