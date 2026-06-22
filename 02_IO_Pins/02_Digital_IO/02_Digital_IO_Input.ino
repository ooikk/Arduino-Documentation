// Define the GPIO pins
const int BUTTON_PIN = 17;
const int LED_PIN = 15;

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);

  // Configure the button pin with an internal pull-up resistor
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Configure the LED pin as an output
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // Read the digital state of the button pin
  int buttonState = digitalRead(BUTTON_PIN);

  // Because of INPUT_PULLUP, LOW means the button is pressed
  if (buttonState == LOW) {
    digitalWrite(LED_PIN, HIGH);  // Turn on LED
    Serial.println("Button is pressed!");
  } else {
    digitalWrite(LED_PIN, LOW);   // Turn off LED
  }
  
  delay(50); // Small delay to de-noise/debounce slightly
}
