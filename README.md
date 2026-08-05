# Arduino-Documentation


## Install and Setup Arduino IDE
1. Get the latest Arduino IDE here: https://www.arduino.cc/en/software/
2. Open IDE, go to: **File > Preferences**, under **Settings > Additional Boards Manager URLs**. Add this:
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
3. Go to the “Board manager” tab, on the left-hand side of the screen. Type “esp32” in the search bar.
   Install **esp32 (by Espressif Systems)**
   https://github.com/espressif/arduino-esp32   
5. Go to **Tools > Board** and select ```ESP32S3 Dev Module```
6. Go to **Tools >** Select the Configure Settings. Configure the following critical settings in the Tools menu:
   - **USB CDC On Boot**: ```Disabled``` 
   - **Flash Mode**: ```QIO 80MHz```
   - **Flash Size**: ```16MB```
   - **PSRAM**: ```OPI PSRAM``` (if your board has Octal SPI PSRAM) or ```Disabled```.
   - **Upload Mode**: ```UART0 / Hardware CDC``` (or ```USB-OTG(TinyUSB)``` if using the TinyUSB stack).
   - **USB Mode**: ```Hardware CDC and JTAG```.      
7. Connect the ESP32S3 Dev Module to your computer using a USB cable. Then, go to **Tools > Port** and select the COM port that the ESP32S3 Dev Module is connected to.
8. If you have trouble to connect to computer, identify the driver for your ESP32S3:<br>
   (a) CH343P Driver (Used by your new ESP32-S3 board), click the link for CH343SER.EXE:<br>
   https://www.wch-ic.com/downloads/CH343SER_EXE.html<br>
   (b) CP2102 Driver (Used by older/standard ESP32 boards), click the link for "CP210x Universal Windows Driver":<br>
   https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads<br>

**Find sketches, libraries, board cores, and other files on your computer**   
https://support.arduino.cc/hc/en-us/articles/4415103213714-Find-sketches-libraries-board-cores-and-other-files-on-your-computer

**Espressif Systems**    
https://www.aliexpress.com/item/1005009523697481.html?

## ESP32-S3 Pins Assignment

### Pins to Avoid

While most pins are available, you should generally avoid these pins to prevent system crashes or interference: <br>  
**GPIO 43 & 44:** Reserved for UART0 (Serial Debugging). GPIO 43 is U0TXD and GPIO 44 is U0RXD. <br>
**GPIO 19 & 20:** Used for the Native USB port. GPIO 19 (USB_D-) and GPIO 20 (USB_D+).  <br>
**GPIO 0, 3, 45, 46:** Strapping pins used for boot modes; using them can prevent the board from starting properly. <br>

<img width="1056" height="720" alt="ESP32-S3-Pins" src="https://github.com/user-attachments/assets/6069c03d-e222-4af0-b8f0-81319b4cd989" />
<br><br>

<img width="1000" height="880" alt="ESP32-S3-Pins-2" src="https://github.com/user-attachments/assets/2fa0093f-97df-4033-b34b-7b91a3ebafc0" />



<img alt="image" style="width: 75%; height: auto;"   src="https://github.com/user-attachments/assets/08bf4314-1528-4d3c-a6fc-99eeba7dcb08" />


<br>

## ESP32 S3 Board

<img width="605" height="491" alt="ESP32-S3-PCBA" src="https://github.com/user-attachments/assets/ec73d7da-acef-4d03-a89f-68f72a6ce86f" />


### Buttons
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

### USB Ports     

ESP32-S3 development boards typically feature two USB ports to accommodate the chip's dual-path communication capabilities: a USB-to-UART bridge and a Native USB (OTG) interface.     
According to the sources, the ESP32-S3 is one of the few chips in the family that natively supports both USB OTG and USB Serial through the Arduino Core.    
1. USB-to-UART Port (Serial/Debug)     
This port is generally used for standard development tasks. It connects to the computer via a bridge chip (such as the CP2102 or CH340) located on the development board.
This port is connected to a separate, dedicated USB-to-UART converter chip on the board (commonly a CH340, CP2102, or CP2102N). This chip converts standard serial UART signals (from ESP32 GPIO 43 and 44) into USB signals. This hardware operates completely independently of the ESP32-S3's internal settings.      
- Firmware Flashing: It is the primary port used to upload code to the SoC.
- Serial Monitoring: It provides a reliable connection for debugging and viewing output via the Arduino IDE Serial Monitor.
2. Native USB Port (USB-OTG)      
This port connects directly to the internal USB peripheral of the ESP32-S3 chip. Because the S3 has native USB support, this port allows the chip to interact with a computer in more advanced ways without needing an external bridge chip.
This port is wired directly to the ESP32-S3’s internal USB peripheral (GPIO 19 and 20). This is the port controlled by the ```USB CDC On Boot``` setting.     
- Device Modes: Using the USB API, the ESP32-S3 can act as a variety of USB devices.
  - USB CDC (Communication Device Class): Allows the chip to create a virtual serial port directly.
  - USB MSC (Mass Storage Class): Allows the ESP32-S3 to appear to your computer as a USB flash drive or disk.
  - HID (Human Interface Device): The chip can act as a native keyboard or mouse (though not explicitly listed as an API in these excerpts, it is a standard function of native USB-OTG).     
- USB Host: The sources note that the ESP32-S3 supports a USB Host peripheral, meaning this port can also be used to connect and control other USB devices, such as keyboards or thumb drives.      

In summary, the UART port is your go-to for programming and standard debugging, while the Native USB port is used when you want the ESP32-S3 to function as a specialized USB device or host.     

What this means for your Arduino IDE Settings:      
Since you are using the UART bridge chip, you should ensure your Arduino IDE Tools menu is configured correctly for this path:
- **USB CDC On Boot**: ```Disable``` (Using USB-UART Bridge Chip)
- **Upload Mode**: ```UART0 / Hardware CDC``` (This tells the IDE to send the compiled binary over the UART bridge)
- **USB Mode**: ```Hardware CDC and JTAG``` (Or Disabled, it doesn't strictly matter for the UART bridge, but Hardware CDC is fine).

If **USB CDC On Boot**: ```Enabled``` (Using Native USB)     
You can use the ESP32-S3's advanced native USB features (like making it act as a USB Keyboard/Mouse, USB MIDI, or USB Mass Storage).      

## Voltage Regulator

LM1117 Linear low drop 3.3V 800mA voltage regulator:   
<img alt="image" style="width: 50%; height: auto;" src="https://github.com/user-attachments/assets/353ebae9-632c-4b28-ae44-4cfbb53b6d1c" />

## Schematics

<img width="702" height="594" alt="image" src="https://github.com/user-attachments/assets/3eb0e614-0a6a-4577-8973-cc6ed4ea199a" />

<img width="707" height="795" alt="image" src="https://github.com/user-attachments/assets/2ecc9c32-3bb2-4270-8604-35b382c3f2b3" />

<img width="627" height="611" alt="image" src="https://github.com/user-attachments/assets/ed28a01f-c33a-4fa7-aed1-7380aa46385e" />

## ESP32-S3 Hardware Information      
Here is the complete Arduino code to extract and print all available hardware information from your ESP32-S3.     
This script uses the built-in esp32-hal library functions to read the chip's physical properties, memory allocations, and unique identifiers.     
```cpp
void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial monitor time to open
  
  Serial.println("\n\n========== ESP32-S3 Hardware Information ==========");
  
  // 1. Basic Chip Information
  Serial.printf("Chip Model:        %s\n", ESP.getChipModel());
  Serial.printf("Chip Revision:     %d\n", ESP.getChipRevision());
  Serial.printf("CPU Cores:         %d\n", ESP.getChipCores());
  Serial.printf("CPU Frequency:     %d MHz\n", ESP.getCpuFreqMHz());
  
  // 2. Flash Memory Information
  uint32_t flashSize = ESP.getFlashChipSize();
  Serial.printf("Flash Size:        %d MB (%d bytes)\n", flashSize / (1024 * 1024), flashSize);
  Serial.printf("Flash Speed:       %d MHz\n", ESP.getFlashChipSpeed() / 1000000);
  
  // Map Flash Mode integer to readable string
  int flashMode = ESP.getFlashChipMode();
  const char* flashModeStr = "Unknown";
  switch(flashMode) {
      case 0: flashModeStr = "QIO (Quad I/O)"; break;
      case 1: flashModeStr = "QOUT (Quad Output)"; break;
      case 2: flashModeStr = "DIO (Dual I/O)"; break;
      case 3: flashModeStr = "DOUT (Dual Output)"; break;
      case 4: flashModeStr = "Fast Read"; break;
      case 5: flashModeStr = "Slow Read"; break;
  }
  Serial.printf("Flash Mode:        %s\n", flashModeStr);

  // 3. PSRAM (External RAM) Information
  uint32_t psramSize = ESP.getPsramSize();
  Serial.printf("PSRAM Total Size:  %d MB (%d bytes)\n", psramSize / (1024 * 1024), psramSize);
  Serial.printf("PSRAM Free:        %d bytes\n", ESP.getFreePsram());
  
  // 4. Internal Heap (RAM) Information
  Serial.printf("Total Internal RAM:%d bytes\n", ESP.getHeapSize());
  Serial.printf("Free Internal RAM: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Min Free Heap:     %d bytes\n", ESP.getMinFreeHeap()); // Lowest free heap since boot
  Serial.printf("Max Alloc Heap:    %d bytes\n", ESP.getMaxAllocHeap()); // Largest contiguous block

  // 5. Sketch (Firmware) Information
  Serial.printf("Sketch Size:       %d bytes\n", ESP.getSketchSize());
  Serial.printf("Free Sketch Space: %d bytes\n", ESP.getFreeSketchSpace());

  // 6. Unique Chip ID (Derived from MAC address / eFuse)
  uint64_t chipId = ESP.getEfuseMac();
  // Format as a 12-character hex string (standard MAC format without colons)
  Serial.printf("Chip ID (eFuse MAC): %04X%08X\n", (uint16_t)(chipId >> 32), (uint32_t)chipId);
  
  Serial.println("==================================================\n");
}

void loop() {
  // Put your main code here, to run repeatedly.
  // We only need to run the hardware check once on boot.
}
```
### Troubleshooting / Notes:     
1. PSRAM shows 0 bytes? If PSRAM Total Size prints 0 MB, it means PSRAM is not enabled in your IDE. In the Arduino IDE, go to Tools > PSRAM and change it to OPI PSRAM (since the R8 variant uses Octal SPI, not Quad SPI).
2. Flash Size shows 4MB or 8MB? If the physical flash size prints lower than 16MB, your IDE is restricting it. Go to Tools > Flash Size and ensure it is set to 16MB (128Mb).
3. Internal RAM: The ESP32-S3 has roughly 512KB of internal SRAM. You will see it fluctuates slightly based on what the core initializes on boot.

Below is the ESP32-S3 N16R8     
```
========== ESP32-S3 Hardware Information ==========
Chip Model:        ESP32-S3
Chip Revision:     2
CPU Cores:         2
CPU Frequency:     240 MHz
Flash Size:        16 MB (16777216 bytes)
Flash Speed:       80 MHz
Flash Mode:        QIO (Quad I/O)
PSRAM Total Size:  8 MB (8388608 bytes)
PSRAM Free:        8384788 bytes
Total Internal RAM:392960 bytes
Free Internal RAM: 356348 bytes
Min Free Heap:     351200 bytes
Max Alloc Heap:    294900 bytes
Sketch Size:       283248 bytes
Free Sketch Space: 1310720 bytes
Chip ID (eFuse MAC): 303ED6F61B44
==================================================
```
### What is the suffix N16R8       
For the ESP32-S3, the suffix N16R8 indicates the memory configuration of the module.     
```
Suffix     Meaning
N16        16 MB SPI Flash
R8         8 MB PSRAM (Octal SPI/OPI on most ESP32-S3 modules)
```

- **Flash vs. PSRAM**
  - Flash (16 MB)
    - Stores your program (firmware)
    - Stores filesystem data (LittleFS/SPIFFS)
    - Supports larger OTA updates
  - PSRAM (8 MB)
    - Extra RAM for runtime data
    - Useful for:
      - Large HTML pages
      - JSON documents
      - Display frame buffers
      - Camera images
      - AI/ML models
      - Audio buffers
     
- **Verify in code**
  You can confirm that both the flash and PSRAM are available:
  ```cpp
  Serial.printf("Flash Size : %u MB\n", ESP.getFlashChipSize() / (1024 * 1024));
  Serial.printf("PSRAM Size : %u MB\n", ESP.getPsramSize() / (1024 * 1024));
  Serial.printf("Free Heap  : %u KB\n", ESP.getFreeHeap() / 1024);
  Serial.printf("Free PSRAM : %u KB\n", ESP.getFreePsram() / 1024);
  ```

### Link to more esp.h API:       
https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/Esp.h?





## ESP32-S3 References

https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/index.html

https://documentation.espressif.com/arduino-esp32/en/latest/index.html

https://documentation.espressif.com/en/home

https://github.com/espressif

https://github.com/espressif/arduino-esp32?

### Official IoT Development Framework     
https://www.espressif.com/en/products/sdks/esp-idf     

### ESP-IDF    
https://developer.espressif.com/tags/esp-idf/

https://github.com/espressif/esp-idf

### Up to date technical documents:      
Copy and paste below link on brower:   
*https://www.espressif.com/en/support/documents/technical-documents*   

Hardware Design Guide:    
https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/index.html

https://mischianti.org/vcc-gnd-studio-yd-esp32-s3-devkitc-1-clone-high-resolution-pinout-and-specs/



## 10 ESP32 Pro Features: Take your Projects to the Next Level!
https://www.youtube.com/watch?v=_HzG5FjH_6g


## Resistor Color Band

<img width="635" height="682" alt="image" src="https://github.com/user-attachments/assets/fa5e2931-75d9-4af8-9c2f-e70d87138844" />

### Resistor Calculator   

https://www.calculator.net/resistor-calculator.html


## Circuit Simulators

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


