# Interrupts

The ESP32-S3 features a robust interrupt architecture powered by its Xtensa® dual-core LX7 processor. In the Arduino IDE (using Arduino ESP32 Core v3.x+), interrupts are generally grouped into four practical categories: 
1.	GPIO External Interrupts (Hardware state changes on pins) 
2.	Hardware Timer Interrupts (Precise microsecond or millisecond schedules) 
3.	Capacitive Touch Interrupts (Touch-sensitive GPIOs) 
4.	Peripheral & IPC Interrupts (UART, SPI, Software/Inter-Core, I2C)       

**Key Golden Rules for ESP32-S3 ISRs**      
When writing Interrupt Service Routines (ISRs) on the ESP32-S3:     
- **ARDUINO_ISR_ATTR / IRAM_ATTR:** Always place this macro before your ISR function so the code is compiled directly into fast Internal RAM (IRAM) instead of Flash memory. 
- **volatile:** Mark variables shared between the main code loop and the ISR as volatile to prevent compiler optimization bugs. 
- **Keep ISRs Short:** Avoid lengthy logic, non-reentrant calls like delay(), or direct Serial.print() inside the ISR. Set a flag or use a queue/semaphore instead.  
- **Use Spinlocks for shared data**: Because the ESP32-S3 is dual-core, use portMUX_TYPE to protect shared variables from being corrupted if the ISR and main loop run simultaneously on different cores.     

## 1. GPIO / External Hardware Interrupts      
These are triggered by a change in the state of a standard GPIO pin (e.g., a button press). You can trigger them on ```RISING```, ```FALLING```, ```CHANGE```, ```LOW```, or ```HIGH``` states.      
Use Case: Reading physical buttons, rotary encoders, or external sensor alerts.

```
const int buttonPin = 2; // Any standard GPIO pin
volatile int interruptCounter = 0;
// Spinlock to protect the variable across dual cores
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED; 

// ISR must have IRAM_ATTR
void IRAM_ATTR handleButtonInterrupt() {
  portENTER_CRITICAL(&mux);
  interruptCounter++;
  portEXIT_CRITICAL(&mux);
}

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  
  // Attach the interrupt. On ESP32, you pass the pin number directly.
  attachInterrupt(buttonPin, handleButtonInterrupt, FALLING);
}

void loop() {
  if(interruptCounter > 0) {
    portENTER_CRITICAL(&mux);
    int count = interruptCounter;
    interruptCounter = 0; // Reset counter
    portEXIT_CRITICAL(&mux);
    
    Serial.printf("Button pressed! Count: %d\n", count);
  }
}
```
Without Spinlock code:     
```
// GPIO External Interrupt Example
const uint8_t BUTTON_PIN = 4; // GPIO4 connected to a button
volatile bool buttonPressed = false;

// ISR placed in IRAM for immediate response time
void ARDUINO_ISR_ATTR handleButtonPress() {
  buttonPressed = true; // Minimal logic inside ISR
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Attach interrupt to GPIO (Triggers on FALLING edge)
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);
}

void loop() {
  if (buttonPressed) {
    buttonPressed = false; // Reset flag
    Serial.println("Button interrupt triggered!");
  }
}
```
