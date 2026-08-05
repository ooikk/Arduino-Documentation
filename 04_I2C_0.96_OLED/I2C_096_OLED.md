# 0.96" I2C OLED SDD1306

OLEDs are one of the types of screens available for use in our electronics and Arduino projects.

An OLED (Organic light-emitting diode) is a type of LED in which the emissive layer is made of an organic compound that emits light in response to electricity.

Like other types of displays, OLEDs need a specific controller that converts the received data into electronic signals to control the display.

In the field of home electronics, small 0.96” OLED displays are commercially available, ready to connect to Arduino. These displays incorporate the SDD1306 controller and have a very small size of 25mm x 14mm. They are monochrome and have a resolution of 128x64 pixels.

OLED screens have the advantage of very low power consumption, around 20mA, since only the necessary pixels are illuminated and they don't require a backlight. This is especially useful in battery-powered applications.

Furthermore, they offer better visibility in bright environments, such as sunlight. However, the small size of these 0.96” OLED screens can pose a challenge for proper viewing.

Depending on the model, communication can be via SPI or I2C bus, making it easy to obtain measured data. The supply voltage supports 3.3V and 5V.

These types of OLED screens are interesting for projects where we can take advantage of their small size. For other applications, we'll generally prefer an LCD display like the Nokia 5110 or a TFT screen.

<img width="333" height="334" alt="image" src="https://github.com/user-attachments/assets/070bcd75-011a-4365-b36e-caba64e9435b" />
<img width="333" height="334" alt="image" src="https://github.com/user-attachments/assets/56e074fd-2923-4a98-989f-c63f20203461" />

## Connect to OLED

```
0.96 OLED        ESP32 S3
GND              GND
VCC              3.3V
SDA              GPIO 8    // default I2C
SCL              GPIO 9    // default I2C
```

## Wiring Diagram

<img width="580" height="587" alt="image" src="https://github.com/user-attachments/assets/9dc00342-cde8-47ed-a046-8a25d050572c" />

## Library for OLED

Install libraries by Adafruit:
1. Adafruit SSD1306  
   https://github.com/adafruit/Adafruit_SSD1306
2. Adafruit GFX Library  
   https://github.com/adafruit/Adafruit-GFX-Library
3. Adafruit BusIO  
   https://github.com/adafruit/Adafruit_BusIO

## Code Initialization for Adafruit Library

```cpp
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET -1     // Reset pin # (or -1 if sharing Arduino reset pin)

#define OLED_SDA_PIN 8  // Default: GPIO 8, can be assigned to any pin
#define OLED_SCL_PIN 9  // Default: GPIO 9

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);  // define I2C pins

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Hello World!");
  display.display();
}

void loop() {
}
```

## Display Bitmap Images in the OLED

Use this tool to convert image to 565RGB array, decode as 1-bit line art.

https://mischianti.org/rgb-image-to-byte-array-converter-for-arduino-tft-displays/

<img width="719" height="617" alt="image" src="https://github.com/user-attachments/assets/ee1aaeea-206d-403c-9494-dceedff6c356" />

Sample code:

```cpp
:
:
static const uint8_t imageArray[] PROGMEM  = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
:
:
  0x1d, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x2f, 0x4f, 0xf0, 0x00, 
  0x03, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 0xff, 0xff, 0x00
};
:
:
display.drawBitmap(0, 0, imageArray, 128, 64, 1);
```

## Reference

https://www.oceanlabz.in/project-3-oled-display-with-esp32-displaying-sensor-data/

https://www.luisllamas.es/conectar-arduino-a-una-pantalla-oled-de-0-96/

https://randomnerdtutorials.com/esp32-ssd1306-oled-display-arduino-ide/
