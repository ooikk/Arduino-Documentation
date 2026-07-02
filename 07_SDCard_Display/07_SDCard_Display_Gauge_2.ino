// Define the SD Chip Select pin (must match wiring)
#define VSPI_PIN   // SD card is using VSPI and TFT is using HSPI

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
TFT_eSprite gauge = TFT_eSprite(&tft); // Canvas buffer to prevent flicker
// ==========================================
// USER CONFIGURATION (EDIT THESE VALUES)
// ==========================================
const float RANGE_MIN   = 0.0;     // Minimum gauge value
const float RANGE_MAX   = 100.0;   // Maximum gauge value
const char* GAUGE_TITLE = "BATTERY"; // Gauge Header text
const char* UNIT_TEXT   = "V";     // Unit of measurement

// Fresh Color Palette (Modern Mint, Crisp Teal, Slate Gray)
#define COLOR_BG       0x18C3      // Dark charcoal background (HEX #14181F)
#define COLOR_CARD     0x2126      // Soft slate container card (HEX #20242D)
#define COLOR_MINT     0x5FFF      // Fresh energetic mint green (HEX #50FFDF)
#define COLOR_DARK_MINT 0x0C2A     // Deep mint glow shadow (HEX #0A5548)
#define COLOR_TEXT_MUTED 0x94B2    // Subdued gray text (HEX #929AA6)
#define COLOR_WHITE    0xFFFF      // High contrast crisp white

// Layout Math Anchors (Do not change)
const int G_SIZE       = 240;      // Square dimension of the gauge canvas
const int CENTER       = 120;      // Rotation center anchor point (X and Y)
const int RADIUS       = 105;      // Outer arc radius
const float START_ANG  = 135.0;    // Gauge start angle (bottom-left)
const float END_ANG    = 405.0;    // Gauge end angle (bottom-right)

float currentValue = RANGE_MIN;
float targetValue = RANGE_MIN;

void simulateSensor();





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

  tft.setRotation(1);              // Landscape orientation
  tft.fillScreen(COLOR_BG);
  
  // Allocate memory for the 240x240 buffer (using ~115KB RAM)
  gauge.createSprite(G_SIZE, G_SIZE);
  gauge.setPivot(CENTER, CENTER);  // Establish the needle pivot matrix anchor


  // Explicitly bind SPI to your SPI pins BEFORE SD.begin()
  // SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN);
  sdSPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN);  // SCK, MISO, MOSI

  Serial.println("Initializing SD card...");
  //if (!SD.begin(SD_CS_PIN, SPI, SD_FREQUENCY)) {
  if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {    
    Serial.println("SD Card initialization failed!");
    while (1) delay(1000);  // Halt
  }
  
  Serial.println("SD Card initialized successfully.");
  //tft.println("SD Card OK");

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  //tft.printf("Size: %lluMB", cardSize);

}

void loop() {
  // Simulate sensor data changing over time
  simulateSensor();
  
  // Smooth out needle physics using linear interpolation (lerp)
  currentValue += (targetValue - currentValue) * 0.15;
  
  // 1. Reset canvas with container background color
  gauge.fillSprite(COLOR_CARD);
  
  // 2. Map current reading to angle layout
  float progressPct = (currentValue - RANGE_MIN) / (RANGE_MAX - RANGE_MIN);
  progressPct = constrain(progressPct, 0.0, 1.0);
  float currentAngle = START_ANG + (progressPct * (END_ANG - START_ANG));
  
  // 3. Render modern track & active arc ring
  drawGaugeTrack(progressPct);
  
  // 4. Render sleek modern needle 
  drawModernNeedle(currentAngle);
  
  // 5. Print typography (Title and Digital Value)
  drawTypography();
  
  // 6. Push fully assembled buffer onto screen (Offset to middle of screen)
  gauge.pushSprite((tft.width() - G_SIZE) / 2, (tft.height() - G_SIZE) / 2);
  
  delay(16); // Target ~60FPS rendering loop
}


void drawGaugeTrack(float pct) {
  // Background static guide track ring
  gauge.drawSmoothArc(CENTER, CENTER, RADIUS, RADIUS - 10, START_ANG, END_ANG, COLOR_BG, COLOR_CARD);
  
  // Dynamic active mint progress ring overlay
  if (pct > 0.01) {
    float activeEnd = START_ANG + (pct * (END_ANG - START_ANG));
    gauge.drawSmoothArc(CENTER, CENTER, RADIUS, RADIUS - 10, START_ANG, activeEnd, COLOR_MINT, COLOR_CARD);
  }
}

void drawModernNeedle(float angle) {
  // Compute floating math coordinates for sharp needle point
  float rad = (angle - 90.0) * DEG_TO_RAD; // Shift coordinate plane orientation
  
  // Modern aesthetic: floating line starting away from center extending past arc
  int needleStartDist = 45;
  int needleEndDist = RADIUS + 4;
  
  int sx = CENTER + cos(rad) * needleStartDist;
  int sy = CENTER + sin(rad) * needleStartDist;
  int ex = CENTER + cos(rad) * needleEndDist;
  int ey = CENTER + sin(rad) * needleEndDist;
  
  // Generate crisp structural line vector 
  gauge.drawWedgeLine(sx, sy, ex, ey, 3, 1, COLOR_MINT);
}

void drawTypography() {
  // Title Metadata Header
  gauge.setTextDatum(MC_DATUM); 
  gauge.setTextColor(COLOR_TEXT_MUTED, COLOR_CARD);
  gauge.drawString(GAUGE_TITLE, CENTER, CENTER - 45, 2); 
  
  // Clean digital output layout value 
  gauge.setTextColor(COLOR_WHITE, COLOR_CARD);
  char valBuffer[10];
  dtostrf(currentValue, 4, 1, valBuffer); // Format string to 1 decimal place
  gauge.drawString(valBuffer, CENTER, CENTER + 30, 6); // Large font index 6
  
  // Unit Metadata Footer
  gauge.setTextColor(COLOR_TEXT_MUTED, COLOR_CARD);
  gauge.drawString(UNIT_TEXT, CENTER, CENTER + 65, 2);
}

void simulateSensor() {
  // Random walk sweep simulator logic
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 3000) {
    lastUpdate = millis();
    targetValue = random(RANGE_MIN * 10, RANGE_MAX * 10) / 10.0;
  }
}

