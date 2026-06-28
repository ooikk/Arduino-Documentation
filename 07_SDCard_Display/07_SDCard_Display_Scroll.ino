#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

// Define the SD Chip Select pin (must match wiring)
#define VSPI_PIN  // SD card is using VSPI and TFT is using HSPI

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
#define SD_FREQUENCY 16000000  // 24000000 16000000 4000000


TFT_eSPI tft = TFT_eSPI();
// 1️⃣ Define SPI class for SD
#ifdef VSPI_PIN
SPIClass sdSPI(VSPI);
#else
SPIClass sdSPI(HSPI);
#endif

TFT_eSprite spr = TFT_eSprite(&tft);  // Sprite object linked to TFT
int x_pos = 0;
int x_dir = 2;

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
  Serial.begin(115200);
  delay(500);

  // Create a sprite canvas (120 pixels wide by 50 pixels high)
  spr.createSprite(120, 50);
  ball.createSprite(SPRITE_SIZE, SPRITE_SIZE);


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

  // 3️⃣ Get the SPI bus instance that the TFT is using
  // SPIClass& sdSPI = tft.getSPIinstance();

  // 3️⃣ Explicitly bind SPI to your SPI pins BEFORE SD.begin()
  // SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN);
  sdSPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN);  // SCK, MISO, MOSI

  // 4️⃣ Now initialize the SD card on the SPI bus
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {
    Serial.println("SD Card initialization failed! Try again...");
    delay(1000);
    if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {
      Serial.println("SD Card initialization failed!");
      tft.println("SD Card init failed!");
      while (1) delay(1000);
    }
  }
  tft.println("SD Card init completed!");

  Serial.printf("Free heap before open: %u\n", ESP.getFreeHeap());


  // ****************** Get card information
  // Get the card object and check read-only
  uint8_t cardType = SD.cardType();
  uint64_t cardSize = SD.cardSize();
  //bool isReadOnly = SD.card()->readOnly();  // may not be available in all versions
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

  tft.fillScreen(TFT_BLACK);

  //waitForSerial();
}

void loop() {

  // 1. Clear the sprite background (instead of the whole screen)
  spr.fillSprite(TFT_BLACK);

  // 2. Set font and colors within the sprite
  spr.setTextColor(TFT_GREEN, TFT_BLACK);  // Text and background color
  spr.drawString("ESP32", 10, 15, 4);      // String, x, y, font index

  // 3. Push the sprite to the screen
  spr.pushSprite(x_pos, 50);  // Push to X position, Y position

  // 4. Update position for movement
  x_pos += x_dir;
  if (x_pos > 200 || x_pos < 0) {
    x_dir = -x_dir;  // Bounce
  }

  //******** Bouncing ball

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


void waitForSerial() {

  // Clear the serial buffer so it doesn't instantly loop again
  while (Serial.available())
    Serial.read();
  Serial.println("Press <SEND> (or type a character) to advance");
  // Wait here until a character is received over Serial
  while (!Serial.available())
    ;
}
