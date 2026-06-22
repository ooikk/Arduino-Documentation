// Define the GPIO pin
const int LED_PIN_R = 15;
const int LED_PIN_B = 16;

// PWM Properties
const int PWM_FREQ = 5000;    // 5 kHz frequency
/*  Old API
const int PWM_CHANNEL = 0;   // Use PWM channel 0 (0-7 available)
*/
const int PWM_RES = 8;       // 8-bit resolution (0 - 255 values)

void setup() {
/* Old API
  // Configure the LEDC PWM channel with frequency and resolution
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  
  // Attach the physical GPIO pin to the configured PWM channel
  ledcAttachPin(LED_PIN, PWM_CHANNEL);
*/

// New API: Automatically configures an underlying channel and attaches the pin
  ledcAttach(LED_PIN_R, PWM_FREQ, PWM_RES);
  ledcAttach(LED_PIN_B, PWM_FREQ, PWM_RES);  
}

void loop() {
  // Fade inside loop (increasing brightness)
  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {
/* Old API
    ledcWrite(PWM_CHANNEL, dutyCycle);
*/
  // Pass the PIN directly to ledcWrite instead of the channel
    ledcWrite(LED_PIN_R, dutyCycle);
    ledcWrite(LED_PIN_B, 255-dutyCycle);
    delay(5);
  }

  // Fade outside loop (decreasing brightness)
  for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
/* Old API
    ledcWrite(PWM_CHANNEL, dutyCycle);
*/
  // Pass the PIN directly to ledcWrite instead of the channel
    ledcWrite(LED_PIN_R, dutyCycle);
    ledcWrite(LED_PIN_B, 255-dutyCycle);    
    delay(5);
  }
}
