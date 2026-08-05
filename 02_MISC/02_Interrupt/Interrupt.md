# Interrupts

The ESP32-S3 features a robust interrupt architecture powered by its Xtensa® dual-core LX7 processor. In the Arduino IDE (using Arduino ESP32 Core v3.x+), interrupts are generally grouped into four practical categories:

1. GPIO External Interrupts (Hardware state changes on pins)
2. Hardware Timer Interrupts (Precise microsecond or millisecond schedules)
3. Capacitive Touch Interrupts (Touch-sensitive GPIOs)
4. Peripheral & IPC Interrupts (UART, SPI, Software/Inter-Core, I2C)

## Key Golden Rules for ESP32-S3 ISRs

When writing Interrupt Service Routines (ISRs) on the ESP32-S3:

- **ARDUINO_ISR_ATTR / IRAM_ATTR:** Always place this macro before your ISR function so the code is compiled directly into fast Internal RAM (IRAM) instead of Flash memory.
- **volatile:** Mark variables shared between the main code loop and the ISR as `volatile` to prevent compiler optimization bugs.
- **Keep ISRs Short:** Avoid lengthy logic, non-reentrant calls like `delay()`, or direct `Serial.print()` inside the ISR. Set a flag or use a queue/semaphore instead.
- **Use Spinlocks for shared data:** Because the ESP32-S3 is dual-core, use `portMUX_TYPE` to protect shared variables from being corrupted if the ISR and main loop run simultaneously on different cores.

## ARDUINO_ISR_ATTR and IRAM_ATTR

In the context of ESP32 development, `ARDUINO_ISR_ATTR` and `IRAM_ATTR` achieve the exact same mechanical result, but they come from different abstraction layers.

### The Fundamental Difference

| Attribute | IRAM_ATTR | ARDUINO_ISR_ATTR |
| - | - | - |
| Origin | ESP-IDF (Espressif's native C/C++ SDK) | Arduino ESP32 Core (Wrapper built on top of ESP-IDF) |
| Primary Scope | Used for any function/variable forced into IRAM | Specifically used for Interrupt Service Routines (ISRs) |
| Portability | Compatible with pure ESP-IDF and Arduino IDE | Preferred semantic standard inside the Arduino API |
| Under the Hood | `__attribute__((section(".iram1." ...)))` | Evaluates directly to `IRAM_ATTR` via macro |

### Why Do We Put Functions in IRAM?

By default, ESP32 compiles code into standard Flash Memory (IROM).

When an interrupt fires, the CPU halts what it's doing and immediately executes the Interrupt Service Routine (ISR). If that ISR is stored in Flash:

- The CPU has to retrieve the code via the Flash MMU cache.
- If there is a cache miss, or if Flash memory is currently busy write-blocking (e.g., saving preferences or using Wi-Fi), the ISR will experience a delay or crash.

Marking a function with either attribute forces the compiler to store the ISR directly in the ESP32-S3's Internal Fast Static RAM (IRAM), guaranteeing ultra-low latency execution.

### Which One Should You Use?

- Use `ARDUINO_ISR_ATTR` if you are writing standard Arduino IDE sketches for ESP32. It signals clear intent that the function is explicitly an ISR.
- Use `IRAM_ATTR` if you are writing cross-platform C/C++ code that might be compiled directly under ESP-IDF or if you are placing non-ISR performance-critical functions into RAM.

## API references

### 1. GPIO External Interrupt Functions

Used to trigger an interrupt when a physical pin changes state.

**`attachInterrupt()`:** Attaches an ISR callback to a specific GPIO pin.

```cpp
void attachInterrupt(uint8_t pin, void (*userFunc)(void), int mode);
```

- `pin`: The GPIO pin number (or `digitalPinToInterrupt(pin)`).
- `userFunc`: The name of your ISR function.
- `mode`: The trigger condition (`RISING`, `FALLING`, `CHANGE`, `LOW`, `HIGH`).

**`attachInterruptArg()`:** Attaches an ISR callback to a GPIO pin while allowing custom arguments to be passed into the callback function.

```cpp
void attachInterruptArg(uint8_t pin, void (*userFunc)(void*), void* arg, int mode);
```

- `arg`: A `void` pointer to pass custom parameters/structs to the ISR.

**`detachInterrupt()`:** Disables and detaches the interrupt handler from a given GPIO pin.

```cpp
void detachInterrupt(uint8_t pin);
```

- `pin`: The GPIO pin to detach.

### 2. Hardware Timer Interrupt Functions

These functions control the hardware timers available on the ESP32.

**`timerBegin()`:** Initializes a hardware timer with a given frequency.

```cpp
hw_timer_t* timerBegin(uint32_t frequency);
```

- `frequency`: Timer tick frequency in Hz (e.g., `1000000` sets 1 tick = 1 microsecond).
- Returns: Pointer handle to the timer object (`hw_timer_t*`).

**`timerAttachInterrupt()`:** Binds an ISR callback function to a designated hardware timer.

```cpp
void timerAttachInterrupt(hw_timer_t* timer, void (*userFunc)(void));
```

- `timer`: The timer object pointer created by `timerBegin()`.
- `userFunc`: Pointer to the function to execute when the alarm triggers.

**`timerDetachInterrupt()`:** Detaches the interrupt callback from a running timer.

```cpp
void timerDetachInterrupt(hw_timer_t* timer);
```

**`timerAlarm()`:** Configures the threshold at which the timer triggers an interrupt.

```cpp
void timerAlarm(hw_timer_t* timer, uint64_t alarm_value, bool autoreload, uint64_t reload_count);
```

- `alarm_value`: Trigger threshold in timer ticks.
- `autoreload`: `true` for periodic/repeating timer, `false` for one-shot.
- `reload_count`: Number of auto-reloads (0 for unlimited repeats).

**`timerStart()` / `timerStop()` / `timerRestart()`:** Manually control the run state of a timer.

```cpp
void timerStart(hw_timer_t* timer);
void timerStop(hw_timer_t* timer);
void timerRestart(hw_timer_t* timer); // Resets counter value back to zero
```

**`timerEnd()`:** Stops the hardware timer and frees up the memory resource.

```cpp
void timerEnd(hw_timer_t* timer);
```

### 3. Capacitive Touch Interrupt Functions

Touch-capable GPIO pins can fire an interrupt when human touch is detected.

**`touchAttachInterrupt()`:** Attaches an ISR to a capacitive touch pin that triggers when capacitance drops below a set threshold.

```cpp
void touchAttachInterrupt(uint8_t pin, void (*userFunc)(void), uint16_t threshold);
```

- `pin`: Touch-capable GPIO pin number.
- `userFunc`: Callback function executed on touch detection.
- `threshold`: Sensitivity limit (triggers when `touchRead(pin)` drops below or meets this value).

**`touchDetachInterrupt()`:** Disables touch interrupt capabilities on the specified pin.

```cpp
void touchDetachInterrupt(uint8_t pin);
```

### 4. Interrupt State Management & Safety Macros

Functions and macros used to prevent concurrency issues or race conditions between the main code thread and an ISR.

**`noInterrupts()` / `interrupts()`:** Globally pause and resume interrupt execution across the microcontroller.

```cpp
noInterrupts(); // Disables all maskable interrupts (creates a critical section)
// ... performs sensitive read/write of a variable shared with an ISR ...
interrupts();   // Re-enables interrupts
```

**Critical Section Macros (`portMUX_TYPE`):**

For dual-core ESP32 chips, `noInterrupts()` is often not enough because another core can still modify memory. The ESP32 task-spinlock functions lock access across both cores:

```cpp
portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;

// Enter critical section (disables interrupts on calling core & locks mutex)
taskENTER_CRITICAL(&myMutex);
// ... safely edit shared variable ...
taskEXIT_CRITICAL(&myMutex);
```

```cpp
// Inside an ISR context (use FromISR variant)
taskENTER_CRITICAL_ISR(&myMutex);
// ... manipulate shared hardware state ...
taskEXIT_CRITICAL_ISR(&myMutex);
```

## 1. GPIO / External Hardware Interrupts

These are triggered by a change in the state of a standard GPIO pin (e.g., a button press). You can trigger them on `RISING`, `FALLING`, `CHANGE`, `LOW`, or `HIGH` states.

**Use Case:** Reading physical buttons, rotary encoders, or external sensor alerts.

```cpp
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

```cpp
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

### Debouncing

When dealing with mechanical buttons, it is common to encounter noise or "bounce" when the button is pressed or released. This can cause multiple interrupts to be triggered for a single button press. Debouncing can be handled in software by adding a small delay or by using a timer.

**Example of Software Debouncing:**

```cpp
void IRAM_ATTR handleButtonPress() {
  static unsigned long lastInterruptTime = 0;
 
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > 200) {  // 200 ms debounce time
    buttonPressed = true;
  }
  lastInterruptTime = interruptTime;
}
```

## 2. Hardware Timer Interrupts

The ESP32-S3 contains 4 internal 54-bit hardware timers. Using the modern Arduino ESP32 Core v3.x API (`timerBegin`), you can create non-blocking, microsecond-accurate periodic interrupts.

**Use Case:** Blinking an LED without `delay()`, reading sensors at exact intervals, or creating a precise system clock.

```cpp
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

```cpp
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

```cpp
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

```cpp
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

**Use Case:** Battery-powered devices that need to sleep to save power and wake up only when a physical button is pressed or a sensor triggers.

```cpp
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

```cpp
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

## Why digitalPinToInterrupt(interruptPin)?

Using `detachInterrupt(digitalPinToInterrupt(interruptPin))` instead of just `detachInterrupt(interruptPin)` comes down to portability, Arduino API standards, and hardware-abstraction safety.

### 1. Cross-Board Compatibility (The Main Reason)

On original 8-bit AVR boards (like the Arduino Uno), physical pin numbers and interrupt vector numbers do not match:

- Digital Pin 2 maps to Interrupt Channel 0 (INT0).
- Digital Pin 3 maps to Interrupt Channel 1 (INT1).

On an Uno, if you passed raw `2` into `attachInterrupt(2, ...)` or `detachInterrupt(2)`, you were actually trying to configure Pin 4 (Interrupt channel 2), leading to silent bugs. The macro `digitalPinToInterrupt(pin)` translates a board's physical pin number into its internal interrupt channel.

### 2. How it Behaves on ESP32 / ESP32-S3

On the ESP32 architecture, every GPIO pin can act as a hardware interrupt, so internal GPIO numbers map directly to interrupt channels.

In the official ESP32 Arduino Core source, `digitalPinToInterrupt()` is defined as a passthrough macro with bounds checking:

```cpp
#define digitalPinToInterrupt(p) (((uint8_t)(p) < SOC_GPIO_PIN_COUNT) ? (p) : -1)
```

- **Safety Catch:** If a pin doesn't exist or isn't capable of interrupts on the ESP32-S3 (or if an out-of-range integer is passed), `digitalPinToInterrupt()` returns `-1` (invalid), preventing hardware configuration crashes.
- **Direct Pass-through:** If the pin is valid, it simply evaluates directly to `p`.

### 3. Pin-Remapping Variants (e.g., Arduino Nano ESP32)

Certain boards use board-variant mapping matrices. For instance, on the Arduino Nano ESP32, the label written on the silkscreen (e.g., D2) does not equal ESP32 GPIO 2.

Wrapping your variable in `digitalPinToInterrupt(pin)` ensures that if the core needs to resolve a board-specific header alias down to the physical silicon GPIO, it gets translated safely.

### Summary Rule of Thumb

| Syntax | Portability | Safety | Recommendation |
| - | - | - | - |
| `detachInterrupt(pin)` | ESP32-specific | Low (No bounds check) | Not recommended for portable code |
| `detachInterrupt(digitalPinToInterrupt(pin))` | Universal (Uno, Mega, ESP32, STM32) | High (Validates pin limit) | Recommended Standard |

## References

[https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-gpio.h](https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-gpio.h)

[https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-timer.h](https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-timer.h)

[https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-matrix.h](https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-matrix.h)

[https://www.oceanlabz.in/esp32-interrupt-timers/](https://www.oceanlabz.in/esp32-interrupt-timers/)

[https://www.luisllamas.es/esp32-interrupciones-hardware/](https://www.luisllamas.es/esp32-interrupciones-hardware/)

[https://randomnerdtutorials.com/esp32-pir-motion-sensor-interrupts-timers/](https://randomnerdtutorials.com/esp32-pir-motion-sensor-interrupts-timers/)
