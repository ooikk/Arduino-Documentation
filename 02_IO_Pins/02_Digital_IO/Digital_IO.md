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

## GPIO Pins for user     

The ESP32-S3 chip features a total of 45 physical GPIO pins (numbered GPIO 0 to 21, and GPIO 26 to 48).  However, the actual number of "user-usable" pins drops significantly because several pins are hardwired internally to handle critical tasks like flash memory, system booting, and communication.The practical breakdown of available pins depends on how your specific module is configured:      

**1. Pins You Cannot Use (Internal Memory)**     
The ESP32-S3 chip relies on external Flash and PSRAM (Pseudo-Static RAM) encapsulated inside the module (like the standard ESP32-S3-WROOM-1).     
- Quad SPI Flash Modules (Standard): Pins *GPIO 26 through 32 (7 pins)* are permanently wired to the internal SPI flash memory. If you try to use them, the micro-controller will instantly crash or refuse to boot.
- Octal Flash / Octal PSRAM Modules (High Performance): If your specific board uses the high-speed Octal memory variants (like the N8R8 or N16R8), an additional 5 pins (*GPIO 33 through 37*) are consumed by the system.
Available Pins Remaining: ~33 to 38 pins.     

**2. Pins to Use With Caution (Strapping & USB)**      
Several pins are exposed and usable, but have major caveats depending on your board's hardware design.
- Strapping Pins (*GPIO 0, 3, 45, 46*): These control how the chip boots up.
  - *GPIO 0*: Must be HIGH to boot normally into your code (it drops LOW to enter flashing mode). You can use it as an output, but avoid pulling it LOW with external hardware during a reset.     
  - *GPIO 46*: Is an input-only pin and lacks internal pull-up/pull-down resistors.
- Native USB & Debugging (*GPIO 19, 20, 43, 44*): * *GPIO 19* and *20* handle the native USB-OTG/JTAG connection. If your board programs via native USB, using these pins will break your serial connection.
  - *GPIO 43* and *44* are typically tied to the UART0 transceiver for hardware-level flashing and logging.
  
**Summary Table: Truly Safe & Usable GPIOs**     
If you want completely un-restricted, safe pins for sensors, displays, and relays without interfering with memory, booting, or standard USB flashing, you have roughly *23* to *28* "safe" pins available on a standard development board.     

|GPIO Ranges	| Usability Status |	Notes / Limitations |
| - | - | - |
|GPIO 1 to 2, 4 to 18, 21	|🟢 100% Safe|	Excellent general-purpose pins. Many double as ADC (Analog) and Capacitive Touch pins. |
|GPIO 38 to 42	|🟡 Generally Safe|	Fully available on Quad-SPI modules, though frequently routed to onboard peripherals (like camera data/microphones) on specialized modules.
|GPIO 0, 3, 45	|⚠️ Conditional|	Strapping pins. Safe for outputs, but must not be forced to conflicting logic levels during a power cycle/reboot.
|GPIO 46	|⚠️ Conditional|	Strapping pin. Input Only. No internal pull-up/pull-down resistors.
|GPIO 19, 20	|❌ Restricted|	Reserved for native USB-OTG/JTAG functionality.
|GPIO 43, 44	|❌ Restricted|	Typically reserved for Main Hardware Serial (UART0 TX/RX).
|GPIO 26 to 32	|🚫 Never Use|	Dedicated entirely to SPI Flash memory.
|GPIO 33 to 37	|🚫 Never Use (Usually)|	Dedicated to High-speed PSRAM/Flash if using an Octal module variant.

## GPIO API     
**pinMode**    
The **pinMode** function is used to define the GPIO operation mode for a specific pin.
```
void pinMode(uint8_t pin, uint8_t mode);
```
*pin* defines the GPIO pin number.    
*mode* sets operation mode.    

The following modes are supported for the basic input and output:
- *INPUT* sets the GPIO as input without pullup or pulldown (high impedance).
- *OUTPUT* sets the GPIO as output/read mode.
- *INPUT_PULLDOWN* sets the GPIO as input with the internal pulldown.
- *INPUT_PULLUP* sets the GPIO as input with the internal pullup.     

**Internal Pullup and Pulldown**     
The ESP32 SoC families supports the internal pullup and pulldown throught a 45kR resistor, that can be enabled when configuring the GPIO mode as INPUT mode. If the pullup or pulldown mode is not defined, the pin will stay in the high impedance mode.     

**digitalWrite**     
The function **digitalWrite** sets the state of the selected GPIO to *HIGH* or *LOW*. This function is only used if the **pinMode** was configured as *OUTPUT*.     
```
void digitalWrite(uint8_t pin, uint8_t val);
```
- *pin* defines the GPIO pin number.
- *val* set the output digital state to *HIGH* or *LOW*.

**digitalRead**    
To read the state of a given pin configured as INPUT, the function digitalRead is used.     
```
int digitalRead(uint8_t pin);
```
- *pin* select GPIO     

This function will return the logical state of the selected pin as *HIGH* or *LOW*.     

**Interrupts**     
The GPIO peripheral on the ESP32 supports interruptions.

**attachInterrupt**     
The function **attachInterrupt** is used to attach the interrupt to the defined pin.
```
attachInterrupt(uint8_t pin, voidFuncPtr handler, int mode);
```
- *pin* defines the GPIO pin number.
- *handler* set the handler function.
- *mode* set the interrupt mode.     
Here are the supported interrupt modes:
- DISABLED
- RISING
- FALLING
- CHANGE
- ONLOW
- ONHIGH
- ONLOW_WE
- ONHIGH_WE

**attachInterruptArg**     
The function **attachInterruptArg** is used to attach the interrupt to the defined pin using arguments.     
```
attachInterruptArg(uint8_t pin, voidFuncPtrArg handler, void * arg, int mode);
```
- *pin* defines the GPIO pin number.
- *handler* set the handler function.
- *arg* pointer to the interrupt arguments.
- *mode* set the interrupt mode.     
**detachInterrupt**     
To detach the interruption from a specific pin, use the **detachInterrupt** function giving the GPIO to be detached.  
```
detachInterrupt(uint8_t pin);
```
- *pin* defines the GPIO pin number.     

**Example Code: GPIO Interrupt**     
```
#include <Arduino.h>

struct Button {
    const uint8_t PIN;
    uint32_t numberKeyPresses;
    bool pressed;
};

Button button1 = {23, 0, false};
Button button2 = {18, 0, false};

void ARDUINO_ISR_ATTR isr(void* arg) {
    Button* s = static_cast<Button*>(arg);
    s->numberKeyPresses += 1;
    s->pressed = true;
}

void ARDUINO_ISR_ATTR isr() {
    button2.numberKeyPresses += 1;
    button2.pressed = true;
}

void setup() {
    Serial.begin(115200);
    pinMode(button1.PIN, INPUT_PULLUP);
    attachInterruptArg(button1.PIN, isr, &button1, FALLING);
    pinMode(button2.PIN, INPUT_PULLUP);
    attachInterrupt(button2.PIN, isr, FALLING);
}

void loop() {
    if (button1.pressed) {
        Serial.printf("Button 1 has been pressed %lu times\n", button1.numberKeyPresses);
        button1.pressed = false;
    }
    if (button2.pressed) {
        Serial.printf("Button 2 has been pressed %lu times\n", button2.numberKeyPresses);
        button2.pressed = false;
    }
    static uint32_t lastMillis = 0;
    if (millis() - lastMillis > 10000) {
      lastMillis = millis();
      detachInterrupt(button1.PIN);
    }
}
```

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
## References    

https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/gpio.html

https://randomnerdtutorials.com/esp32-digital-inputs-outputs-arduino/

https://www.oceanlabz.in/esp32-inputs-outputs/

https://deepbluembedded.com/esp32-digital-inputs-outputs-arduino/

https://docs.arduino.cc/language-reference/en/functions/digital-io/pinMode/
