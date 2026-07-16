/*
 * ESP32-S3 Calculator Keypad on ILI9488 3.5" TFT (480x320)
 * --------------------------------------------------------
 * Features:
 *  - 4x4 calculator keypad (0-9, +, -, *, /, =, C)
 *  - Visual "haptic" feedback: color change + shrink on press
 *  - Live display of pressed keys; 'C' clears, '=' calculates
 *  - Uses TFT_eSPI native touch with exact calibration data
 *
 * Library: TFT_eSPI (No separate XPT2046 library needed)
 */

#include <TFT_eSPI.h>
#include <SPI.h>
#include <math.h>  // For isnan()

#define USE_FREEFONT


// ============================================================
// 1. CONFIGURATION
// ============================================================
namespace Config {
constexpr uint16_t SCREEN_W = 480;
constexpr uint16_t SCREEN_H = 320;

constexpr uint16_t COLS = 4;
constexpr uint16_t ROWS = 4;
constexpr uint16_t KEY_GAP = 6;
constexpr uint16_t DISPLAY_H = 70;
constexpr uint16_t KEYPAD_TOP = DISPLAY_H + 10;
constexpr uint16_t KEY_W = (SCREEN_W - (COLS + 1) * KEY_GAP) / COLS;
constexpr uint16_t KEY_H = (SCREEN_H - KEYPAD_TOP - (ROWS + 1) * KEY_GAP) / ROWS;

constexpr float SHRINK = 0.92f;

constexpr uint16_t BG_COLOR = 0x1082;
constexpr uint16_t DISPLAY_BG = 0x0000;
constexpr uint16_t DISPLAY_FG = 0xFFE0;
constexpr uint16_t KEY_NUM_BG = 0x2965;
constexpr uint16_t KEY_OP_BG = 0xFBC0;
constexpr uint16_t KEY_CLR_BG = 0xF800;
constexpr uint16_t KEY_TEXT = 0xFFFF;
constexpr uint16_t KEY_PRESSED_BG = 0x07E0;

constexpr uint8_t FONT_KEY = 4;
#ifdef USE_FREEFONT
// For display, use a FreeFont
constexpr const GFXfont* FONT_DISPLAY = &FreeSansBold18pt7b;  // 12, 18, 24
//constexpr const FONT_DISPLAY = &FreeSansBold12pt7b;
#else
constexpr uint8_t FONT_DISPLAY = 4;  // Changed from 6 to 2 (full ASCII support)
#endif
}

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
struct Button {
  uint16_t x, y, w, h;
  const char* label;
  uint16_t baseColor;
  bool pressed;

  void draw(TFT_eSPI& t) const {
    float scale = pressed ? Config::SHRINK : 1.0f;
    uint16_t dw = (uint16_t)(w * scale);
    uint16_t dh = (uint16_t)(h * scale);
    uint16_t dx = x + (w - dw) / 2;
    uint16_t dy = y + (h - dh) / 2;

    uint16_t fill = pressed ? Config::KEY_PRESSED_BG : baseColor;

    t.fillRect(x, y, w, h, Config::BG_COLOR);
    t.fillRoundRect(dx, dy, dw, dh, 8, fill);
    t.drawRoundRect(dx, dy, dw, dh, 8, Config::KEY_TEXT);

    t.setTextDatum(MC_DATUM);
    t.setTextColor(Config::KEY_TEXT, fill);
    t.setTextFont(Config::FONT_KEY);
    t.drawString(label, dx + dw / 2, dy + dh / 2);
  }

  bool contains(uint16_t px, uint16_t py) const {
    return px >= x && px < x + w && py >= y && py < y + h;
  }
};

const char* LABELS[Config::ROWS][Config::COLS] = {
  { "7", "8", "9", "/" },
  { "4", "5", "6", "*" },
  { "1", "2", "3", "-" },
  { "C", "0", "=", "+" }
};

Button buttons[Config::ROWS][Config::COLS];

// ============================================================
// 4. DISPLAY AREA & MATH EVALUATOR
// ============================================================
String inputBuffer = "";
bool justEvaluated = false;

float evaluateExpression(String expr) {
  expr.trim();
  if (expr.length() == 0) return NAN;

  float result = 0;
  float currentVal = 0;
  char currentOp = '+';
  int i = 0;

  while (i < expr.length() && (isdigit(expr[i]) || expr[i] == '.')) i++;
  if (i == 0) return NAN;
  result = expr.substring(0, i).toFloat();

  while (i < expr.length()) {
    currentOp = expr[i];
    if (currentOp != '+' && currentOp != '-' && currentOp != '*' && currentOp != '/') return NAN;
    i++;
    int start = i;
    while (i < expr.length() && (isdigit(expr[i]) || expr[i] == '.')) i++;
    if (i == start) return NAN;
    currentVal = expr.substring(start, i).toFloat();

    if (currentOp == '+') result += currentVal;
    else if (currentOp == '-') result -= currentVal;
    else if (currentOp == '*') result *= currentVal;
    else if (currentOp == '/') {
      if (currentVal == 0) return NAN;
      result /= currentVal;
    }
  }
  return result;
}

void drawDisplay() {
  //tft.fillRect(0, 0, Config::SCREEN_W, Config::DISPLAY_H, Config::DISPLAY_BG);
  tft.fillRectVGradient(0, 0, Config::SCREEN_W, Config::DISPLAY_H, Config::DISPLAY_BG, 0x71f8);
  tft.drawRect(0, 0, Config::SCREEN_W, Config::DISPLAY_H, Config::KEY_TEXT);
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(Config::DISPLAY_FG, Config::DISPLAY_BG);

#ifdef USE_FREEFONT
  tft.setFreeFont(Config::FONT_DISPLAY);  // Use this instead of setTextFont
#else
  uint8_t font = Config::FONT_DISPLAY;

  if (inputBuffer.length() > 12) font = 4;
  if (inputBuffer.length() > 18) font = 2;
  tft.setTextFont(font);
#endif
  tft.drawString(inputBuffer, Config::SCREEN_W - 15, Config::DISPLAY_H / 2);
}


// ============================================================
// 5. SETUP
// ============================================================
void setup() {

  Serial.begin(115200);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(Config::BG_COLOR);

  // Apply the calibration data to TFT_eSPI's native touch handler
  tft.setTouch(calData);

  // Build buttons:
  for (uint16_t r = 0; r < Config::ROWS; ++r) {
    for (uint16_t c = 0; c < Config::COLS; ++c) {
      uint16_t x = Config::KEY_GAP + c * (Config::KEY_W + Config::KEY_GAP);
      uint16_t y = Config::KEYPAD_TOP + Config::KEY_GAP + r * (Config::KEY_H + Config::KEY_GAP);

      const char* lbl = LABELS[r][c];
      uint16_t color;

      if (strcmp(lbl, "C") == 0) color = Config::KEY_CLR_BG;
      else if (strchr("+-*/=", lbl[0])) color = Config::KEY_OP_BG;
      else color = Config::KEY_NUM_BG;
      /*
      // --- ROBUST COLOR ASSIGNMENT ---
      char firstChar = lbl[0]; // Get the first character safely

      if (strcmp(lbl, "C") == 0) {
        color = Config::KEY_CLR_BG;
      }
      // Explicitly check for operator characters. This is 100% reliable.
      else if (firstChar == '+' || firstChar == '-' || firstChar == '*' || firstChar == '/' || firstChar == '=') {
        color = Config::KEY_OP_BG;
      } else {
        color = Config::KEY_NUM_BG;
      }

      // Optional: Debug print to verify colors are assigning correctly
      // Serial.print("Key: "); Serial.print(lbl);
      // Serial.print(" | Char: "); Serial.print(firstChar);
      // Serial.print(" | Color Hex: "); Serial.println(color, HEX);
*/

      buttons[r][c] = { x, y, Config::KEY_W, Config::KEY_H, lbl, color, false };
      buttons[r][c].draw(tft);
    }
  }

  drawDisplay();
  Serial.println("Calculator ready with calibrated touch.");
}

// ============================================================
// 6. TOUCH HANDLING
// ============================================================
int8_t lastR = -1, lastC = -1;

void handleTouch() {
  uint16_t x = 0, y = 0;

  // tft.getTouch returns true if touched, and populates x, y
  // with ALREADY CALIBRATED screen coordinates (0 to 479, 0 to 319).
  // No manual map() needed! It also uses the IRQ pin automatically if defined in User_Setup.h
  //if (!tft.getTouch(&x, &y)) return;

  if (!tft.getTouch(&x, &y)) {
    // --- Release detection ---
    if (lastR >= 0) {
      buttons[lastR][lastC].pressed = false;
      buttons[lastR][lastC].draw(tft);
      lastR = -1;
      lastC = -1;
    }
    return;
  }

  int8_t rHit = -1, cHit = -1;
  for (uint16_t r = 0; r < Config::ROWS; ++r) {
    for (uint16_t c = 0; c < Config::COLS; ++c) {
      if (buttons[r][c].contains(x, y)) {
        rHit = r;
        cHit = c;
      }
    }
  }

  // --- Press event ---
  if (rHit >= 0 && !(rHit == lastR && cHit == lastC)) {
    // new press event
    if (lastR >= 0) {
      buttons[lastR][lastC].pressed = false;
      buttons[lastR][lastC].draw(tft);
    }

    buttons[rHit][cHit].pressed = true;
    buttons[rHit][cHit].draw(tft);
    lastR = rHit;
    lastC = cHit;

    // --- Logic Execution ---
    const char* lbl = buttons[rHit][cHit].label;

    if (strcmp(lbl, "C") == 0) {
      inputBuffer = "";
      justEvaluated = false;
    } else if (strcmp(lbl, "=") == 0) {
      float res = evaluateExpression(inputBuffer);
      if (!isnan(res)) {
        if (res == (long)res) {
          inputBuffer = String((long)res);
        } else {
          inputBuffer = String(res, 6);
          while (inputBuffer.endsWith("0")) inputBuffer.remove(inputBuffer.length() - 1);
          if (inputBuffer.endsWith(".")) inputBuffer.remove(inputBuffer.length() - 1);
        }
      } else {
        inputBuffer = "Error";
      }
      justEvaluated = true;
    } else {
      if (justEvaluated && strchr("0123456789+-/*", lbl[0])) {
        inputBuffer = "";
        justEvaluated = false;
        if (strchr("+-/*", lbl[0])) lbl = "";
      }
      if (inputBuffer.length() < 24) {
        inputBuffer += lbl;
      }
    }

    drawDisplay();
  }

  delay(15);  // Minor debounce

  /*
  // --- Release detection ---
  uint16_t releaseX = 0, releaseY = 0;
  if (!tft.getTouch(&releaseX, &releaseY) && lastR >= 0) {
    buttons[lastR][lastC].pressed = false;
    buttons[lastR][lastC].draw(tft);
    lastR = -1;
    lastC = -1;
  }
*/
}

// ============================================================
// 7. LOOP
// ============================================================
void loop() {
  handleTouch();
}
