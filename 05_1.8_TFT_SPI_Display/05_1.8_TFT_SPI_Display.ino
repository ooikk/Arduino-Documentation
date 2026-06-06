#define ESPI_LIBRARY

#ifdef ESPI_LIBRARY

#include <TFT_eSPI.h>
#include <SPI.h>
TFT_eSPI tft = TFT_eSPI();

#else

#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_ST7735.h>  // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h>  // Hardware-specific library for ST7789
#include <SPI.h>


// Pin definitions for ESP32
#define TFT_CS 10
#define TFT_RST 9
#define TFT_DC 14
#define TFT_MOSI 11
#define TFT_SCLK 12

// For ST7735-based displays, we will use this call
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

#define TFT_WHITE ST77XX_WHITE
#define TFT_BLACK ST77XX_BLACK
#define TFT_RED ST77XX_RED
#define TFT_GREEN ST77XX_GREEN
#define TFT_BLUE ST77XX_BLUE

#endif

int intRotate, intTextSize;
unsigned int colorBack[] = { 0x7199, 0x7199, 0x7199, 0x7199 };
unsigned int colorFront[] = { 0xeb26, 0xfd6d, 0xff1c, 0x8efb };


void waitForSerial() {

  // Clear the serial buffer so it doesn't instantly loop again
  while (Serial.available())
    Serial.read();
  Serial.println("Press <SEND> (or type a character) to advance");
  // Wait here until a character is received over Serial
  while (!Serial.available())
    ;
}



void setup() {
  Serial.begin(115200);
#ifdef ESPI_LIBRARY
  tft.init();
#else
  tft.initR(INITR_BLACKTAB);  // Init ST7735S chip, black tab
#endif
  tft.setTextWrap(true);
  tft.fillScreen(TFT_BLACK);
  tft.println("ESP32-S3 OK!");
  waitForSerial();
}

void loop() {

  intRotate = 0;
  intTextSize = 1;
  tft.setTextFont(1);  // set to default
  for (int i = 0; i < 4; i++) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(colorFront[i], colorBack[i]);
    tft.setTextSize(intTextSize);
    tft.setCursor(0, 0);
    tft.setRotation(intRotate);  

    String msg = "Rotate: " + String(intRotate++) + " Size:" + String(intTextSize);
    tft.println(msg);
    Serial.println(msg);
    waitForSerial();

#ifdef ESPI_LIBRARY
    //tft.fillScreen(TFT_BLACK);
    tft.drawString(msg, 0, 40, intTextSize++);
    Serial.println(msg);
    waitForSerial();
#endif

    if (intTextSize > 2) intTextSize = 1;
  }


#ifdef ESPI_LIBRARY
  Serial.println("Font start");
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED);

  tft.setTextFont(1);
  tft.drawString("Font 1", 0, 0);

  tft.setTextFont(2);
  tft.setTextColor(TFT_BLUE);
  tft.drawString("Font 2", 0, 10);

  tft.setTextFont(4);
  tft.setTextColor(TFT_ORANGE);
  tft.drawString("Font 4", 0, 40);
  waitForSerial();

  Serial.println("Freefont start");
  int yPos = 0;
  tft.fillScreen(TFT_BLACK);
  // Small font
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(TFT_GREEN);
  tft.drawString("Small Sans", 0, yPos);
  yPos += 30;

  // Medium bold
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("Bold", 0, yPos);
  yPos += 25;

  // Large mono
  tft.setFreeFont(&FreeMono9pt7b);
  tft.setTextColor(TFT_CYAN);
  tft.drawString("Mono", 0, yPos);
  yPos += 35;

  // Serif italic
  tft.setFreeFont(&FreeSerifItalic9pt7b);
  tft.setTextColor(TFT_MAGENTA);
  tft.drawString("Serif Italic", 0, yPos);
  Serial.println("Freefont end");
  waitForSerial();
#endif

  tft.fillScreen(colorFront[0]);
  delay(1000);
  tft.fillScreen(colorFront[1]);
  delay(1000);
  tft.fillScreen(colorFront[2]);
  delay(1000);
  tft.fillScreen(colorFront[3]);
  delay(1000);
  waitForSerial();
}