// Define the GPIO pin connected to the LED
const int LED_PIN = 4; 

void setup() {
  // Configure the pin as an OUTPUT
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH); // Turn the LED on (Output 3.3V)
  delay(1000);                 // Wait for 1 second
  
  digitalWrite(LED_PIN, LOW);  // Turn the LED off (Output 0V)
  delay(1000);                 // Wait for 1 second
}
