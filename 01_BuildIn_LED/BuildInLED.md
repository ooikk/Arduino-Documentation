# Built-in WS2812B RGB LED

Unlike the original ESP32, which often featured a simple red LED on GPIO 2, most ESP32-S3 development boards (including the DevKitC-1 v1.1) use a built-in WS2812B RGB LED (commonly known as a NeoPixel).

This means you cannot turn it on with a simple digitalWrite(HIGH). Instead, you must use a library that communicates with the LED's specific single-wire timing protocol.

## 1. Hardware Details
   - LED Type: WS2812B Addressable RGB LED.
   - Standard Pin: On the ESP32-S3-DevKitC-1, the LED is internally connected to GPIO 48.
   - Power: It shares the board's 3.3V or 5V rail (depending on the specific board version), but the data signal is 3.3V.

## 2. Recommended Libraries
   - There are two primary industry-standard libraries for controlling this LED:

```
Library                Best For                                               Complexity
Adafruit NeoPixel      Ease of use and quick setup.                           Low
FastLED                Advanced animations and high-performance color math.   Moderate
```
To install these, go to **Sketch > Include Library > Manage Libraries**... and search for their names.

## Adafruit NeoPixel

Import Library: Adafruit_NeoPixel, 1.15.4 by Adafruit

https://github.com/adafruit/Adafruit_NeoPixel

Basic Setup and Control Code:

```
#include <Adafruit_NeoPixel.h>

#define LED_PIN    48   // Standard GPIO for S3 built-in RGB
#define NUM_PIXELS 1    // Most S3 boards have only 1 built-in LED

// Initialize the LED object
Adafruit_NeoPixel builtInLED(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  builtInLED.begin();           // Initialize the LED
  builtInLED.setBrightness(50); // Set brightness (0-255)
}

void loop() {
  // Set to Red
  setLEDColor(255, 0, 0); 
  delay(1000);

  // Set to Green
  setLEDColor(0, 255, 0);
  delay(1000);

  // Set to Blue
  setLEDColor(0, 0, 255);
  delay(1000);
}

// Helper function to update the color
void setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
  builtInLED.setPixelColor(0, builtInLED.Color(r, g, b));
  builtInLED.show(); // Required to push the data to the LED
}
```

## FastLED

Import Library: FastLED, 3.10.3 by Daniel Garcia

https://github.com/FastLED/FastLED

If you require smoother color transitions or are building a complex project, FastLED is the professional choice. It is highly optimized for the ESP32-S3 architecture.

Sample Function for FastLED:

```
#include <FastLED.h>

#define DATA_PIN 48
CRGB leds[1];

void setup() {
  // ESP32-S3 requires specific timing provided by the WS2812B controller
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, 1);
}

void loop() {
  // Use HSV for easy color cycling (Hue, Saturation, Value)
  static uint8_t hue = 0;
  leds[0] = CHSV(hue++, 255, 255); 
  FastLED.show();
  delay(20);
}
```

## Pro-Tip: Power Management

Addressable LEDs can be quite bright and draw significant current.

- **Heat:** Running the built-in LED at full brightness (255, 255, 255) for long periods can heat up the ESP32-S3 chip area.
- **Current:** If you add an external strip of these LEDs, do not power them directly from the ESP32-S3's 3.3V pin; use an external 5V power supply to avoid damaging the board's voltage regulator.
