// Define the GPIO pin connected to the LED
const int LED_PIN_R = 15; 
const int LED_PIN_B = 16; 

void setup() {
  // Configure the pin as an OUTPUT
  pinMode(LED_PIN_R, OUTPUT);
  pinMode(LED_PIN_B, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN_R, HIGH); // Turn the LED on (Output 3.3V)
  digitalWrite(LED_PIN_B, LOW); // Turn the LED on (Output 3.3V)
  delay(1000);                 // Wait for 1 second
  
  digitalWrite(LED_PIN_R, LOW);  // Turn the LED off (Output 0V)
  digitalWrite(LED_PIN_B, HIGH); // Turn the LED on (Output 3.3V)  
  delay(1000);                 // Wait for 1 second
}
