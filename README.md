# Arduino-Documentation
Store all link related to Arduino

# Install and Setup Arduino IDE
1. Get the latest Arduino IDE here: https://www.arduino.cc/en/software/
2. Open IDE, go to: **File > Preferences**, under **Settings > Additional Boards Manager URLs**. Add this:
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
3. Go to the “Board manager” tab, on the left-hand side of the screen. Type “esp32” in the search bar.
   Install **esp32 (by Espressif Systems)**
4. Go to **Tools > Board** and select **ESP32S3 Dev Module**
5. Connect the ESP32S3 Dev Module to your computer using a USB cable. Then, go to **Tools > Port** and select the COM port that the ESP32S3 Dev Module is connected to.
6. If you have trouble to connect to computer, identify the driver for your ESP32S3:<br>
   (a) CH343P Driver (Used by your new ESP32-S3 board), click the link for CH343SER.EXE: https://www.wch-ic.com/downloads/CH343SER_EXE.html<br>
   (b) CP2102 Driver (Used by older/standard ESP32 boards), click the link for "CP210x Universal Windows Driver": https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads<br>

# ESP32-S3 Pins Assignment

## Pins to Avoid

While most pins are available, you should generally avoid these pins to prevent system crashes or interference: <br>  
**GPIO 43 & 44:** Reserved for UART0 (Serial Debugging). GPIO 43 is U0TXD and GPIO 44 is U0RXD. <br>
**GPIO 19 & 20:** Used for the Native USB port. GPIO 19 (USB_D-) and GPIO 20 (USB_D+).  <br>
**GPIO 0, 3, 45, 46:** Strapping pins used for boot modes; using them can prevent the board from starting properly. <br>

<img width="1056" height="720" alt="ESP32-S3-Pins" src="https://github.com/user-attachments/assets/6069c03d-e222-4af0-b8f0-81319b4cd989" />
<br><br>
<img width="1149" height="800" alt="image" src="https://github.com/user-attachments/assets/ff95dae1-60c7-4860-8398-d2eef7d9ca09" />

<br>
https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/index.html
<br>

**Up to date technical documents:**   
https://www.espressif.com/en/support/documents/technical-documents/?

# Resistor Color Band

<img width="635" height="682" alt="image" src="https://github.com/user-attachments/assets/fa5e2931-75d9-4af8-9c2f-e70d87138844" />

**Resistor Calculator**

https://www.calculator.net/resistor-calculator.html

# 10 ESP32 Pro Features: Take your Projects to the Next Level!
https://www.youtube.com/watch?v=_HzG5FjH_6g

# ESP32 S3 Board

## Buttons
**1. The RST (Reset) Button**  
What it does: It restarts the microcontroller.   
How it works: It temporarily pulls the EN (Enable) pin to a low state, which cuts power to the chip's core and immediately restarts it.   
Analogy: It is exactly like pressing the physical "Restart" button on a PC, or unplugging it and plugging it back in.   
Result: The ESP32 will reboot and immediately start running the sketch (code) that is currently stored in its memory. It does not erase or change your code.   


**2. The Boot Button**
What it does: It tells the ESP32 to enter "Download Mode" (also called Flash Mode).   
How it works: It is physically connected to GPIO 0. When the ESP32 boots up and sees that GPIO 0 is pulled LOW (grounded), it knows you want to upload new code, so it pauses the normal boot process and waits for a new sketch from the computer.   
Result: The ESP32 will not run your current sketch. Instead, it will wait for the Arduino IDE (or esptool) to send a new program over the USB cable.   

**How to Use Them Together (The Manual Upload Sequence)**   
Sometimes, the Arduino IDE fails to automatically put the ESP32-S3 into Download Mode (this happens frequently with ESP32-S3 boards that use Native USB). When this happens, you must manually trigger Download Mode using both buttons.     
The "Hold Boot, Tap RST" Sequence:
1. Press and HOLD the Boot button.
2. While still holding Boot, press and release the RST button.
3. Release the Boot button.

What just happened?   
By holding Boot, you forced GPIO 0 to be LOW. By tapping RST, you restarted the chip. The chip restarted, saw that GPIO 0 was LOW, and entered Download Mode.   
Once you do this, you will usually see a message in the Arduino IDE Serial Monitor or the bottom status bar saying "Connecting..." or "Hard resetting via RTS pin...". At this exact moment, click the Upload button in the Arduino IDE, and it will successfully flash your new code.   

**Summary**   
*RST:* "Restart the current program."   
*Boot:* "Get ready to receive a new program."   
*Boot + RST:* "Force the board into the mode required to upload a new program."   

## Voltage Regulator

LM1117 Linear low drop 3.3V 800mA voltage regulator:   
<img alt="image" style="width: 50%; height: auto;" src="https://github.com/user-attachments/assets/353ebae9-632c-4b28-ae44-4cfbb53b6d1c" />



# ST7796S 4" SPI TFT touch screen  display
The MSP4020 / MSP4021 is a 4.0-inch TFT display using the ST7796S driver and XPT2046 touch controller. To set it up with an ESP32, you will primarily use the TFT_eSPI library for the display and the XPT2046_Touchscreen library for touch functionality.

## 1. Wiring Connection (ESP32 to MSP4020/4021).<br>
   The display and touch controller share the same SPI bus (MOSI, MISO, SCK) but use different Chip Select (CS) pins.<br>
   ```
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
```
## 2. Software Setup (Arduino IDE)
**Step 1: Install Libraries**<br>
  - Open Library Manager and install TFT_eSPI by Bodmer.<br>
  - Install XPT2046_Touchscreen by Paul Stoffregen.<br>

**Step 2: Configure TFT_eSPI**<br>
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

**Step 3: Example Code Snippet**<br>
```
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
```
## 3. Documentation Resources<br>
   LCD Wiki: Search for "MSP4020" at lcdwiki.com for the full user manual and schematic.<br>
     https://www.lcdwiki.com/Main_Page#top<br>
   TFT_eSPI Docs: Official documentation on Read the Docs.<br>
     https://doc-tft-espi.readthedocs.io/

# Circuit Simulators

Tinkercad: 
https://www.tinkercad.com/

CRUMB:
https://www.crumbsim.com/

Falstad:
https://www.falstad.com/circuit/

Qucs:
https://qucs.sourceforge.net/

EveryCircuit:
https://everycircuit.com/

CircuitLab:
https://www.circuitlab.com/

LTspice:
https://www.analog.com/en/resources/d...

TINA-TI
https://www.ti.com/tool/TINA-TI

Proteus:
https://www.labcenter.com/

Cirkit Designer:
https://app.cirkitdesigner.com/
