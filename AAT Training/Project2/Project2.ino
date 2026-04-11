#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int pirPin = D5;
int detectionState = 0;
unsigned long lastMotion = 0;

void setup() {
  pinMode(pirPin, INPUT);
  Serial.begin(115200);
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
  display.println("Room Status");
  display.display();
}

void loop() {
  detectionState = digitalRead(pirPin);
  if (detectionState == 1) {
    lastMotion = millis();
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Room Status");
    display.print("Occupied");
    display.display();
  }

  if ((millis() - lastMotion) > 10000) {

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Room Status");
    display.print("Empty");
    display.display();
  }
}
