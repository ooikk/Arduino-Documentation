/*
 * ESP32-S3 Keypad on ILI9488 3.5" TFT (480x320)
 * --------------------------------------------------------
 * Features:
 *
 *  - Uses TFT_eSPI native touch with exact calibration data
 *
 * Library: TFT_eSPI (No separate XPT2046 library needed)
 */

#include <TFT_eSPI.h>
#include <SPI.h>
#include <math.h>  // For isnan()

// ============================================================
// 1. CONFIGURATION
// ============================================================

#define SCREEN_W 480
#define SCREEN_H 320

#define DISPLAY_H 70
#define PAD_TOP DISPLAY_H + 10
#define SHRINK 0.92

#define TOUCHPAD 4
#define PAD_W (SCREEN_W - (TOUCHPAD + 1) * PAD_GAP) / TOUCHPAD
#define PAD_H PAD_W / 2
#define PAD_GAP 6

#define TOUCHICON 6
#define ICON_W PAD_H
#define ICON_H PAD_H
#define ICON_GAP 6
#define ICON_MARGIN ICON_W * 0.1

#define BG_COLOR 0x1082
#define DISPLAY_BG 0x0000
#define DISPLAY_FG 0xFFE0
#define PAD_BASE_BG 0x2965
#define ICON_BASE_BG 0xFBC0
#define OUTLINE_COLOR 0x858f  //0xFFFF
#define KEY_TEXT_COLOR 0xFFFF
#define KEY_PRESSED_BG 0x07E0

#define KEY_FONT 4
#define DISPLAY_FONT 4  // Changed from 6 to 2 (full ASCII support)

const char* label1[TOUCHPAD] = { "Yes", "No", "ENTER", "CANCEL" };

enum Direction {
  DIR_UP,
  DIR_DOWN,
  DIR_LEFT,
  DIR_RIGHT,
  CIRCLE,
  GOOGLE
};

// Define the array of directions
Direction label2[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT, CIRCLE, GOOGLE };

#define Google_TRANSPARENT_COLOR 0xffff
#include "google1.h"


// ============================================================
// 2. GLOBAL OBJECTS
// ============================================================
TFT_eSPI tft = TFT_eSPI();

// Your exact calibration data from Touch_calibrate.ino
// Format: { x_min, x_max, y_min, y_max, rotation }
uint16_t calData[5] = { 263, 3627, 233, 3513, 7 };

// ============================================================
// 3. BUTTON MODEL
// ============================================================
struct ButtonPad {
  uint16_t x, y, w, h;
  const char* label;
  uint16_t baseColor;
  bool pressed;

  void draw(TFT_eSPI& t) const {
    float scale = pressed ? SHRINK : 1.0f;
    uint16_t dw = (uint16_t)(w * scale);
    uint16_t dh = (uint16_t)(h * scale);
    uint16_t dx = x + (w - dw) / 2;
    uint16_t dy = y + (h - dh) / 2;

    uint16_t fill = pressed ? KEY_PRESSED_BG : baseColor;

    t.fillRect(x, y, w, h, BG_COLOR);
    t.fillRoundRect(dx, dy, dw, dh, 8, fill);
    t.drawRoundRect(dx, dy, dw, dh, 8, OUTLINE_COLOR);

    t.setTextDatum(MC_DATUM);
    t.setTextColor(KEY_TEXT_COLOR, fill);
    t.setTextFont(KEY_FONT);
    t.drawString(label, dx + dw / 2, dy + dh / 2);
  }

  bool contains(uint16_t px, uint16_t py) const {
    return px >= x && px < x + w && py >= y && py < y + h;
  }
};

struct ButtonIcon {
  uint16_t x, y, w, h;
  Direction label;
  uint16_t baseColor;
  bool pressed;

  void draw(TFT_eSPI& t) const {
    float scale = pressed ? SHRINK : 1.0f;
    uint16_t dw = (uint16_t)(w * scale);
    uint16_t dh = (uint16_t)(h * scale);
    uint16_t dx = x + (w - dw) / 2;
    uint16_t dy = y + (h - dh) / 2;

    uint16_t fill = pressed ? KEY_PRESSED_BG : baseColor;

    t.fillRect(x, y, w, h, BG_COLOR);
    //t.fillRoundRect(dx, dy, dw, dh, 8, fill);
    t.drawRoundRect(dx, dy, dw, dh, 8, OUTLINE_COLOR);
    dx += ICON_MARGIN;
    dy += ICON_MARGIN;
    dw -= ICON_MARGIN * 2;
    dh -= ICON_MARGIN * 2;

    // uint32_t color = KEY_TEXT;
    // fillTriangle(int32_t x1,int32_t y1, int32_t x2,int32_t y2, int32_t x3,int32_t y3, uint32_t color);
    // drawTriangle(int32_t x1,int32_t y1, int32_t x2,int32_t y2, int32_t x3,int32_t y3, uint32_t color);
    // fillCircle(int32_t x, int32_t y, int32_t r, uint32_t color);
    // drawCircle(int32_t x, int32_t y, int32_t r, uint32_t color);
    switch (label) {
      case DIR_UP:
        t.fillTriangle(dx, dy + dh, dx + dw, dy + dh, dx + dw / 2, dy, fill);
        t.drawTriangle(dx, dy + dh, dx + dw, dy + dh, dx + dw / 2, dy, OUTLINE_COLOR);
        break;
      case DIR_DOWN:
        t.fillTriangle(dx, dy, dx + dw, dy, dx + dw / 2, dy + dh, fill);
        t.drawTriangle(dx, dy, dx + dw, dy, dx + dw / 2, dy + dh, OUTLINE_COLOR);
        break;
      case DIR_LEFT:
        t.fillTriangle(dx + dw, dy, dx + dw, dy + dh, dx, dy + dh / 2, fill);
        t.drawTriangle(dx + dw, dy, dx + dw, dy + dh, dx, dy + dh / 2, OUTLINE_COLOR);
        break;
      case DIR_RIGHT:
        t.fillTriangle(dx, dy, dx, dy + dh, dx + dw, dy + dh / 2, fill);
        t.drawTriangle(dx, dy, dx, dy + dh, dx + dw, dy + dh / 2, OUTLINE_COLOR);
        break;
      case CIRCLE:
        t.fillCircle(dx + dw / 2, dy + dh / 2, dw / 2, fill);
        t.drawCircle(dx + dw / 2, dy + dh / 2, dw / 2, OUTLINE_COLOR);
        break;
      case GOOGLE:
        t.setSwapBytes(true);
        if (pressed) {
          t.fillRect(dx, dy, dw, dh, fill);

          // 1. Allocate memory safely (assuming dw and dh are your destination width/height)
          uint16_t* data = new (std::nothrow) uint16_t[dw * dh];
          if (data == nullptr) {
            Serial.println("Error: Out of memory for image scaling!");
            return;  // Exit safely
          }
          // 2. Nearest-neighbour scaling (copy pixels)
          for (int y1 = 0; y1 < dh; y1++) {
            for (int x1 = 0; x1 < dw; x1++) {
              // Calculate source coordinates
              int srcX = (x1 * GOOGLE_WIDTH) / dw;
              int srcY = (y1 * GOOGLE_HEIGHT) / dh;
              // Calculate 1D index for the destination array
              int destIndex = (y1 * dw) + x1;
              // Calculate 1D index for the source array and read from PROGMEM
              int srcIndex = (srcY * GOOGLE_WIDTH) + srcX;
              // CORRECT: Use 1D indexing AND pgm_read_word - read from program memory
              data[destIndex] = pgm_read_word(&google[srcIndex]);
            }
          }
          // 3. Draw the scaled bitmap
          t.pushImage(dx, dy, dw, dh, data, Google_TRANSPARENT_COLOR);
          // 4. Release the memory when done
          delete[] data;
          data = nullptr;
        } else
          t.pushImage(dx, dy, GOOGLE_WIDTH, GOOGLE_HEIGHT, google, Google_TRANSPARENT_COLOR);  // Draw a small bitmap
        break;
    }
  }
  bool contains(uint16_t px, uint16_t py) const {
    return px >= x && px < x + w && py >= y && py < y + h;
  }
};

ButtonPad buttonPad[TOUCHPAD];
ButtonIcon buttonIcon[TOUCHICON];


// ============================================================
// 4. DISPLAY AREA & MATH EVALUATOR
// ============================================================
String inputBuffer = "";
//const char* lbl = "";

void drawDisplay() {
  //tft.fillRect(0, 0,  SCREEN_W,  DISPLAY_H,  DISPLAY_BG);
  tft.fillRectVGradient(0, 0, SCREEN_W, DISPLAY_H, DISPLAY_BG, 0x71f8);
  tft.drawRect(0, 0, SCREEN_W, DISPLAY_H, OUTLINE_COLOR);
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(DISPLAY_FG, DISPLAY_BG);
  uint8_t font = DISPLAY_FONT;
  tft.setTextFont(font);
  tft.drawString(inputBuffer, SCREEN_W - 15, DISPLAY_H / 2);
}

// ============================================================
// 5. TOUCH HANDLING
// ============================================================
int8_t lastP = -1, lastI = -1;

void handleTouch() {
  uint16_t x = 0, y = 0;

  // tft.getTouch returns true if touched, and populates x, y
  // with ALREADY CALIBRATED screen coordinates (0 to 479, 0 to 319).
  // No manual map() needed! It also uses the IRQ pin automatically if defined in User_Setup.h
  //if (!tft.getTouch(&x, &y)) return;

  if (!tft.getTouch(&x, &y)) {
    // --- Release detection ---
    if (lastP >= 0) {
      buttonPad[lastP].pressed = false;
      buttonPad[lastP].draw(tft);
      lastP = -1;
    } else if (lastI >= 0) {
      buttonIcon[lastI].pressed = false;
      buttonIcon[lastI].draw(tft);
      lastI = -1;
    }
    return;
  }

  int8_t pHit = -1;
  for (uint16_t r = 0; r < TOUCHPAD; ++r) {
    if (buttonPad[r].contains(x, y)) {
      pHit = r;
    }
  }
  int8_t iHit = -1;
  if (pHit == -1) {
    for (uint16_t r = 0; r < TOUCHICON; ++r) {
      if (buttonIcon[r].contains(x, y)) {
        iHit = r;
      }
    }
  }

  // --- Press event ---
  if (pHit >= 0 && (pHit != lastP)) {
    // new press event
    if (lastP >= 0) {
      buttonPad[lastP].pressed = false;
      buttonPad[lastP].draw(tft);
    }

    buttonPad[pHit].pressed = true;
    buttonPad[pHit].draw(tft);
    lastP = pHit;
    inputBuffer = "x: " + String(x) + " y: " + String(y) + "  ";
    //snprintf(inputBuffer, sizeof(inputBuffer), "x: %d y: %d", x, y);
    inputBuffer += buttonPad[pHit].label;
    drawDisplay();
  }

  if (iHit >= 0 && !(iHit == lastI)) {
    // new press event
    if (lastI >= 0) {
      buttonIcon[lastI].pressed = false;
      buttonIcon[lastI].draw(tft);
    }

    buttonIcon[iHit].pressed = true;
    buttonIcon[iHit].draw(tft);
    lastI = iHit;
    inputBuffer = "x: " + String(x) + " y: " + String(y) + "  ";
    // --- Logic Execution ---
    switch ((Direction)buttonIcon[iHit].label) {
      case DIR_UP:
        inputBuffer += "Direction Up";
        break;
      case DIR_DOWN:
        inputBuffer += "Direction Down";
        break;
      case DIR_LEFT:
        inputBuffer += "Direction Left";
        break;
      case DIR_RIGHT:
        inputBuffer += "Direction Right";
        break;
      case CIRCLE:
        inputBuffer += "Direction Circle";
        break;
      case GOOGLE:
        inputBuffer += "Google Icon";
        break;
    }
    drawDisplay();
  }
}


// ============================================================
// 6. SETUP
// ============================================================
void setup() {

  Serial.begin(115200);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(BG_COLOR);

  // Apply the calibration data to TFT_eSPI's native touch handler
  tft.setTouch(calData);

  // Build buttons:
  for (uint16_t r = 0; r < TOUCHPAD; ++r) {
    uint16_t x = PAD_GAP + r * (PAD_W + PAD_GAP);
    uint16_t y = PAD_TOP + PAD_GAP;  //+ r * (KEY_H + KEY_GAP);

    const char* lbl = label1[r];
    buttonPad[r] = { x, y, PAD_W, PAD_H, lbl, PAD_BASE_BG, false };
    buttonPad[r].draw(tft);
  }

  for (uint16_t r = 0; r < TOUCHICON; ++r) {
    uint16_t x = ICON_GAP + r * (ICON_W + ICON_GAP);
    uint16_t y = PAD_TOP + ICON_GAP * 4 + PAD_H;

    Direction lbl = label2[r];
    buttonIcon[r] = { x, y, ICON_W, ICON_H, lbl, ICON_BASE_BG, false };
    buttonIcon[r].draw(tft);
  }
  inputBuffer = "Ready to test the touch pad.";
  drawDisplay();
  //Serial.printf("Icon width: %d\n", ICON_W);
  Serial.println("Touch Pad ready with calibrated touch.");
}


// ============================================================
// 7. LOOP
// ============================================================
void loop() {
  handleTouch();
  delay(15);  // Minor debounce
}
