const int ledPin = 18; // GPIO 18
const int pwmChannel = 0;
const int pwmFrequency = 5000; // 5 kHz
const int pwmResolution = 8;   // 8-bit resolution (0-255)
 
void setup() {
    // Configure PWM
    ledcSetup(pwmChannel, pwmFrequency, pwmResolution);
    // Attach PWM to GPIO
    ledcAttachPin(ledPin, pwmChannel);
}
 
void loop() {
    // Gradually increase brightness
    for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {
        ledcWrite(pwmChannel, dutyCycle);
        delay(15);
    }
    // Gradually decrease brightness
    for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
        ledcWrite(pwmChannel, dutyCycle);
        delay(15);
    }
}
