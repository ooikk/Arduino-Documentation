#ifndef PLOT_GRAPH_H
#define PLOT_GRAPH_H

#include <TFT_eSPI.h>
#include <math.h>

// =============================================================================
//  COMPILE-TIME PLOT CONFIGURATION (modify to suit your needs)
// =============================================================================

// Margins inside each chart's outer rectangle
#ifdef USE_TFT_DISPLAY
#define MARGIN_LEFT 75
#define MARGIN_RIGHT 10
#define MARGIN_TOP 30
#define MARGIN_BOTTOM 50
#else
#define MARGIN_LEFT 40    //50
#define MARGIN_RIGHT 20   //10
#define MARGIN_TOP 20     //40
#define MARGIN_BOTTOM 30  //30
#endif

// ------------------------------------------------------------
// AXIS SCALING MODE
#define X_MIN_AUTO true
#define X_MIN_VALUE 0.0f
#define X_MAX_AUTO true
#define X_MAX_VALUE 100.0f

#define Y_MIN_AUTO true
#define Y_MIN_VALUE 5.0f
#define Y_MAX_AUTO true
#define Y_MAX_VALUE 15.0f

// ------------------------------------------------------------
// TITLES
#define CHART_TITLE "My Graph"
#define X_AXIS_TITLE "Sample Index"
#define Y_AXIS_TITLE "Value"

// ------------------------------------------------------------
// COLOURS
#define BACKGROUND_COLOR 0x31ed  // TFT_BLACK
#define AXIS_COLOR TFT_SILVER    //TFT_WHITE
#define GRID_MAJOR_COLOR TFT_LIGHTGREY
#define GRID_MINOR_COLOR TFT_DARKGREY
#define DATA_LINE_COLOR TFT_RED
#define TEXT_COLOR TFT_WHITE

// ------------------------------------------------------------
// FONTS (TFT_eSPI numeric font IDs)
#define TITLE_FONT 2
#define AXIS_LABEL_FONT 2
#define TICK_FONT 1

// ------------------------------------------------------------
// TICK CONFIGURATION
#define X_MAJOR_AUTO true
#define X_MAJOR_STEP 20.0f
#define X_MINOR_TICKS_PER_MAJOR 5

#define Y_MAJOR_AUTO true
#define Y_MAJOR_STEP 2.0f
#define Y_MINOR_TICKS_PER_MAJOR 4

#define X_TICK_DECIMALS 0
#define Y_TICK_DECIMALS 1

// =============================================================================
//  FUNCTION PROTOTYPE (works with TFT_eSPI or TFT_eSprite)
// =============================================================================
void plotGraph(TFT_eSPI &display,
               int chartX, int chartY, int chartW, int chartH,
               float yData[], int dataSize);

// =============================================================================
//  INTERNAL UTILITIES
// =============================================================================

static inline float mapFloat(float x, float in_min, float in_max,
                             float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static float niceStep(float roughStep) {
  if (roughStep <= 0) return 1;
  float exponent = floor(log10(roughStep));
  float fraction = roughStep / pow(10, exponent);
  float nice;
  if (fraction < 1.5) nice = 1;
  else if (fraction < 3) nice = 2;
  else if (fraction < 7) nice = 5;
  else nice = 10;
  return nice * pow(10, exponent);
}

static float autoMajorStep(float range) {
  if (range <= 0) return 1;
  float rough = range / 5.0f;
  float step = niceStep(rough);
  int intervals = (int)(range / step);
  if (intervals > 8) step = niceStep(rough * 2);
  else if (intervals < 3) step = niceStep(rough / 2);
  return step;
}

// ------------------------------------------------------------
//  NEW: smart data line drawing – one segment per X pixel
// ------------------------------------------------------------
static void drawDataLine(TFT_eSPI &display,
                         float xMin, float xMax,
                         float yMin, float yMax,
                         int plotLeft, int plotRight,
                         int plotTop, int plotBottom,
                         float yData[], int dataSize,
                         uint16_t color) {
  if (dataSize < 2) return;

  int prevX = -1, prevY = 0;
  for (int i = 0; i < dataSize; i++) {
    int x = (int)mapFloat((float)i, xMin, xMax, plotLeft, plotRight);
    int y = (int)mapFloat(yData[i], yMin, yMax, plotBottom, plotTop);

    // Keep line inside plot area (safety clamp)
    if (y < plotTop) y = plotTop;
    if (y > plotBottom) y = plotBottom;

    // Draw only when X pixel changes (avoid multiple segments in same column)
    if (x != prevX) {
      if (prevX != -1) {
        display.drawLine(prevX, prevY, x, y, color);
        //display.drawWideLine(prevX, prevY, x, y, 2, color);
      }
      prevX = x;
      prevY = y;
    } else {
      // Still moving vertically in the same column – update the reference point.
      // (You could also keep min/max and draw a vertical line, but this works fine for continuous signals.)
      prevY = y;
    }
  }
}

// =============================================================================
//  MAIN PLOTTING FUNCTION
// =============================================================================

#ifdef USE_TFT_DISPLAY
void plotGraph(TFT_eSPI &display,
               int chartX, int chartY, int chartW, int chartH,
               float yData[], int dataSize) {

  if (dataSize < 1) return;

  // --- Clear only this chart's outer rectangle ---
  //display.fillRect(chartX, chartY, chartW, chartH, BACKGROUND_COLOR);
  display.fillRect(chartX + display.fontHeight(AXIS_LABEL_FONT) -6, chartY + MARGIN_TOP - 4, chartW, chartH, BACKGROUND_COLOR);

#else
void plotGraph(TFT_eSprite &display,
               int chartX, int chartY, int chartW, int chartH,
               float yData[], int dataSize) {

  if (dataSize < 1) return;

  // --- Clear only this chart's outer rectangle ---
  //display.fillRect(chartX, chartY, chartW, chartH, BACKGROUND_COLOR);
  display.fillRect(0, 0, chartW, chartH, BACKGROUND_COLOR);

#endif




  // --- Calculate inner plotting area ---
  int plotLeft = chartX + MARGIN_LEFT;
  int plotRight = chartX + chartW - MARGIN_RIGHT;
  int plotTop = chartY + MARGIN_TOP;
  int plotBottom = chartY + chartH - MARGIN_BOTTOM;
  int plotWidth = plotRight - plotLeft;
  int plotHeight = plotBottom - plotTop;

  //display.fillRect(plotLeft-1, plotTop-1, plotWidth+4, plotHeight+4, BACKGROUND_COLOR);

  // ---------- 1. DATA RANGES ----------
  float xMin = X_MIN_AUTO ? 0.0f : X_MIN_VALUE;
  float xMax = X_MAX_AUTO ? (float)(dataSize - 1) : X_MAX_VALUE;

  float yMin = Y_MIN_AUTO ? yData[0] : Y_MIN_VALUE;
  float yMax = Y_MAX_AUTO ? yData[0] : Y_MAX_VALUE;
  if (Y_MIN_AUTO || Y_MAX_AUTO) {
    for (int i = 1; i < dataSize; i++) {
      if (Y_MIN_AUTO && yData[i] < yMin) yMin = yData[i];
      if (Y_MAX_AUTO && yData[i] > yMax) yMax = yData[i];
    }
  }
  if (yMax <= yMin) {
    if (Y_MIN_AUTO && Y_MAX_AUTO) yMax = yMin + 1.0f;
    else if (!Y_MIN_AUTO && Y_MAX_AUTO) yMax = yMin + 1.0f;
    else if (Y_MIN_AUTO && !Y_MAX_AUTO) yMin = yMax - 1.0f;
  }
  if (xMax <= xMin) xMax = xMin + 1.0f;

  // ---------- 2. CHART TITLE ----------
  display.setTextColor(TEXT_COLOR);
  display.setTextFont(TITLE_FONT);
  int tw = display.textWidth(CHART_TITLE, TITLE_FONT);
  int th = display.fontHeight(TITLE_FONT);
  int titleX = chartX + (chartW - tw) / 2;
  int titleY = chartY + MARGIN_TOP / 2 - th / 2;
  display.setCursor(titleX, titleY);
  display.print(CHART_TITLE);

  // ---------- 3. AXIS LABELS ----------
  display.setTextFont(AXIS_LABEL_FONT);

  // X axis label
  tw = display.textWidth(X_AXIS_TITLE, AXIS_LABEL_FONT);
  th = display.fontHeight(AXIS_LABEL_FONT);
  int xLabelX = plotLeft + (plotWidth - tw) / 2;
  //int xLabelY = plotBottom + MARGIN_BOTTOM / 2 - th / 2;
  int xLabelY = chartY + chartH - th;
  //display.setCursor(xLabelX, xLabelY);
  //display.print(X_AXIS_TITLE);
  display.drawString(X_AXIS_TITLE, xLabelX, xLabelY);  // fills whole sprite


  // Y axis label (vertical, rotated)
  // Measure text size (horizontal orientation)
  tw = display.textWidth(Y_AXIS_TITLE, AXIS_LABEL_FONT);
  //tw = display.textWidth(String(Y_AXIS_TITLE) + " ", AXIS_LABEL_FONT);
  th = display.fontHeight(AXIS_LABEL_FONT);

  /*
  uint8_t currentTextFont = display.textfont;
  uint8_t currentTextSize = display.textsize;
  display.setCursor(20, 20);
  display.printf("X:%d %d ", currentTextFont,currentTextSize);
*/
  // Create a sprite exactly big enough for the text
  TFT_eSprite yAxisTitle(&display);  // 'display' is your TFT or main sprite
  //TFT_eSprite yAxisTitle(&tft);  // 'display' is your TFT or main sprite
  yAxisTitle.createSprite(tw, th);
  yAxisTitle.fillSprite(BACKGROUND_COLOR);
  yAxisTitle.setTextFont(AXIS_LABEL_FONT);
  yAxisTitle.setTextSize(display.textsize);  // inherit Display/ X-axis text size

  /*
  currentTextFont = yAxisTitle.textfont;
  currentTextSize = yAxisTitle.textsize;
  //yAxisTitle.setTextFont(currentTextFont);
  display.printf("Y:%d %d", currentTextFont, currentTextSize);
*/

  yAxisTitle.setTextColor(TEXT_COLOR, BACKGROUND_COLOR);
  yAxisTitle.drawString(Y_AXIS_TITLE, 0, 0);  // fills whole sprite
  //yAxisTitle.setCursor(0, 0);
  //yAxisTitle.print(Y_AXIS_TITLE);      // last character may not print

  // Where on screen should the centre of the rotated label appear?
  int16_t centerX = chartX + th / 2;           // horizontal middle of left margin
  int16_t centerY = plotTop + plotHeight / 2;  // vertical middle of the plot area
  yAxisTitle.setPivot(tw / 2, th / 2);

  display.setPivot(centerX, centerY);
  // Pivot = centre of the sprite
  // Push rotated by 270° (reads top‑to‑bottom) or 90° (bottom‑to‑top)
#ifdef USE_TFT_DISPLAY
  yAxisTitle.pushRotated(270, BACKGROUND_COLOR);
#else
  yAxisTitle.pushRotated(&display, 270, BACKGROUND_COLOR);
#endif
  yAxisTitle.deleteSprite();  // free memory if you no longer need it

  /*
// We draw character by character, each below the previous.
    const char* yTitle = Y_AXIS_TITLE;
    int len = strlen(yTitle);
    int charWidth = display.textWidth("W", AXIS_LABEL_FONT);   // typical max width
    int charHeight = display.fontHeight(AXIS_LABEL_FONT);
    // Total height of the vertical string
    int totalHeight = len * charHeight;
    // Start Y so the string is vertically centred in the plot area
    int startY = plotTop + (plotHeight - totalHeight) / 2;
    // Horizontal position: centre the character in the left margin
    int xPos = chartX + (MARGIN_LEFT - charWidth) / 2;
    if (xPos < chartX) xPos = chartX + 1;   // safety

    for (int i = 0; i < len; i++) {
        display.setCursor(xPos, startY + i * charHeight);
        display.print(yTitle[i]);
    }
*/


  // ---------- 4. AXIS BOX ----------
  display.drawRect(plotLeft, plotTop, plotWidth, plotHeight, AXIS_COLOR);

  // ---------- 5. TICK COMPUTATION ----------
  float xRange = xMax - xMin;
  float xMajorStep = X_MAJOR_AUTO ? autoMajorStep(xRange) : X_MAJOR_STEP;
  int xMinorDiv = X_MINOR_TICKS_PER_MAJOR;
  float xMinorStep = xMajorStep / xMinorDiv;

  float yRange = yMax - yMin;
  float yMajorStep = Y_MAJOR_AUTO ? autoMajorStep(yRange) : Y_MAJOR_STEP;
  int yMinorDiv = Y_MINOR_TICKS_PER_MAJOR;
  float yMinorStep = yMajorStep / yMinorDiv;

  // ---------- 6. GRID LINES & TICK LABELS ----------
  display.setTextFont(TICK_FONT);
  display.setTextColor(TEXT_COLOR);

  // X axis (vertical grid)
  float xStartTick = ceil(xMin / xMajorStep) * xMajorStep;
  for (float xVal = xStartTick; xVal <= xMax; xVal += xMajorStep) {
    int xPixel = (int)mapFloat(xVal, xMin, xMax, plotLeft, plotRight);
    if (xPixel >= plotLeft && xPixel <= plotRight) {
      display.drawFastVLine(xPixel, plotTop, plotHeight, GRID_MAJOR_COLOR);
      char buf[16];
      dtostrf(xVal, 4, X_TICK_DECIMALS, buf);
      while (buf[0] == ' ') memmove(buf, buf + 1, strlen(buf));
      tw = display.textWidth(buf, TICK_FONT);
      th = display.fontHeight(TICK_FONT);
      display.setCursor(xPixel - tw / 2, plotBottom + 2);
      display.print(buf);
    }
    for (int m = 1; m < xMinorDiv; m++) {
      float minorVal = xVal + m * xMinorStep;
      if (minorVal > xMax) break;
      int minorPixel = (int)mapFloat(minorVal, xMin, xMax, plotLeft, plotRight);
      if (minorPixel >= plotLeft && minorPixel <= plotRight)
        display.drawFastVLine(minorPixel, plotTop, plotHeight, GRID_MINOR_COLOR);
    }
  }

  // Y axis (horizontal grid)
  float yStartTick = ceil(yMin / yMajorStep) * yMajorStep;
  for (float yVal = yStartTick; yVal <= yMax; yVal += yMajorStep) {
    int yPixel = (int)mapFloat(yVal, yMin, yMax, plotBottom, plotTop);
    if (yPixel >= plotTop && yPixel <= plotBottom) {
      display.drawFastHLine(plotLeft, yPixel, plotWidth, GRID_MAJOR_COLOR);
      char buf[16];
      dtostrf(yVal, 4, Y_TICK_DECIMALS, buf);
      while (buf[0] == ' ') memmove(buf, buf + 1, strlen(buf));
      tw = display.textWidth(buf, TICK_FONT);
      th = display.fontHeight(TICK_FONT);
      display.setCursor(plotLeft - tw - 2, yPixel - th / 2);
      display.print(buf);
    }
    for (int m = 1; m < yMinorDiv; m++) {
      float minorVal = yVal + m * yMinorStep;
      if (minorVal > yMax) break;
      int minorPixel = (int)mapFloat(minorVal, yMin, yMax, plotBottom, plotTop);
      if (minorPixel >= plotTop && minorPixel <= plotBottom)
        display.drawFastHLine(plotLeft, minorPixel, plotWidth, GRID_MINOR_COLOR);
    }
  }

  // ---------- 7. PLOT DATA (now using the aliasing‑free line drawing) ----------
  drawDataLine(display,
               xMin, xMax, yMin, yMax,
               plotLeft, plotRight, plotTop, plotBottom,
               yData, dataSize, DATA_LINE_COLOR);
}

#endif  // PLOT_GRAPH_H