#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1); // Set to Landscape (480x320)
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  // 1. Test Shapes and Colors
  testShapes();
  delay(4000);

  // 2. Test Fonts
  testFonts();
  delay(4000);

  // 3. Test Lines and Geometry
  testLines();
  delay(4000);

  // 4. Test Rotations (0, 1, 2, 3)
  testRotations();
  delay(1000);
  
  // Reset to landscape for the next loop
  tft.setRotation(1); 
}

// --- TEST FUNCTIONS ---

void testShapes() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TC_DATUM); // Top-Center alignment
  tft.drawCentreString("Shapes & Colors", 240, 10, 4);

  // Filled Shapes
  tft.fillRect(50, 60, 100, 100, TFT_RED);
  tft.fillCircle(240, 110, 50, TFT_BLUE);
  tft.fillTriangle(350, 160, 400, 60, 450, 160, TFT_GREEN);
  
  // Outlined Shapes
  tft.drawRoundRect(80, 200, 150, 80, 15, TFT_YELLOW);
  tft.fillRoundRect(280, 200, 150, 80, 15, TFT_CYAN);
  
  // Custom RGB565 Colors (Purple and Orange)
  tft.fillCircle(155, 240, 30, 0xF81F); // Purple (Red max, Blue max, some Green)
  tft.fillCircle(355, 240, 30, 0xFD20); // Orange (Red max, half Green)
}

void testFonts() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // White text with black background to prevent overlap
  
  tft.drawCentreString("Font Test", 240, 10, 4);
  
  int y = 50;
  // Loop through built-in fonts 1 through 8
  for (int f = 1; f <= 8; f++) {
    tft.setTextFont(f);
    tft.drawCentreString("Font " + String(f) + " - ESP32-S3", 240, y, f);
    
    // Move Y down based on the height of the current font
    y += tft.fontHeight(f) + 5; 
    
    // Prevent drawing off-screen
    if (y > 310) break; 
  }
}

void testLines() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TC_DATUM);
  tft.drawCentreString("Lines & Geometry", 240, 10, 4);

  int w = tft.width();
  int h = tft.height();

  // Draw lines from top-left and top-right corners to the bottom edge
  for (int i = 0; i < w; i += 15) {
    tft.drawLine(0, 0, i, h, TFT_GREEN);
    tft.drawLine(w, 0, i, h, TFT_RED);
  }
  
  // Draw lines from bottom corners to the top edge
  for (int i = 0; i < h; i += 15) {
    tft.drawLine(0, h, w, i, TFT_BLUE);
    tft.drawLine(w, h, 0, i, TFT_YELLOW);
  }
  
  // Draw a pixel grid pattern in the center
  for (int x = 150; x < 330; x += 10) {
    for (int y = 100; y < 220; y += 10) {
      tft.drawPixel(x, y, TFT_WHITE);
      tft.drawPixel(x+1, y, TFT_WHITE);
      tft.drawPixel(x, y+1, TFT_WHITE);
    }
  }
}

void testRotations() {
  tft.setTextDatum(MC_DATUM); // Middle-Center alignment
  
  for (int r = 0; r < 4; r++) {
    tft.setRotation(r);
    tft.fillScreen(TFT_BLACK);
    
    int cx = tft.width() / 2;
    int cy = tft.height() / 2;
    
    // Draw a border around the screen
    tft.drawRect(0, 0, tft.width(), tft.height(), TFT_WHITE);
    
    // Draw text in the center
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawCentreString("Rotation " + String(r), cx, cy - 20, 4);
    
    // Draw dimensions
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawCentreString(String(tft.width()) + " x " + String(tft.height()), cx, cy + 20, 2);
    
    delay(2000);
  }
}