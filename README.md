# Arduino-Documentation
Store all link related to Arduino

# 10 ESP32 Pro Features: Take your Projects to the Next Level!
https://www.youtube.com/watch?v=_HzG5FjH_6g

# ST7796S 4" SPI TFT touch screen  display
The MSP4020 / MSP4021 is a 4.0-inch TFT display using the ST7796S driver and XPT2046 touch controller. To set it up with an ESP32, you will primarily use the TFT_eSPI library for the display and the XPT2046_Touchscreen library for touch functionality.

1. Wiring Connection (ESP32 to MSP4020/4021)
   The display and touch controller share the same SPI bus (MOSI, MISO, SCK) but use different Chip Select (CS) pins.
   Module Pin        Function              ESP32 Pin (Typical VSPI)   Notes
   VCC               Power                 5V or 3.3V                 Depends on jumper setting
   GND               Ground                GND
   LCD_CS            Display Chip Select   GPIO 5
   LCD_RST           Reset                 GPIO 4
   LCD_RS/DC         Data/Command          GPIO 2                     Also labeled DC or RS
   SDI (MOSI)        Data In               GPIO 23
   SCK               SPI Clock             GPIO 18
   SDO (MISO)        Data Out              GPIO 19                    Required for touch read
   LED               Backlight             3.3V or GPIO               Can use PWM for brightness
   T_CS              Touch Chip Select     GPIO 21                    Separate from LCD_CS
   T_IRQ             Touch Interrupt       GPIO 22                    Optional

2. Software Setup (Arduino IDE)
Step 1: Install Libraries
  1. Open Library Manager and install TFT_eSPI by Bodmer.
  2. Install XPT2046_Touchscreen by Paul Stoffregen.

Step 2: Configure TFT_eSPI
TFT_eSPI requires you to edit a header file inside the library folder to match your hardware.
  1. Navigate to your Arduino libraries folder: Documents/Arduino/libraries/TFT_eSPI/.
  2. Open User_Setup.h and make the following changes:
     1. Uncomment #define ST7796_DRIVER.
     2. Define dimensions:
        #define TFT_WIDTH  320
        #define TFT_HEIGHT 480
     3. Define pins (using the GPIO numbers from the table above):
        #define TFT_MISO 19
        #define TFT_MOSI 23
        #define TFT_SCLK 18
        #define TFT_CS    5
        #define TFT_DC    2
        #define TFT_RST   4
        #define TOUCH_CS 21
    4. Set Frequency: #define SPI_FREQUENCY 27000000 (Max stability for ST7796S).

Step 3: Example Code Snippet
   #include <SPI.h>
   #include <TFT_eSPI.h> // Hardware-specific library
   TFT_eSPI tft = TFT_eSPI(); // Invoke library
   void setup() {
     tft.init();
     tft.setRotation(1); // Landscape
     tft.fillScreen(TFT_BLACK);
     tft.setTextColor(TFT_WHITE);
     tft.setTextSize(2);
     tft.setCursor(20, 20);
     tft.println("ESP32 + ST7796S Ready!");
   }
   void loop() {
     // Your code here
   }

3. Documentation Resources
   LCD Wiki: Search for "MSP4020" at lcdwiki.com for the full user manual and schematic.
     https://www.lcdwiki.com/Main_Page#top
   TFT_eSPI Docs: Official documentation on Read the Docs.
     https://doc-tft-espi.readthedocs.io/
  
