# Arduino-Documentation
Store all link related to Arduino

# 10 ESP32 Pro Features: Take your Projects to the Next Level!
https://www.youtube.com/watch?v=_HzG5FjH_6g

# ST7796S 4" SPI TFT touch screen  display
The MSP4020 / MSP4021 is a 4.0-inch TFT display using the ST7796S driver and XPT2046 touch controller. To set it up with an ESP32, you will primarily use the TFT_eSPI library for the display and the XPT2046_Touchscreen library for touch functionality.

1. Wiring Connection (ESP32 to MSP4020/4021).<br>
   The display and touch controller share the same SPI bus (MOSI, MISO, SCK) but use different Chip Select (CS) pins.<br>
   ```Module Pin        Function              ESP32 Pin (Typical VSPI)   Notes```<br>
   ```VCC               Power                 5V or 3.3V                 Depends on jumper setting```<br>
   ```GND               Ground                GND```<br>
   ```LCD_CS            Display Chip Select   GPIO 5```<br>
   ```LCD_RST           Reset                 GPIO 4```<br>
   ```LCD_RS/DC         Data/Command          GPIO 2                     Also labeled DC or RS```<br>
   ```SDI (MOSI)        Data In               GPIO 23```<br>
   ```SCK               SPI Clock             GPIO 18```<br>
   ```SDO (MISO)        Data Out              GPIO 19                    Required for touch read```<br>
   ```LED               Backlight             3.3V or GPIO               Can use PWM for brightness```<br>
   ```T_CS              Touch Chip Select     GPIO 21                    Separate from LCD_CS```<br>
   ```T_IRQ             Touch Interrupt       GPIO 22                    Optional```<br>

4. Software Setup (Arduino IDE)<br>
Step 1: Install Libraries<br>
  - Open Library Manager and install TFT_eSPI by Bodmer.<br>
  - Install XPT2046_Touchscreen by Paul Stoffregen.<br>

Step 2: Configure TFT_eSPI<br>
TFT_eSPI requires you to edit a header file inside the library folder to match your hardware.<br>
  - Navigate to your Arduino libraries folder: Documents/Arduino/libraries/TFT_eSPI/.<br>
  - Open User_Setup.h and make the following changes:<br>
     1. Uncomment<br>
        #define ST7796_DRIVER.<br>
     3. Define dimensions:<br>
        #define TFT_WIDTH  320<br>
        #define TFT_HEIGHT 480<br>
     4. Define pins (using the GPIO numbers from the table above):
        #define TFT_MISO 19<br>
        #define TFT_MOSI 23<br>
        #define TFT_SCLK 18<br>
        #define TFT_CS    5<br>
        #define TFT_DC    2<br>
        #define TFT_RST   4<br>
        #define TOUCH_CS 21<br>
    5. Set Frequency: #define SPI_FREQUENCY 27000000 (Max stability for ST7796S).<br>

Step 3: Example Code Snippet<br>
```   #include <SPI.h>```<br>
```   #include <TFT_eSPI.h> // Hardware-specific library```<br>
```   TFT_eSPI tft = TFT_eSPI(); // Invoke library```<br>
```   void setup() {```<br>
```     tft.init();```<br>
```     tft.setRotation(1); // Landscape```<br>
```     tft.fillScreen(TFT_BLACK);```<br>
```     tft.setTextColor(TFT_WHITE);```<br>
```     tft.setTextSize(2);```<br>
```     tft.setCursor(20, 20);```<br>
```     tft.println("ESP32 + ST7796S Ready!");```<br>
```   }```<br>
```   void loop() {```<br>
```     // Your code here```<br>
```   }```<br>

3. Documentation Resources<br>
   LCD Wiki: Search for "MSP4020" at lcdwiki.com for the full user manual and schematic.<br>
     https://www.lcdwiki.com/Main_Page#top<br>
   TFT_eSPI Docs: Official documentation on Read the Docs.<br>
     https://doc-tft-espi.readthedocs.io/

