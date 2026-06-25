#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET -1     // Reset pin # (or -1 if sharing Arduino reset pin)

#define ESP32S3

#ifdef ESP32S3
#define OLED_SDA_PIN 8  // Default: GPIO 8, can be assigned to any pin
#define OLED_SCL_PIN 9  // Default: GPIO 9
#endif


// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


void setup() {
  Serial.begin(115200);
  delay(1000);
#ifdef ESP32S3
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);  // define I2C pins
#endif
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
  Serial.println("Hello world.");
}

void loop() {
  display.setCursor(0, 0);
  display.print("Hello World!");
  display.display();
  delay(2000);
  display.clearDisplay();
  display.display();
  Serial.println("Hello world.");
  delay(1000);
}