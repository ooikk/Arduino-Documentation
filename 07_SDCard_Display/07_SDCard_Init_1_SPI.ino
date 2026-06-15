#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

#define SD_CS_PIN 7
#define SD_FREQUENCY 16000000

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

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  tft.printf("Size: %lluMB", cardSize);

  listRootdir();
  Serial.println("File List Successfully.");
  tft.println("File List Successfully.");
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