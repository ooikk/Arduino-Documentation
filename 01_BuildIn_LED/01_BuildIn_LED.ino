#include <Adafruit_NeoPixel.h>

#define PIN 48  // Common GPIO for internal RGB on S3

#define NUMPIXELS 1  // Most boards have just one internal LED

#define STEPS 63   // brightness
#define DELAY 250  // 500 mili sec
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int R, G, B;

void setup() {
  Serial.begin(115200);
  pixels.begin();            // INITIALIZE NeoPixel strip object
  pixels.setBrightness(128);  // Set LED brightness, max 255
}

void loop() {
  pixels.clear();  // Set all pixel colors to 'off'

  // pixels.Color(Red, Green, Blue) -> Values 0 to 255
  pixels.setPixelColor(0, pixels.Color(100, 100, 100));  // Bright White
  pixels.show();
  Serial.println("WHITE");
  delay(1500);
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));  // Bright White
  pixels.show();
  Serial.println("OFF");
  delay(1500);

  pixels.setPixelColor(0, pixels.Color(255, 0, 0));  // Bright Red
  pixels.show();                                     // Send the updated color to the hardware
  Serial.println("RED");
  delay(1500);

  pixels.setPixelColor(0, pixels.Color(0, 255, 0));  // Bright Green
  pixels.show();
  Serial.println("GREEN");
  delay(1500);

  pixels.setPixelColor(0, pixels.Color(0, 0, 255));  // Bright Blue
  pixels.show();
  Serial.println("BLUE");
  delay(1500);

  pixels.setPixelColor(0, pixels.Color(255, 255, 255));  // Bright White
  pixels.show();
  Serial.println("WHITE");
  delay(1500);
  R = 0;
  G = 0;
  B = 0;
  do {
    do {

      do {
        pixels.setPixelColor(0, pixels.Color(R, G, B));
        pixels.show();
        Serial.print("**" + String(R) + " " + String(G) + " " + String(B) + " ");
        delay(DELAY);
        R = R + STEPS;
      } while (R < 256);
      Serial.println(" ");
      R = 0;
      G = G + STEPS;
    } while (G < 256);
    G = 0;
    B = B + STEPS;
  } while (B < 256);
  B = 0;
  // esp_deep_sleep_start();
  pixels.setPixelColor(0, pixels.Color(100, 100, 100));  // Bright White
  pixels.show();
  Serial.println("WHITE");
  delay(1500);

  R = 0;
  G = 0;
  B = 0;
  do {
    do {

      do {
        pixels.setPixelColor(0, pixels.Color(B, R, G));
        pixels.show();
        Serial.print("**" + String(R) + " " + String(G) + " " + String(B) + " ");
        delay(DELAY);
        G = G + STEPS;
      } while (G < 256);
      Serial.println(" ");
      G = 0;
      B = B + STEPS;
    } while (B < 256);
    B = 0;
    R = R + STEPS;
  } while (R < 256);

  // esp_deep_sleep_start();
  pixels.setPixelColor(0, pixels.Color(100, 100, 100));  // Bright White
  pixels.show();
  Serial.println("WHITE");
  delay(1500);

  R = 0;
  G = 0;
  B = 0;
  do {
    do {

      do {
        pixels.setPixelColor(0, pixels.Color(G, B, R));
        pixels.show();
        Serial.print("**" + String(R) + " " + String(G) + " " + String(B) + " ");
        delay(DELAY);
        B = B + STEPS;
      } while (B < 256);
      Serial.println(" ");
      B = 0;
      R = R + STEPS;
    } while (R < 256);
    R = 0;
    G = G + STEPS;
  } while (G < 256);

  // esp_deep_sleep_start();
}