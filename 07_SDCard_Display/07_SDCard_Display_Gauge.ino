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


//#define SMALLER_IMAGE
/*
#define SCR_WIDTH 200
#define SCR_HEIGHT 150
#define SCR_OFFSET_X 20
#define SCR_OFFSET_Y 30
*/

TFT_eSPI tft = TFT_eSPI();

// Gauge Meter
TFT_eSprite gaugeSpr = TFT_eSprite(&tft);  // Create a localized sprite instance
int needleAngle = 0;
void drawGaugeNeedle(int centerX, int centerY, int angle, int length, uint16_t color);
void startGaugeMeter(int needleAngle);
#define SPR_DIAMETER 120

// 150x150 pixel twin gauge canvas
TFT_eSprite gaugeMeter = TFT_eSprite(&tft);  // Create a localized sprite instance
float temperature = 0.0;
float speed = 0.0;
void drawCustomGauge(float val, float minVal, float maxVal, const char* label);
#define CUSTOM_DIAMETER 150

// Dedicated sprite for the classic panel gauge
TFT_eSprite classGauge = TFT_eSprite(&tft);
void drawClassicGauge(float val, float minVal, float maxVal, const char* label);
// ==========================================
// USER CONFIGURATION: Set your preferred width
// ==========================================
#define PANEL_WIDTH 280                     // Pick any size (e.g., 160, 200, 240, 280)
#define PANEL_HEIGHT (PANEL_WIDTH * 5 / 8)  // Squashed to a flat 2:1 ratio to completely delete the top white space
#define POLYGON_NEEDLE
// Readout value example: 2.5 Amps on a 0 to 15 scale range
float currentReading = 0.0;

// 1️⃣ Define SPI class for SD
#ifdef VSPI_PIN
SPIClass sdSPI(VSPI);
#else
SPIClass sdSPI(HSPI);
#endif



void setup() {
  Serial.begin(115200);
  delay(500);

  // Allocate memory for a 120x120 pixel canvas for the odometer gauge
  gaugeSpr.createSprite(SPR_DIAMETER, SPR_DIAMETER);

  // Allocate memory block for our 150x150 pixel gauge canvas
  gaugeMeter.createSprite(CUSTOM_DIAMETER, CUSTOM_DIAMETER);

  // Allocate the Sprite memory box ONCE right here at startup
  classGauge.createSprite(PANEL_WIDTH, PANEL_HEIGHT);

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

  /*  Guage 1 */
  startGaugeMeter(needleAngle);
  gaugeSpr.pushSprite(10, 10);
  // Update odometer value for next frame
  needleAngle += 2;
  if (needleAngle > 180) needleAngle = 0;
  //delay(20);  // Smooth ~50 FPS update rate

  /*  Guage 2 */
  // --- DRAW METER 1 (Left Side) ---
  // Value: 40.34, Min: 0, Max: 100, Unit label: " °C"
  drawCustomGauge(temperature, -30.0, 120.0, " °C");  // ensure max and min div by 5
  // Push the complete canvas to screen coordinates X=10, Y=150
  gaugeMeter.pushSprite(10, 140);
  temperature += 5;
  if (temperature > 130) temperature = -30.0;


  // --- DRAW METER 2 (Right Side) ---
  // Value: 62.50, Min: 0, Max: 100, Unit label: " %"
  drawCustomGauge(speed, 0.0, 250.0, " km/h");  // ensure max and min div by 5
  // Push the complete canvas to screen coordinates X=165, Y=150
  gaugeMeter.pushSprite(165, 140);
  speed += 5;
  if (speed > 250) speed = 0.0;

  /*  Guage 3 */
  // Draw the gauge workspace structure
  //drawClassicGauge(currentReading, 0.0, 15.0, "A");
  drawClassicGauge(currentReading, 0.0, 180.0, "SPEED");

  // Push the fixed-size panel cleanly onto the screen (Centered layout example)
  //int xPos = (320 - PANEL_WIDTH) / 2;
  //int yPos = (240 - PANEL_HEIGHT) / 2;
  //classGauge.pushSprite(xPos, yPos);
  classGauge.pushSprite(10, 300);

  //currentReading += 0.5;
  //if (currentReading > 15) currentReading = 0.0;
  currentReading += 10;
  if (currentReading > 180) currentReading = 0.0;

  delay(100);  // Small delay before refreshing data values
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

void startGaugeMeter(int needleAngle) {
  // 1. Clear the hidden canvas workspace
  gaugeSpr.fillSprite(TFT_BLACK);

  // 2. Draw static gauge elements on the sprite
  gaugeSpr.drawCircle(SPR_DIAMETER / 2, SPR_DIAMETER / 2, SPR_DIAMETER / 2 - 5, TFT_WHITE);  // Smaller radius with margin
  gaugeSpr.setFreeFont(&FreeSans9pt7b);
  gaugeSpr.drawCentreString("km/h", SPR_DIAMETER / 2, SPR_DIAMETER / 2 + 20, 1);  // 20 pixel below center of gauge

  // 3. Draw the dynamic updating needle
  // pivot points are center of sprite (60,60), length 45 pixels
  drawGaugeNeedle(SPR_DIAMETER / 2, SPR_DIAMETER / 2, needleAngle, SPR_DIAMETER / 2 - 15, TFT_RED);  // ensure needle length is less than the circle

  // 4. Push the finished, crisp frame instantly to coordinates (X=50, Y=50)
  //gaugeSpr.pushSprite(10, 10);
}

void drawGaugeNeedle(int centerX, int centerY, int angle, int length, uint16_t color) {
  // Convert angle to radians (subtract 90 or 180 degrees depending on where 0 is on your gauge)
  // Assuming 0 degrees points straight left, 90 is straight up, 180 is straight right:
  float radians = (angle - 180) * 3.14159 / 180.0;

  // Calculate endpoint using basic trigonometry
  int endX = centerX + (length * cos(radians));
  int endY = centerY + (length * sin(radians));

  // Draw the needle onto your sprite canvas
  gaugeSpr.drawLine(centerX, centerY, endX, endY, color);

  // Optional: Add a small center hub cap to make it look clean
  gaugeSpr.fillCircle(centerX, centerY, 4, color);
}


/**
 * Draws a complete custom gauge meter on a dedicated sprite workspace.
 * @param val     The current value to display (where the needle points)
 * @param minVal  The minimum value on the dial (e.g., 0)
 * @param maxVal  The maximum value on the dial (e.g., 100)
 * @param label   Optional unit string (e.g., "°C" or "RPM")
 */
void drawCustomGauge(float val, float minVal, float maxVal, const char* label) {
  // 1. Initialize/Clear the Sprite workspace (Size: 150x150 pixels)
  gaugeMeter.fillSprite(TFT_BLACK);

  int cx = CUSTOM_DIAMETER / 2;      // Center X of the gauge box
  int cy = CUSTOM_DIAMETER / 2;      // Center Y of the gauge box
  int r = CUSTOM_DIAMETER / 2 - 10;  // Outer radius of the gauge scale ring, add margin

  // Define angles: 0 degrees is straight right in math, so:
  // 135 deg = bottom-left start, 405 deg = bottom-right end (total 270 deg arc)
  int startAngle = 135;
  int endAngle = 405;

  // 2. Draw the Scale Ticks (Lines radiating outwards)
  int totalTicks = 21;  // Adjust for more or fewer subdivisions
  for (int i = 0; i < totalTicks; i++) {
    // Map current tick index to angle space
    float angle = startAngle + (i * (endAngle - startAngle) / (totalTicks - 1));
    float rad = angle * 3.14159 / 180.0;

    // Set tick mark length (major ticks every 5 steps are longer)
    int tickLen = (i % 5 == 0) ? 8 : 4;

    // Calculate start and end coordinates for the line segment
    int x0 = cx + ((r - tickLen) * cos(rad));
    int y0 = cy + ((r - tickLen) * sin(rad));
    int x1 = cx + (r * cos(rad));
    int y1 = cy + (r * sin(rad));

    // Color code scales like the image (Blue zone -> Gray -> Red zone)
    uint16_t tickColor = 0xBDF7;     // Default light gray
    if (i < 4) tickColor = 0x03FF;   // Cold Blue zone
    if (i > 16) tickColor = 0xF800;  // Hot Red zone

    gaugeMeter.drawLine(x0, y0, x1, y1, tickColor);
  }

  // 3. Draw Number Text Labels (5 major subdivisions)
  gaugeMeter.setTextColor(TFT_WHITE);
  gaugeMeter.setTextSize(1);  // Standard small text or use a small FreeFont

  int totalLabels = 6;  // 0, 20, 40, 60, 80, 100 style layout
  for (int i = 0; i < totalLabels; i++) {
    float angle = startAngle + (i * (endAngle - startAngle) / (totalLabels - 1));
    float rad = angle * 3.14159 / 180.0;

    // Calculate text values dynamically based on user-defined min/max bounds
    int displayVal = minVal + (i * (maxVal - minVal) / (totalLabels - 1));

    // Place text slightly inward from the ticks
    int textX = cx + ((r - 18) * cos(rad));
    int textY = cy + ((r - 18) * sin(rad)) - 4;  // Shift up slightly to vertically center text

    gaugeMeter.drawCentreString(String(displayVal), textX, textY, 1);
  }

  // 4. Calculate and Draw the Indicator Needle
  // Constrain input value safely inside user limits
  val = constrain(val, minVal, maxVal);

  // Map value to the matching rotation angle
  float needleAngle = startAngle + ((val - minVal) * (endAngle - startAngle) / (maxVal - minVal));
  float needleRad = needleAngle * 3.14159 / 180.0;

  int needleLength = CUSTOM_DIAMETER / 2 - 30;  // 48;
  int nx = cx + (needleLength * cos(needleRad));
  int ny = cy + (needleLength * sin(needleRad));

  // Draw smooth thicker needle using drawWideLine
  //gaugeMeter.drawWideLine(cx, cy, nx, ny, 2, TFT_WHITE, TFT_WHITE);
  gaugeMeter.drawWideLine(cx, cy, nx, ny, 2, TFT_YELLOW, TFT_YELLOW);

  // 5. Center Pivot Cap (The white dot at the center of the needle)
  gaugeMeter.fillCircle(cx, cy, 6, TFT_YELLOW);
  gaugeMeter.drawCircle(cx, cy, 6, TFT_YELLOW);
  //gaugeMeter.fillCircle(cx, cy, 6, TFT_WHITE);
  //gaugeMeter.drawCircle(cx, cy, 6, TFT_DARKGREY);

  // 6. Draw Digital Value Readout at the bottom
  gaugeMeter.setTextColor(0xFFFF);
  //String valStr = String(val, 2) + label; // e.g., "40.34°C"
  //gaugeMeter.drawCentreString(valStr, cx, cy + 45, 2);
  String valStr = String(val, 1);  // e.g., "40.34°C"
  gaugeMeter.drawCentreString(valStr, cx, cy + 20, 2);
  //String valStr = String(val, 2) + label; // e.g., "40.34°C"
  gaugeMeter.drawCentreString(label, cx, cy + 40, 2);
}

/**
 * Draws a classic analog panel meter.
 * @param val     The current value to display (e.g., 0 to 15)
 * @param minVal  The minimum scale value (0)
 * @param maxVal  The maximum scale value (15)
 * @param label   The central unit character (e.g., "A" or "V")
 */
void drawClassicGauge(float val, float minVal, float maxVal, const char* label) {
  // 1. Clear the sprite canvas with a clean white background
  //classGauge.fillSprite(TFT_BLACK);  //TFT_WHITE);

  // 2. Map coordinates directly to a widescreen layout configuration
  int cx = PANEL_WIDTH / 2;          // Centered horizontally
  int cy = PANEL_HEIGHT * 0.9;       //+ (PANEL_WIDTH / 45);  // Drop center point *just* below the floor line
  int r = (PANEL_WIDTH * 45) / 100;  // Boosted radius to 45% of total width (90% of half-width) to fill edges

  // True classic wide sweeping arc limit angles
  int startAngle = 218;
  int endAngle = 322;

  // 3. Draw Frame Border Outline
  //classGauge.drawRect(0, 0, PANEL_WIDTH, PANEL_HEIGHT, TFT_SILVER);
  //classGauge.drawRect(1, 1, PANEL_WIDTH - 2, PANEL_HEIGHT - 2, 0x7BEF);
  classGauge.fillRoundRect(0, 0, PANEL_WIDTH, PANEL_HEIGHT, 4, TFT_SILVER);
  //classGauge.drawRoundRect(0, 0, PANEL_WIDTH, PANEL_HEIGHT, 4, TFT_BLACK);
  classGauge.drawRoundRect(1, 1, PANEL_WIDTH - 2, PANEL_HEIGHT - 2, 4, TFT_BLACK);
  classGauge.drawRoundRect(2, 2, PANEL_WIDTH - 4, PANEL_HEIGHT - 4, 4, TFT_BLACK);
  classGauge.drawRoundRect(4, 4, PANEL_WIDTH - 8, PANEL_HEIGHT - 8, 4, TFT_BLACK);

  // 4. Draw the Continuous Curved Scale Background Arc Line
#define USE_DRAW_ARC
#ifndef USE_DRAW_ARC
  for (int a = startAngle; a < endAngle; a++) {
    float rad1 = a * 3.14159 / 180.0;
    float rad2 = (a + 1) * 3.14159 / 180.0;
    classGauge.drawLine(cx + (r * cos(rad1)), cy + (r * sin(rad1)),
                        cx + (r * cos(rad2)), cy + (r * sin(rad2)), TFT_BLACK);
  }
#else
  classGauge.drawSmoothArc(cx, cy, r + 1, r + 1, startAngle - 90, endAngle - 90, TFT_BLACK, TFT_SILVER);
#endif

  // 5. Draw All Scale Ticks (31 marks total creates perfect increments of 0.5)
  int totalTicks = 31;
  int tickWidth = 1;
  for (int i = 0; i < totalTicks; i++) {
    float angle = startAngle + (i * (endAngle - startAngle) / (float)(totalTicks - 1));
    float rad = angle * 3.14159 / 180.0;

    // Scale tick mark lines based on frame depth factors
    int tickLen = PANEL_WIDTH / 60;
    if (tickLen < 3) tickLen = 3;
    tickWidth = 1;
    if (i % 10 == 0) {
      tickLen = PANEL_WIDTH / 16;  // Major markers (0, 5, 10, 15)
      tickWidth = 3;
    } else if (i % 5 == 0) {
      tickLen = PANEL_WIDTH / 24;  // Halfway marker points
      tickWidth = 2;
    }

    // Ticks extend inward from the main arc line boundary path
    int x0 = cx + (r * cos(rad));
    int y0 = cy + (r * sin(rad));
    int x1 = cx + ((r - tickLen) * cos(rad));
    int y1 = cy + ((r - tickLen) * sin(rad));

    //classGauge.drawLine(x0, y0, x1, y1, TFT_BLACK);
    classGauge.drawWideLine(x0, y0, x1, y1, tickWidth, TFT_BLACK);
  }

  // 6. Draw the 4 Major Scale Numbers (0, 5, 10, 15)
  classGauge.setTextColor(TFT_BLACK);
  int numLabels = 4;

  for (int i = 0; i < numLabels; i++) {
    float angle = startAngle + (i * (endAngle - startAngle) / (float)(numLabels - 1));
    float rad = angle * 3.14159 / 180.0;
    int displayVal = minVal + (i * (maxVal - minVal) / (numLabels - 1));

    // Place text numbers outside the arc line loop space
    int offsetDist = PANEL_WIDTH / 26;
    int textX = cx + ((r + offsetDist) * cos(rad));
    int textY = cy + ((r + offsetDist) * sin(rad)) - 4;

    // Tight boundary tuning for 2:1 widescreen ratio limits
    if (i == 0) {
      textX += 0;  //6;
      textY += 5;
    }
    if (i == (numLabels - 1)) {
      textX -= -4;  //6;
      textY += 5;
    }

    classGauge.drawCentreString(String(displayVal), textX, textY, 1);
  }

  // 7. Draw Central Unit Label Symbol ("A") Centered below Arc Crest
  //classGauge.setTextColor(TFT_BLACK);
  classGauge.setTextColor(TFT_BLUE);  //TFT_BLACK);
  int labelFont = (PANEL_WIDTH > 140) ? 4 : 2;

  // Calculate the absolute peak vertical height of the main arc curve line
  int arcCrestY = cy - r;

  // Position the text label exactly a balanced fraction below the arc line peak
  int labelY = PANEL_HEIGHT * 5 / 8;  //arcCrestY + (PANEL_HEIGHT / 4.5);
  classGauge.drawCentreString(label, cx, labelY, labelFont);
  /*
  // Position the stylistic underline offset directly beneath the new label position
  int lineOffset = PANEL_WIDTH / 25;
  int textUnderlineY = labelY + (PANEL_HEIGHT / 7);
  classGauge.drawLine(cx - lineOffset, textUnderlineY, cx + lineOffset, textUnderlineY, 0xfcca);  //TFT_BLACK);
*/



#ifdef POLYGON_NEEDLE
  // =========================================================================
  // 8. IMPROVED: Draw Tapered Pointer Needle with Counterweight
  // =========================================================================
  val = constrain(val, minVal, maxVal);
  float needleAngle = startAngle + ((val - minVal) * (endAngle - startAngle) / (maxVal - minVal));

  // Radians for the main pointer direction and its perpendicular thickness vectors
  float rad = needleAngle * 3.14159 / 180.0;
  float radLeft = (needleAngle - 90) * 3.14159 / 180.0;
  float radRight = (needleAngle + 90) * 3.14159 / 180.0;

  // Length dimensions scaled to your radius
  int tipLength = r - 2;              // Reaches out near the tick marks
  int tailLength = PANEL_WIDTH / 15;  // The counterweight tail extending backwards

  // Thickness dimensions
  float baseThickness = PANEL_WIDTH / 75.0;  // Thickness at the center pivot hub point
  if (baseThickness < 2.5) baseThickness = 2.5;

  // Calculate the 4 coordinate positions
  // Tip Point
  int tx = cx + (tipLength * cos(rad));
  int ty = cy + (tipLength * sin(rad));

  // Base Left Point
  int blx = cx + (baseThickness * cos(radLeft));
  int bly = cy + (baseThickness * sin(radLeft));

  // Base Right Point
  int brx = cx + (baseThickness * cos(radRight));
  int bry = cy + (baseThickness * sin(radRight));

  // Tail Point
  int lx = cx - (tailLength * cos(rad));
  int ly = cy - (tailLength * sin(rad));

  // Render the needle by drawing two solid triangles side-by-side
  // Triangle 1: Left side of the needle body (Tip to Left Base to Tail)
  classGauge.fillTriangle(tx, ty, blx, bly, lx, ly, TFT_RED);

  // Triangle 2: Right side of the needle body (Tip to Right Base to Tail)
  classGauge.fillTriangle(tx, ty, brx, bry, lx, ly, TFT_RED);

  // Smooth out the back-tail tip by adding a small matching round cap accent
  int tailCapRadius = baseThickness * 0.8;
  if (tailCapRadius < 2) tailCapRadius = 2;
  classGauge.fillCircle(lx, ly, tailCapRadius, TFT_RED);

  // =========================================================================
  // 9. Pivot Hub accent circle overlay at the base
  // =========================================================================
  int hubRadius = PANEL_WIDTH / 15;
  classGauge.fillCircle(cx, cy, hubRadius, 0x31A6);
  classGauge.fillCircle(cx, cy, hubRadius - 2, TFT_BLACK);
  classGauge.fillCircle(cx, cy, 2, TFT_SILVER);  // Small center pin head accent dot


#else
  // 8. Calculate and Draw Needle Pointer
  val = constrain(val, minVal, maxVal);
  float needleAngle = startAngle + ((val - minVal) * (endAngle - startAngle) / (maxVal - minVal));
  float needleRad = needleAngle * 3.14159 / 180.0;

  // The needle source pivot origin starts off-screen and sweeps upward smoothly
  int needleStartR = PANEL_WIDTH / 12;
  int needleEndR = r - 2;

  int nx0 = cx + (needleStartR * cos(needleRad));
  int ny0 = cy + (needleStartR * sin(needleRad));
  int nx1 = cx + (needleEndR * cos(needleRad));
  int ny1 = cy + (needleEndR * sin(needleRad));

  //classGauge.drawLine(nx0, ny0, nx1, ny1, TFT_RED);
  //classGauge.drawWideLine(nx0, ny0, nx1, ny1, 2, TFT_RED, TFT_RED);
  classGauge.drawWedgeLine(nx0, ny0, nx1, ny1, 4, 1, TFT_RED, TFT_RED);
#endif
}