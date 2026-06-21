# Digital I/O Pin on the ESP32-S3

A Digital Input/Output (I/O) pin—often called a GPIO (General Purpose Input/Output) pin—is a physical pin on the ESP32-S3 microcontroller that can interact with external electronic components.

Unlike analog pins that process a continuous range of voltages, digital pins operate in a binary world. They process two distinct states:

- **HIGH (1)**: Represents the presence of voltage (typically 3.3V on the ESP32-S3).
- **LOW (0)**: Represents the absence of voltage (0V or Ground).

When configured as an **output**, the pin acts as a switch to apply 3.3V or 0V to a component (like turning an LED on or off). When configured as an **input**, the pin reads whether an external component is applying 3.3V or 0V to it (like detecting a button press).

**Pin Modes Configuration**    
The ESP32-S3 is highly flexible. Using the standard Arduino framework, a GPIO pin can be configured into four primary modes using the **pinMode()** function:

- **OUTPUT**: The pin is set to drive a high or low voltage. It has low impedance and can source or sink current (up to about 20mA per pin, though it's best to keep it lower).
- **INPUT**: The pin is set to sample the incoming voltage. It has high impedance, meaning it draws almost no current from the circuit it is measuring. In this state, the pin is "floating" and can randomly flip between HIGH and LOW if left unconnected.
- **INPUT_PULLUP**: The pin is configured as an input but activates an internal resistor connected to 3.3V. This prevents the pin from floating, ensuring it reads HIGH by default until an external switch pulls it down to LOW (Ground).
- **INPUT_PULLDOWN**: The pin is configured as an input but activate an internal resistor connected to ground.

## Code Examples (Arduino IDE)     
Here are practical code examples demonstrating how to use these modes.

**1. Output Mode: Blinking an LED**
This example configures a pin as an output to repeatedly turn an LED on and off.

```
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
```

**2. Input Mode: Reading a Push Button (With Internal Pull-up)**     
Using INPUT_PULLUP is the standard way to read a simple button. When the button is not pressed, the pin reads HIGH. When the button is pressed (connecting the pin to Ground), the pin reads LOW.

```
// Define the GPIO pins
const int BUTTON_PIN = 5;
const int LED_PIN = 4;

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
```
