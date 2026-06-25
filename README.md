# Arduino-Documentation
Store all link related to Arduino

# Install and Setup Arduino IDE
1. Get the latest Arduino IDE here: https://www.arduino.cc/en/software/
2. Open IDE, go to: **File > Preferences**, under **Settings > Additional Boards Manager URLs**. Add this:
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
3. Go to the “Board manager” tab, on the left-hand side of the screen. Type “esp32” in the search bar.
   Install **esp32 (by Espressif Systems)**
   https://github.com/espressif/arduino-esp32   
5. Go to **Tools > Board** and select **ESP32S3 Dev Module**
6. Connect the ESP32S3 Dev Module to your computer using a USB cable. Then, go to **Tools > Port** and select the COM port that the ESP32S3 Dev Module is connected to.
7. If you have trouble to connect to computer, identify the driver for your ESP32S3:<br>
   (a) CH343P Driver (Used by your new ESP32-S3 board), click the link for CH343SER.EXE: https://www.wch-ic.com/downloads/CH343SER_EXE.html<br>
   (b) CP2102 Driver (Used by older/standard ESP32 boards), click the link for "CP210x Universal Windows Driver": https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads<br>

**Find sketches, libraries, board cores, and other files on your computer**   
https://support.arduino.cc/hc/en-us/articles/4415103213714-Find-sketches-libraries-board-cores-and-other-files-on-your-computer

**Espressif Systems**    
https://www.aliexpress.com/item/1005009523697481.html?

# ESP32-S3 Pins Assignment

## Pins to Avoid

While most pins are available, you should generally avoid these pins to prevent system crashes or interference: <br>  
**GPIO 43 & 44:** Reserved for UART0 (Serial Debugging). GPIO 43 is U0TXD and GPIO 44 is U0RXD. <br>
**GPIO 19 & 20:** Used for the Native USB port. GPIO 19 (USB_D-) and GPIO 20 (USB_D+).  <br>
**GPIO 0, 3, 45, 46:** Strapping pins used for boot modes; using them can prevent the board from starting properly. <br>

<img width="1056" height="720" alt="ESP32-S3-Pins" src="https://github.com/user-attachments/assets/6069c03d-e222-4af0-b8f0-81319b4cd989" />
<br><br>

<img width="1000" height="880" alt="ESP32-S3-Pins-2" src="https://github.com/user-attachments/assets/2fa0093f-97df-4033-b34b-7b91a3ebafc0" />



<img alt="image" style="width: 75%; height: auto;"   src="https://github.com/user-attachments/assets/08bf4314-1528-4d3c-a6fc-99eeba7dcb08" />


<br>

# ESP32 S3 Board

<img width="605" height="491" alt="ESP32-S3-PCBA" src="https://github.com/user-attachments/assets/ec73d7da-acef-4d03-a89f-68f72a6ce86f" />


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

## Schematics

<img width="702" height="594" alt="image" src="https://github.com/user-attachments/assets/3eb0e614-0a6a-4577-8973-cc6ed4ea199a" />

<img width="707" height="795" alt="image" src="https://github.com/user-attachments/assets/2ecc9c32-3bb2-4270-8604-35b382c3f2b3" />

<img width="627" height="611" alt="image" src="https://github.com/user-attachments/assets/ed28a01f-c33a-4fa7-aed1-7380aa46385e" />


# ESP32-S3 References

https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/index.html
<br>

https://github.com/espressif

**Up to date technical documents:**    
Copy and paste below link on brower:   
*https://www.espressif.com/en/support/documents/technical-documents*   

Hardware Design Guide:    
https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/index.html

https://mischianti.org/vcc-gnd-studio-yd-esp32-s3-devkitc-1-clone-high-resolution-pinout-and-specs/



# 10 ESP32 Pro Features: Take your Projects to the Next Level!
https://www.youtube.com/watch?v=_HzG5FjH_6g


# Resistor Color Band

<img width="635" height="682" alt="image" src="https://github.com/user-attachments/assets/fa5e2931-75d9-4af8-9c2f-e70d87138844" />

**Resistor Calculator**

https://www.calculator.net/resistor-calculator.html


# Circuit Simulators

✔️ Cirkit Designer:
https://app.cirkitdesigner.com/

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


