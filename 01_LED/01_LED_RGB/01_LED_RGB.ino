// Define the GPIO pin
const int LED_PIN_R = 15;  // LED pin 1
const int LED_PIN_G = 16;  // LED pin 3
const int LED_PIN_B = 17;  // LED pin 4

// PWM Properties
const int PWM_FREQ = 5000;  // 5 kHz frequency
/*  Old API
const int PWM_CHANNEL = 0;   // Use PWM channel 0 (0-7 available)
*/
const int PWM_RES = 8;  // 8-bit resolution (0 - 255 values)

void setup() {
  Serial.begin(115200);
  // New API: Automatically configures an underlying channel and attaches the pin
  ledcAttach(LED_PIN_R, PWM_FREQ, PWM_RES);
  ledcAttach(LED_PIN_G, PWM_FREQ, PWM_RES);
  ledcAttach(LED_PIN_B, PWM_FREQ, PWM_RES);
}

void loop() {

  ledcWrite(LED_PIN_R, 128);
  ledcWrite(LED_PIN_G, 0);
  ledcWrite(LED_PIN_B, 0);
  Serial.println("RED Light");
  delay(1000);
  ledcWrite(LED_PIN_R, 0);
  ledcWrite(LED_PIN_G, 128);
  ledcWrite(LED_PIN_B, 0);
  Serial.println("GREEN Light");
  delay(1000);
  ledcWrite(LED_PIN_R, 0);
  ledcWrite(LED_PIN_G, 0);
  ledcWrite(LED_PIN_B, 128);
  Serial.println("BLUE Light");
  delay(1000);
  Serial.println("**Fading Light**");
  // Fade inside loop (increasing brightness)
  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {

    // Pass the PIN directly to ledcWrite instead of the channel
    ledcWrite(LED_PIN_R, dutyCycle);
    ledcWrite(LED_PIN_G, 255 - dutyCycle);
    ledcWrite(LED_PIN_B, 255 - dutyCycle);
    delay(5);
  }

  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {

    // Pass the PIN directly to ledcWrite instead of the channel
    ledcWrite(LED_PIN_R, 255 - dutyCycle);
    ledcWrite(LED_PIN_G, dutyCycle);
    ledcWrite(LED_PIN_B, 255 - dutyCycle);
    delay(5);
  }

  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {

    // Pass the PIN directly to ledcWrite instead of the channel
    ledcWrite(LED_PIN_R, 255 - dutyCycle);
    ledcWrite(LED_PIN_G, 255 - dutyCycle);
    ledcWrite(LED_PIN_B, dutyCycle);
    delay(5);
  }
  // Fade outside loop (decreasing brightness)
  for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {

    // Pass the PIN directly to ledcWrite instead of the channel
    ledcWrite(LED_PIN_R, dutyCycle);
    ledcWrite(LED_PIN_G, 255 - dutyCycle);
    ledcWrite(LED_PIN_B, 255 - dutyCycle);
    delay(5);
  }
  for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
    // Pass the PIN directly to ledcWrite instead of the channel
    ledcWrite(LED_PIN_R, 255 - dutyCycle);
    ledcWrite(LED_PIN_G, dutyCycle);
    ledcWrite(LED_PIN_B, 255 - dutyCycle);
    delay(5);
  }
  for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
    // Pass the PIN directly to ledcWrite instead of the channel
    ledcWrite(LED_PIN_R, 255 - dutyCycle);
    ledcWrite(LED_PIN_G, 255 - dutyCycle);
    ledcWrite(LED_PIN_B, dutyCycle);
    delay(5);
  }
}
