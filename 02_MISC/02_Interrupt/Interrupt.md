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
**Without Spinlock code:**     
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

## 2. Hardware Timer Interrupts     

The ESP32-S3 contains 4 internal 64-bit hardware timers. Using the modern Arduino ESP32 Core v3.x API (timerBegin), you can create non-blocking, microsecond-accurate periodic interrupts.     

Use Case: Blinking an LED without delay(), reading sensors at exact intervals, or creating a precise system clock.
```
hw_timer_t * myTimer = NULL;
volatile bool timerFlag = false;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// ISR for Timer
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL(&timerMux);
  timerFlag = true;
  portEXIT_CRITICAL(&timerMux);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  // Initialize timer with a frequency of 1,000,000 Hz (1 MHz)
  myTimer = timerBegin(1000000); 
  
  // Attach the ISR to the timer
  timerAttachInterrupt(myTimer, &onTimer);
  
  // Set alarm: 1,000,000 ticks at 1MHz = 1 second. 
  // Parameters: timer, count, autoreload, reload_count
  timerAlarm(myTimer, 1000000, true, 0); 
}

void loop() {
  if (timerFlag) {
    portENTER_CRITICAL(&timerMux);
    timerFlag = false;
    portEXIT_CRITICAL(&timerMux);
    
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    Serial.println("1 Second Passed (Timer Interrupt)");
  }
}
```
**Without Spinlock code:**     
```
// Hardware Timer Interrupt Example (Toggling LED every 1 sec)
#define LED_PIN 38

hw_timer_t *timer = NULL;
volatile bool timerFlag = false;

void ARDUINO_ISR_ATTR onTimer() {
  timerFlag = true;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // Set timer frequency to 1 MHz (1 tick = 1 microsecond)
  timer = timerBegin(1000000);

  // Attach interrupt handler
  timerAttachInterrupt(timer, &onTimer);

  // Trigger alarm every 1,000,000 µs (1 second), with auto-reload (true)
  timerAlarm(timer, 1000000, true, 0);
}

void loop() {
  if (timerFlag) {
    timerFlag = false;
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    Serial.println("Timer interrupt fired!");
  }
}
```

## 3. Capacitive Touch Interrupts     

The ESP32-S3 supports native capacitive touch on specific touch-capable pins (e.g., TOUCH1–TOUCH14). You can trigger an interrupt when the capacitance drops below a designated threshold (when a finger touches the pin).      

```
// On ESP32-S3, Touch pins are typically GPIO 1 to 14. Let's use GPIO 4 (Touch 4)
const int touchPin = 4; 
volatile bool touchDetected = false;
portMUX_TYPE touchMux = portMUX_INITIALIZER_UNLOCKED;

// ISR for Touch
void IRAM_ATTR onTouch() {
  portENTER_CRITICAL(&touchMux);
  touchDetected = true;
  portEXIT_CRITICAL(&touchMux);
}

void setup() {
  Serial.begin(115200);
  
  // Attach touch interrupt. 
  // The last parameter (40) is the threshold. 
  // You may need to adjust this based on your physical hardware.
  touchAttachInterrupt(touchPin, onTouch, 40); 
}

void loop() {
  if (touchDetected) {
    portENTER_CRITICAL(&touchMux);
    touchDetected = false;
    portEXIT_CRITICAL(&touchMux);
    
    Serial.println("Touch Pad Interrupt Triggered!");
    
    // Optional: Read the actual value to see how far it crossed the threshold
    int touchValue = touchRead(touchPin);
    Serial.printf("Touch Value: %d\n", touchValue);
  }
}
```
**Without Spinlock code:**     
```
// Touch Pad Interrupt Example
const int TOUCH_PIN = 4; // Touch-capable pin (e.g., TOUCH4 / GPIO4)
const int TOUCH_THRESHOLD = 30000; // Adjust threshold based on touchRead() raw output
volatile bool touchDetected = false;

void ARDUINO_ISR_ATTR gotTouched() {
  touchDetected = true;
}

void setup() {
  Serial.begin(115200);
  
  // Attach touch interrupt with callback function and threshold value
  touchAttachInterrupt(TOUCH_PIN, gotTouched, TOUCH_THRESHOLD);
}

void loop() {
  if (touchDetected) {
    touchDetected = false;
    Serial.println("Capacitive Touch Interrupt Triggered!");
  }
}
```
## 4. RTC GPIO / Deep Sleep Wake-up Interrupts      

The ESP32-S3 has a Real-Time Clock (RTC) subsystem that remains powered on even in Deep Sleep mode. Specific GPIO pins (RTC GPIOs, typically GPIO 0 through 21 on the S3) can be configured to wake the chip up from Deep Sleep via an interrupt.     

Use Case: Battery-powered devices that need to sleep to save power and wake up only when a physical button is pressed or a sensor triggers.    
```
#include <esp_sleep.h>

// GPIO 0 is the BOOT button on most dev boards and is an RTC GPIO on S3
const int wakeupPin = 0; 

void setup() {
  Serial.begin(115200);
  
  // Configure the pin
  pinMode(wakeupPin, INPUT_PULLUP);

  // Check if we woke up from deep sleep
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Woke up from Deep Sleep via RTC GPIO Interrupt!");
  } else {
    Serial.println("Normal Boot (First time startup)");
  }

  Serial.println("Going to Deep Sleep in 2 seconds...");
  delay(2000);
  
  // Enable EXT0 wake up source. 
  // Wake up when the pin goes LOW.
  esp_sleep_enable_ext0_wakeup((gpio_num_t)wakeupPin, LOW);

  // Enter deep sleep
  esp_deep_sleep_start();
  
  // Code below will never be reached because the ESP32 resets upon waking up
  Serial.println("This will never print"); 
}

void loop() {
  // Nothing to do here
}
```

## 5. Hardware Peripheral / Software Interrupts (FreeRTOS)      

```
// Advanced Hardware-to-Software Task Interrupt Example
#include <Arduino.h>

const uint8_t SENSOR_PIN = 5;
SemaphoreHandle_t xBinarySemaphore;

void ARDUINO_ISR_ATTR sensorISR() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  // Give semaphore to unblock high-priority background task
  xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

void ProcessingTask(void *pvParameters) {
  for (;;) {
    // Wait for the interrupt to release the semaphore
    if (xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE) {
      Serial.println("Interrupt deferred to background FreeRTOS Task!");
      // Perform complex calculations or network calls safely here
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_PIN, INPUT_PULLUP);

  xBinarySemaphore = xSemaphoreCreateBinary();

  // Create a processing task pinned to Core 1
  xTaskCreatePinnedToCore(ProcessingTask, "ProcessingTask", 2048, NULL, 3, NULL, 1);

  // Attach hardware interrupt
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), sensorISR, FALLING);
}

void loop() {
  // Main loop remains completely unblocked
}
```
