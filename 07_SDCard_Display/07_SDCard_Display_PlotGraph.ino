// Define the SD Chip Select pin (must match wiring)
#define VSPI_PIN  // SD card is using VSPI and TFT is using HSPI

// Need to comment off #define MY_ILI9488 in User_Setup.h
//#define TFT_DISPLAY_1P8

#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

//#define USE_TFT_DISPLAY
#include "plot_graph.h"

#ifndef USE_TFT_DISPLAY
TFT_eSprite chartSprite(&tft);  // sprite object, not a pointer
// Max memory/heap 420*310*2 = 260,400
#define CHART_W 240  //420
#define CHART_H 160  //310
#endif


#ifdef VSPI_PIN
#define SD_SCLK_PIN 4
#define SD_MISO_PIN 5
#define SD_MOSI_PIN 6
#else
#define SD_SCLK_PIN 12
#define SD_MISO_PIN 13
#define SD_MOSI_PIN 11
#endif

#ifndef TFT_DISPLAY_1P8

#define SD_CS_PIN 7
#define SD_FREQUENCY 16000000  // 16MHz or 4MHz
#endif


#ifdef VSPI_PIN
SPIClass sdSPI(VSPI);
#else
SPIClass sdSPI(HSPI);
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef DEG2RAD
#define DEG2RAD (M_PI / 180.0)
#endif

#ifdef TFT_DISPLAY_1P8

#else

#endif

// Live data buffer (circular, here just a full array)
float data1[200], data2[200], data3[200], data4[200];

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

#ifndef TFT_DISPLAY_1P8
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
#endif

#ifdef TFT_DISPLAY_1P8

#else
tft.setRotation(1);  // Adjust rotation as needed
tft.fillScreen(TFT_BLACK);
#endif


  // fill data with different signals...
  for (int i = 0; i < 200; i++) data1[i] = 10 + 5 * sin(i * 0.1);
  for (int i = 0; i < 200; i++) data2[i] = 5 + 2 * cos(i * 0.2);
  for (int i = 0; i < 200; i++) data3[i] = 3 + i * 0.1;
  for (int i = 0; i < 200; i++) data4[i] = 8 + 4 * sin(i * 0.3) * cos(i * 0.05);

#ifndef USE_TFT_DISPLAY
  // Create a sprite the size of your chart area
  chartSprite.createSprite(CHART_W, CHART_H);  // e.g., top‑left quarter
  chartSprite.fillScreen(TFT_BLACK);           // clear sprite background if needed
#endif
  Serial.printf("Free 8-bit heap: %u\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
}

void waitForSerial() {

  // Clear the serial buffer so it doesn't instantly loop again
  while (Serial.available())
    Serial.read();
  Serial.println("Press <SEND> (or type a character) to advance...");
  // Wait here until a character is received over Serial
  while (!Serial.available())
    ;
}



void loop() {
  // Simulate incoming live data – update one element
  static int idx = 0, idx_3 = 0, idx_4 = 0;
  static float phase = 0.0;

  for (idx = 0; idx < 200; idx++) {
    data1[idx] = 10.0f + 5.0f * sin(phase + 2 * M_PI * idx / 100);  // 100 -> 2 cycles
    //Serial.printf(" idx= %d, S= %0.1f ", idx, data1[idx]);
  }
  //Serial.println("");

  for (idx = 0; idx < 200; idx++)
    data2[idx] = 5.0f * cos(phase + 2 * M_PI * idx / 100);  // 100 -> 2 cycles

#ifndef USE_TFT_DISPLAY
  // Call plotGraph using the SPRITE as the "display"
  //plotGraph(chartSprite, 0, 0, CHART_W, CHART_H, data1, 200);
  plotGraph(chartSprite, 0, 0, CHART_W, CHART_H, data1, 200);
  // Push the finished sprite to the display at (0,0)
  chartSprite.pushSprite(0, 0);




  // Call plotGraph using the SPRITE as the "display"
  plotGraph(chartSprite, 0, 0, CHART_W, CHART_H, data2, 200);

  // Push the finished sprite to the display at (0,0)
  chartSprite.pushSprite(CHART_W, 0);

  // chartSprite.deleteSprite();  // free RAM if you recreate each time

  // Call plotGraph using the SPRITE as the "display"
  plotGraph(chartSprite, 0, 0, CHART_W, CHART_H, data3, idx_3);

  // Push the finished sprite to the display at (0,0)
  chartSprite.pushSprite(0, CHART_H);

  // Call plotGraph using the SPRITE as the "display"
  plotGraph(chartSprite, 0, 0, CHART_W, CHART_H, data4, idx_4);

  // Push the finished sprite to the display at (0,0)
  chartSprite.pushSprite(CHART_W, CHART_H);


#else

  // plot directly to the screen, image will be flicky
  // need to comment out this above #define USE_SPRITE_ROTATE
  //plotGraph(tft, 0, 0, tft.width(), tft.height(), data1, 200);
  plotGraph(tft, 0, 0, tft.width(), tft.height(), data4, idx_4);
#endif

  phase += 5.0 * DEG2RAD;
  if (phase > 2 * M_PI) phase = 0.0;

  idx_3 += 1;
  if (idx_3 > 200) idx_3 = 0;
  idx_4 += 2;
  if (idx_4 > 200) idx_4 = 0;

  delay(50);
  // waitForSerial();
}
