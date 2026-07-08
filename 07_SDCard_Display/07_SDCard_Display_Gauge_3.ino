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

#ifdef VSPI_PIN
SPIClass sdSPI(VSPI);
#else
SPIClass sdSPI(HSPI);
#endif

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite gauge = TFT_eSprite(&tft);  // Canvas buffer to prevent flicker

// =========================================================================
// 1. RESOLUTION RECONFIGURATION PANEL (SETS BOTH SCREEN & SPRITE LAYOUT)
// =========================================================================
// Simply change these parameters to adapt instantly to your physical display.
const int SCREEN_WIDTH = 320;   // e.g., 240 for 240x240, 320 for standard landscape
const int SCREEN_HEIGHT = 240;  // e.g., 240 for 240x240, 240 for standard landscape

// Size of the square gauge widget box. RAM calculation footprint:
// 240x240 pixels * 2 bytes (16-bit color) = ~115 KB (Perfect safe fit for ESP32)
const int G_SIZE = 240;
const int CENTER = G_SIZE / 2;  // Middle relative origin offset (120)
const int RADIUS = 105;         // Base tracking perimeter outer bounds

// =========================================================================
// 2. USER DEFINABLE CONFIGURATION MATRIX
// =========================================================================
const float RANGE_MIN = 0.0;           // Scale bottom limit
const float RANGE_MAX = 100.0;         // Scale upper threshold limit
const char* GAUGE_TITLE = "PRESSURE";  // Overlay contextual header text
const char* UNIT_TEXT = "PSI";         // Units metric string text

// 3. CRITICAL ALARM STATUS COLOR THRESHOLDS
const float WARNING_THRES = 70.0;   // Threshold marker where meter goes yellow
const float CRITICAL_THRES = 88.0;  // Threshold marker where meter goes red

// Modern Aesthetic Dark UX Interface Palette
#define COLOR_BG 0x10A2         // Deep slate canvas backing tint
#define COLOR_CARD 0x9d70       //0x18E4     // Inner module panel structure framing base
#define COLOR_TICK_MUTE 0x4208  // Subdued low contrast grid markings
#define COLOR_WHITE 0xFFFF      // Clean standard reading values

// Fresh Active Status State Colors
#define FRESH_MINT 0x5FFF      // Safe Operations Normal (Mint Green)
#define ALERT_AMBER 0xFD00     // Warning Active Range (Crisp Amber Yellow)
#define DANGER_CRIMSON 0xF800  // System Emergency Fault (Pure Crimson Red)

// Math Track Angle Anchors (Degrees mapping matching physical clock hands positions)
const float START_ANG = 135.0;  // Start arc (Bottom-Left)
const float END_ANG = 405.0;    // Finish arc (Bottom-Right)

float currentValue = RANGE_MIN;
float targetValue = RANGE_MIN;
void simulateSensorData();




void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize TFT Display
  Serial.println("Initializing TFT...");
  tft.init();
  //tft.setRotation(2);
  //tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  Serial.println("TFT ready...");
  tft.println("TFT ready...");

  tft.setRotation(1);  // Lock to native horizontal viewing orientation
  tft.fillScreen(COLOR_BG);

  // Initialize and assign hardware buffer space blocks
  gauge.createSprite(G_SIZE, G_SIZE);
  gauge.setPivot(CENTER, CENTER);  // Bind rotation matrices to internal center anchors



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
  //tft.println("SD Card OK");

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  //tft.printf("Size: %lluMB", cardSize);
}

void loop() {
  simulateSensorData();  // Generates tracking waves dynamically

  // Smooth raw input using Linear Interpolation (lerp) filter math
  //currentValue += (targetValue - currentValue) * 0.12;
  currentValue = targetValue;

  // Flush background buffer card surface canvas
  gauge.fillSprite(COLOR_CARD);

  // Translate numeric scale values directly into layout polar angle steps
  float progressPct = (currentValue - RANGE_MIN) / (RANGE_MAX - RANGE_MIN);
  progressPct = constrain(progressPct, 0.0, 1.0);
  float currentAngle = START_ANG + (progressPct * (END_ANG - START_ANG));

  // 1. DYNAMIC COLOR EVALUATION THRESHOLD LOGIC
  uint16_t activeDynamicColor = FRESH_MINT;
  if (currentValue >= CRITICAL_THRES) {
    activeDynamicColor = DANGER_CRIMSON;
  } else if (currentValue >= WARNING_THRES) {
    activeDynamicColor = ALERT_AMBER;
  }

  // 2. RENDER SUBPRIMITIVE STRUCTURAL BLOCKS
  drawAnalogTicks();
  drawGaugeTrack(progressPct, activeDynamicColor);
  drawModernNeedle(currentAngle, activeDynamicColor);
  drawTypography(activeDynamicColor);

  // 3. ZERO-FLICKER SCREEN INJECTION PUSH
  // Center math balances alignment completely independent of what screen limits are specified above
  int xOffset = (SCREEN_WIDTH - G_SIZE) / 2;
  int yOffset = (SCREEN_HEIGHT - G_SIZE) / 2;
  gauge.pushSprite(xOffset, yOffset);

  delay(16);  // Throttled processing cycle targeting silky fluid ~60 fps steps
}

// Mathematical vector generation engine rendering high density radial indicator ticks
void drawAnalogTicks() {
  int totalTicks = 21;  // Creates 20 balanced split segments
  float angleStep = (END_ANG - START_ANG) / (totalTicks - 1);

  for (int i = 0; i < totalTicks; i++) {
    float currentTickAngle = START_ANG + (i * angleStep);
    float rad = (currentTickAngle - 90.0) * DEG_TO_RAD;  // Phase correction step
                                                         //    float rad = (currentTickAngle - 0.0) * DEG_TO_RAD;  // Phase correction step

    // Polar coordinate resolution conversion transformations
    float cosRad = cos(rad);
    float sinRad = sin(rad);

    // Render ticks sitting inside inner radius layout walls
    int innerStart = RADIUS - 16;
    int innerEnd = RADIUS - 10;

    int tx0 = CENTER + cosRad * innerStart;
    int ty0 = CENTER + sinRad * innerStart;
    int tx1 = CENTER + cosRad * innerEnd;
    int ty1 = CENTER + sinRad * innerEnd;

    // Establish structural visibility colors based on raw tick range mapping positions
    float tickValueVal = RANGE_MIN + ((float)i / (totalTicks - 1)) * (RANGE_MAX - RANGE_MIN);
    uint16_t tickColor = COLOR_TICK_MUTE;

    if (currentValue >= tickValueVal) {
      if (tickValueVal >= CRITICAL_THRES) tickColor = DANGER_CRIMSON;
      else if (tickValueVal >= WARNING_THRES) tickColor = ALERT_AMBER;
      else tickColor = FRESH_MINT;
    }

    // Draw fine structural lines
    gauge.drawLine(tx0, ty0, tx1, ty1, tickColor);
  }
}

// Generates structural tracking arcs with anti-aliasing edges
void drawGaugeTrack(float pct, uint16_t dynamicColor) {

  // Empty background static containment guide channel ring
  // drawSmoothArc 0 deg is at 6 O'clock position, need to -90, total -180 due to  addition -90 phase correction at drawLine
  // START_ANG - 180 = -45, so add 360 to make it positive
  float tempAngle_Start = START_ANG - 180;
  //tempAngle_Start < 0 ? tempAngle_Start + 360 : tempAngle_Start;
  if (tempAngle_Start < 0) {
    tempAngle_Start += 360;
  }
  float tempAngle_End = END_ANG - 180;
  if (tempAngle_End < 0) {
    tempAngle_End += 360;
  }
  gauge.drawSmoothArc(CENTER, CENTER, RADIUS, RADIUS - 8, tempAngle_Start, tempAngle_End, COLOR_BG, COLOR_CARD);
  // Overlay responsive dynamic progress indicator stroke filling track
  if (pct > 0.01) {
    float activeEndAngle = START_ANG + (pct * (END_ANG - START_ANG)) - 180;

    if (activeEndAngle < 0) {
      activeEndAngle += 360;
    }
    gauge.drawSmoothArc(CENTER, CENTER, RADIUS, RADIUS - 8, tempAngle_Start, activeEndAngle, dynamicColor, COLOR_CARD);
  }
}



// Sleek needle rendering using asymmetric geometric wedge geometry
void drawModernNeedle(float angle, uint16_t dynamicColor) {
  float rad = (angle - 90.0) * DEG_TO_RAD;
  float cosRad = cos(rad);
  float sinRad = sin(rad);

  // Floating design: needle suspended away from text box extending past the arc track
  int startGapDistance = 42;
  int endTipDistance = RADIUS + 4;

  int sx = CENTER + cosRad * startGapDistance;
  int sy = CENTER + sinRad * startGapDistance;
  int ex = CENTER + cosRad * endTipDistance;
  int ey = CENTER + sinRad * endTipDistance;

  // Renders a modern anti-aliased wedge line narrowing from 3px thickness down to a sharp 1px at the tip
  gauge.drawWedgeLine(sx, sy, ex, ey, 3, 1, dynamicColor);
}

// Modern UI typography layouts
void drawTypography(uint16_t dynamicColor) {
  gauge.setTextDatum(MC_DATUM);  // Lock alignment origin points directly to middle-center text coordinates

  // Contextual Sub-header Metadata
  gauge.setTextColor(0x8410, COLOR_CARD);  // Muted silver gray alpha
  gauge.drawString(GAUGE_TITLE, CENTER, CENTER - 30, 4);

  // Main Digital Core Readout
  gauge.setTextColor(COLOR_WHITE, COLOR_CARD);
  char valBuffer[10];
  dtostrf(currentValue, 4, 1, valBuffer);               // Standard string format parsing safely with 1 floating decimal point
  gauge.drawString(valBuffer, CENTER, CENTER + 30, 6);  // Renders using robust built-in High-Contrast Font 6

  // Responsive Unit Badge Footer Box
  int textWidth = gauge.textWidth(UNIT_TEXT, 2) + 12;
  int boxHeight = 16;
  gauge.fillRoundRect(CENTER - (textWidth / 2), CENTER + 60, textWidth, boxHeight, 4, COLOR_BG);

  gauge.setTextColor(dynamicColor, COLOR_BG);  // Unit color changes dynamically to provide crisp context states
  gauge.drawString(UNIT_TEXT, CENTER, CENTER + 68, 2);
}

// Basic simulated sine sweep engine mirroring genuine sensor fluctuations over execution intervals
void simulateSensorData() {
  static unsigned long lastTimeCheck = 0;
  if (millis() - lastTimeCheck > 1000) {
    lastTimeCheck = millis();
    // Drops dynamic target limits anywhere randomly spanning scale minimum directly through scale maximum
    //targetValue = random(RANGE_MIN * 10, RANGE_MAX * 10) / 10.0;
    targetValue += 5;
    if (targetValue > RANGE_MAX)
      targetValue = RANGE_MIN;
  }
}
