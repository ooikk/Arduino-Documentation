// Define the GPIO pin connected to the LED
const int LED_PIN_R = 15;
const int LED_PIN_B = 16;
const int RESOLUTION = 8;
const int FREQUENCY = 5000;

void setup() {
  Serial.begin(115200);
  // Configure the pin as an OUTPUT
  pinMode(LED_PIN_R, OUTPUT);
  pinMode(LED_PIN_B, OUTPUT);
  analogWriteResolution(LED_PIN_R, RESOLUTION);
  analogWriteResolution(LED_PIN_B, RESOLUTION);
  analogWriteFrequency(LED_PIN_R, FREQUENCY);
  analogWriteFrequency(LED_PIN_B, FREQUENCY);
  Serial.println("Start analogWrite..");
}

void loop() {
  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {

    analogWrite(LED_PIN_R, dutyCycle);
    analogWrite(LED_PIN_B, 255 - dutyCycle);
    delay(5);
  }

  for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
    analogWrite(LED_PIN_R, dutyCycle);
    analogWrite(LED_PIN_B, 255 - dutyCycle);
    delay(5);
  }
}
