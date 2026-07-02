// Define the SD Chip Select pin (must match wiring)
#define VSPI_PIN  // SD card is using VSPI and TFT is using HSPI

#include <SPI.h>
#include <SD.h>
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

/*
  ESP32 Odometer – Sprite-based, no flicker, fully configurable.
  All drawing is done on a sprite and pushed to the screen.
  All key parameters are defined as constants for easy customization.
  Screen size can range from 100 to 320 pixels (diameter).
*/

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef DEG2RAD
#define DEG2RAD (M_PI / 180.0)
#endif

TFT_eSprite gaugeSprite = TFT_eSprite(&tft);  // sprite for off‑screen rendering


// ======================== USER CONFIGURABLE PARAMETERS ========================

const int MIN_SPEED = 0;
const int MAX_SPEED = 140;
const int RED_ZONE_START = 120;
const int GAUGE_SIZE = 240;  // diameter in pixels
const int MARGIN = 10;
const char* ODO_TITLE = "KM/H";  //

const float START_ANGLE_DEG = 135.0;  // upper‑left
const float END_ANGLE_DEG = 405.0;    // upper‑right (270° sweep)

const uint16_t COLOR_BG = TFT_BLACK;
const uint16_t COLOR_TICKS = TFT_WHITE;
const uint16_t COLOR_RED_TICK = TFT_RED;
const uint16_t COLOR_NUMBERS = TFT_WHITE;
const uint16_t COLOR_NEEDLE = TFT_RED;          //TFT_GREEN;
const uint16_t COLOR_DIGITAL = TFT_BLACK;       //TFT_CYAN;
const uint16_t COLOR_DIGITAL_TITLE = TFT_CYAN;  // KK
const uint16_t COLOR_DIGITAL_BG = 0xfffd;       // KK
const uint16_t COLOR_DIGITAL_FRAME = 0xef7d;    // KK Sivler


const int MAJOR_TICK_UNIT = 10;      //10;
const int MINOR_TICK_UNIT = 2;       //5;
const int NEEDLE_LENGTH_RATIO = 65;  // shortened to avoid numbers

// ======================== COMPUTED CONSTANTS ================================

const int RADIUS = GAUGE_SIZE / 2 - MARGIN;
const int CENTER_X = RADIUS + MARGIN;
const int CENTER_Y = RADIUS + MARGIN;
const int NEEDLE_LENGTH = (RADIUS * NEEDLE_LENGTH_RATIO) / 100;
const int TICK_MAJOR_LEN = RADIUS * 0.08;  //10;
const int TICK_MINOR_LEN = RADIUS * 0.06;

// Number placement – keep inward offset for size 1 (smaller)
const int NUM_OFFSET = 8;  // original offset for size 1

const int SPRITE_W = GAUGE_SIZE + 2 * MARGIN;
const int SPRITE_H = GAUGE_SIZE + 2 * MARGIN;
int spriteOffsetX, spriteOffsetY;

// ======================== FUNCTION PROTOTYPES ==============================

float speedToAngle(float speed);
void drawGauge(float speed);
void drawTicksAndNumbers();
void drawNeedle(float speed);
void drawDigitalValue(float speed);

// ======================== SETUP ============================================
// Odormeter input
float speed = MIN_SPEED;

void setup() {
  Serial.begin(115200);
  delay(1000);


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
      Serial.println("SD Card initialization failed!");
      while (1) delay(1000);  // Halt
    }
  }

  Serial.println("SD Card initialized successfully.");
  tft.println("SD Card OK");

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  //tft.printf("Size: %lluMB", cardSize);

  // *****************************************************

  tft.setRotation(1);  // Adjust rotation as needed
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(TFT_WHITE, COLOR_BG);
  tft.setTextSize(1);

  // Center the sprite on the screen
  int screenW = tft.width();
  int screenH = tft.height();

  spriteOffsetX = 0;  //(screenW - SPRITE_W) / 2;
  spriteOffsetY = 0;  //(screenH - SPRITE_H) / 2;

  Serial.printf("Screen W: %d H: %d\n", screenW, screenH);
  Serial.printf("sprite X: %d Y: %d\n", spriteOffsetX, spriteOffsetY);

  gaugeSprite.createSprite(SPRITE_W, SPRITE_H);
  gaugeSprite.setTextColor(TFT_WHITE, COLOR_BG);
  gaugeSprite.setTextSize(1);  // base size, we'll set per element

  drawGauge(0);
}

void loop() {

  // Example: read speed from an analog input (potentiometer)
  // Replace with your own data source.
  // int raw = analogRead(36);                 // GPIO36 (VP) on ESP32
  // float speed = map(raw, 0, 4095, MIN_SPEED, MAX_SPEED);

  // Uncomment below for a demo cycling speed:

  speed += 1.0;
  if (speed > MAX_SPEED) speed = MIN_SPEED;

  drawGauge(speed);

  delay(1);  // update rate
}


// ======================== FUNCTION IMPLEMENTATIONS =========================

float speedToAngle(float speed) {
  speed = constrain(speed, MIN_SPEED, MAX_SPEED);
  float fraction = (speed - MIN_SPEED) / (MAX_SPEED - MIN_SPEED);
  return START_ANGLE_DEG + fraction * (END_ANGLE_DEG - START_ANGLE_DEG);
}

void drawGauge(float speed) {
  gaugeSprite.fillSprite(COLOR_BG);
  drawTicksAndNumbers();
  //  drawNeedle(speed);
  drawDigitalValue(speed);
  drawNeedle(speed);
  gaugeSprite.pushSprite(spriteOffsetX, spriteOffsetY);
}

void drawTicksAndNumbers() {
  // Draw tick marks
  for (int speed = MIN_SPEED; speed <= MAX_SPEED; speed += MINOR_TICK_UNIT) {
    float angleDeg = speedToAngle(speed);
    float angleRad = angleDeg * DEG2RAD;

    int xOut = CENTER_X + RADIUS * cos(angleRad);
    int yOut = CENTER_Y + RADIUS * sin(angleRad);

    bool isMajor = (speed % MAJOR_TICK_UNIT == 0);
    int tickLen = isMajor ? TICK_MAJOR_LEN : TICK_MINOR_LEN;
    int xIn = CENTER_X + (RADIUS - tickLen) * cos(angleRad);
    int yIn = CENTER_Y + (RADIUS - tickLen) * sin(angleRad);

    uint16_t tickColor = (speed >= RED_ZONE_START) ? COLOR_RED_TICK : COLOR_TICKS;
    if (isMajor)
      gaugeSprite.drawWideLine(xOut, yOut, xIn, yIn, 4, tickColor);
    else
      gaugeSprite.drawLine(xOut, yOut, xIn, yIn, tickColor);
  }

  // Draw numbers (major ticks) with font size 1 (keep original)
  gaugeSprite.setTextColor(COLOR_NUMBERS, COLOR_BG);
  gaugeSprite.setTextSize(1);  // <-- scale numbers remain size 1
  gaugeSprite.setTextDatum(MC_DATUM);

  for (int speed = MIN_SPEED; speed <= MAX_SPEED; speed += MAJOR_TICK_UNIT) {
    float angleDeg = speedToAngle(speed);
    float angleRad = angleDeg * DEG2RAD;

    int numRadius = RADIUS - TICK_MAJOR_LEN - NUM_OFFSET;
    int xNum = CENTER_X + numRadius * cos(angleRad);
    int yNum = CENTER_Y + numRadius * sin(angleRad);
    gaugeSprite.drawNumber(speed, xNum, yNum);
  }
}

void drawNeedle(float speed) {
  float angle = speedToAngle(speed);
  float rad = angle * DEG2RAD;
  int xTip = CENTER_X + NEEDLE_LENGTH * cos(rad);
  int yTip = CENTER_Y + NEEDLE_LENGTH * sin(rad);
  //gaugeSprite.drawLine(CENTER_X, CENTER_Y, xTip, yTip, COLOR_NEEDLE);
  //  gaugeSprite.drawWideLine(CENTER_X, CENTER_Y, xTip, yTip,4, COLOR_NEEDLE);
  gaugeSprite.drawWedgeLine(CENTER_X, CENTER_Y, xTip, yTip, 3, 1, COLOR_NEEDLE);

  gaugeSprite.fillCircle(CENTER_X, CENTER_Y, RADIUS * 0.05, COLOR_NEEDLE);
  gaugeSprite.drawCircle(CENTER_X, CENTER_Y, RADIUS * 0.05, COLOR_BG);
}

void drawDigitalValue(float speed) {
  int boxWidth, boxHeight, x0, y0;

  //gaugeSprite.fillRect(x0, y0, boxWidth, boxHeight, COLOR_DIGITAL_BG);
  // Digital speed value – font size 2
  gaugeSprite.setTextColor(COLOR_DIGITAL);  //, COLOR_DIGITAL_BG);  // KK
  gaugeSprite.setTextSize(4);
  boxHeight = gaugeSprite.fontHeight();
  boxWidth = gaugeSprite.textWidth("1234"); //find 4 digit width
  //gaugeSprite.fillRect(x0, y0 - 4, boxWidth, currentFontHeight + 4, COLOR_DIGITAL_BG);
  x0 = CENTER_X - boxWidth / 2;
  y0 = CENTER_Y + RADIUS * 0.3;        // 30% below center
  gaugeSprite.setTextDatum(TC_DATUM);  // MC_DATUM);
  gaugeSprite.fillRoundRect(x0, y0 - 4, boxWidth, boxHeight + 4, 3, COLOR_DIGITAL_BG);
  gaugeSprite.drawRoundRect(x0, y0 - 4, boxWidth, boxHeight + 4, 3, COLOR_DIGITAL_FRAME);
  //gaugeSprite.drawRoundRect(x0 + 1, y0 - 3, boxWidth - 2, boxHeight + 2, 3, COLOR_DIGITAL_FRAME);
  //gaugeSprite.drawRoundRect(x0 + 2, y0 - 2, boxWidth - 4, boxHeight + 0, 3, COLOR_DIGITAL_FRAME);

  char buf[10];
  sprintf(buf, "%d", (int)(speed + 0.5));
  //gaugeSprite.drawString(buf, CENTER_X, y0 + boxHeight * 0.35);
  gaugeSprite.drawString(buf, CENTER_X, y0);  //+ currentFontHeight / 2);

  // "km/h" label – also font size 2
  gaugeSprite.setTextColor(COLOR_DIGITAL_TITLE);  // KK

  //gaugeSprite.setFreeFont(&FreeMono9pt7b);
  gaugeSprite.setTextFont(4);
  gaugeSprite.setTextSize(1);

  gaugeSprite.setTextDatum(TC_DATUM);
  //gaugeSprite.drawString("km/h", CENTER_X, y0 + boxHeight * 0.75);
  gaugeSprite.drawString(ODO_TITLE, CENTER_X, y0 + boxHeight + 4);
  //gaugeSprite.setFreeFont(NULL);
  gaugeSprite.setTextFont(1);
  gaugeSprite.setTextSize(1);
}