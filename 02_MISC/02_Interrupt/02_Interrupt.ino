// GPIO External Interrupt Example with debouncing
const uint8_t BUTTON_PIN = 4;  // GPIO4 connected to a button
volatile bool buttonPressed = false;

// ISR placed in IRAM for immediate response time
void ARDUINO_ISR_ATTR handleButtonPress() {
  //buttonPressed = true; // Minimal logic inside ISR
  // handle debouncing
  static unsigned long lastInterruptTime = 0;

  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > 200) {  // 200 ms debounce time
    buttonPressed = true;
  }
  lastInterruptTime = interruptTime;
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Attach interrupt to GPIO (Triggers on FALLING edge)
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);
}

void loop() {
  if (buttonPressed) {
    buttonPressed = false;  // Reset flag
    Serial.println("Button interrupt triggered!");
  }
}
