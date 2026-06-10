#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  tft.init();

  // run Touch_calibrate.ino from Example to do the calibration
  // Use this calibration code in setup():
  uint16_t calData[5] = { 263, 3627, 233, 3513, 7 }; 
  tft.setTouch(calData);

  tft.setRotation(1);  // Landscape
  Serial.println("TFT_eSPI Touch Test...");
  tft.setTextSize(2);
  tft.println("Touch anywhere on the screen");
}

void loop() {
  uint16_t x = 0, y = 0;

  // tft.getTouch(&x, &y, threshold)
  // The third parameter (300) is the minimum pressure threshold to register a touch
  if (tft.getTouch(&x, &y, 300)) {
    Serial.print("Screen X: ");
    Serial.print(x);
    Serial.print(" | Screen Y: ");
    Serial.println(y);

    // Draw a dot where you touch
    tft.fillCircle(x, y, 5, TFT_RED);

    delay(50);  // Debounce
  }
}
