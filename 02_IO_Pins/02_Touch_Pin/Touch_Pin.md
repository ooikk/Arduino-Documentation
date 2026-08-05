# Capacitive Touch Sensor Pins

The ESP32 S3 has 14 capacitive touch GPIOs. These GPIOs can sense variations in anything that holds an electrical charge, like the human skin. So they can detect variations induced when touching the GPIOs with a finger.

Important note for ESP32-S3: Unlike the original ESP32 (where touch readings decrease when touched), the ESP32-S3 touch sensor readings INCREASE when touched. Keep this in mind when determining your threshold!

 


## ESP32 S3 Touch Pins
```
Touch1     GPIO1
Touch2     GPIO2
Touch3     GPIO3
Touch4     GPIO4
Touch5     GPIO5
Touch6     GPIO6
Touch7     GPIO7
Touch8     GPIO8
Touch9     GPIO9
Touch10    GPIO10
Touch11    GPIO11
Touch12    GPIO12
Touch13    GPIO13
Touch14    GPIO14
```

## Detection

You can detect touch input using just a wire or a piece of foil — no external module needed. You can use the following command to read the touch pin capacitance.

```cpp
touchVal = touchRead(GPIO);
if (touchVal > THESHOLD) {
// touch detected
}
```

## Touch API

The Touch Sensor API in the Arduino ESP32 core (esp32-hal-touch.h) provides functions to measure capacitance on touch-capable GPIO pins and trigger interrupts or wake-ups.

### 1. Primary Touch Sensing APIs

**`touchRead()`:** Reads the current raw capacitive value of a touch pin.

```cpp
touch_value_t touchRead(uint8_t pin);
```

- pin: Touch-capable GPIO pin number (e.g., TOUCH4 / GPIO 4).
- Returns: A numeric value representing capacitance. Use this in `setup()` via `Serial.println(touchRead(TOUCH_PIN))` to observe baseline vs. touched values before setting an interrupt threshold.

**`touchAttachInterrupt()`:** Attaches a basic Interrupt Service Routine (ISR) callback that fires when touch capacitance passes the threshold.

```cpp
void touchAttachInterrupt(uint8_t pin, void (*userFunc)(void), touch_value_t threshold);
```

- pin: Touch pin.
- userFunc: Pointer to your ISR callback function.
- threshold: Trigger value. On ESP32-S3, the interrupt fires when the pin reading exceeds this number.

**`touchAttachInterruptArg()`:** Attaches an ISR callback with custom parameters passed to the handler function.

```cpp
void touchAttachInterruptArg(uint8_t pin, void (*userFunc)(void *), void *arg, touch_value_t threshold);
```

- userFunc: Function signature must accept a `void*` argument: `void myISR(void *arg)`
- arg: Pointer to data (variable, struct, or class object) passed directly to the ISR.

**`touchDetachInterrupt()`:** Disables touch interrupt capabilities on the designated pin.

```cpp
void touchDetachInterrupt(uint8_t pin);
```

### 2. Status & Configuration Functions

**`touchInterruptGetLastStatus()`** *(ESP32-S2 & ESP32-S3 specific)*: Checks if the touch pin is currently actively touched.

```cpp
bool touchInterruptGetLastStatus(uint8_t pin);
```

- Returns: `true` if the touch pad is actively pressed/touched, `false` if untouched/released.
- Use Case: Perfect for inside an ISR or background task to distinguish between a Touch Press and a Touch Release event.

**`touchSetCycles()`:** Adjusts the hardware measurement cycle duration and sleep periods.

```cpp
void touchSetCycles(uint16_t measure, uint16_t sleep);
```

- measure: Duration of the measurement phase (default: 0x1000).
- sleep: Idle period between measurements (default: 0x1000).
- Note: Tweaking these can reduce power consumption or improve filtering noise at the cost of execution speed.

**`touchSleepWakeUpEnable()`:** Configures a touch pin as a wake-up trigger source to exit ESP32-S3 Deep Sleep.

```cpp
void touchSleepWakeUpEnable(uint8_t pin, touch_value_t threshold);
```

- pin: Touch pin designated as wake-up source.
- threshold: Capacitance threshold that triggers a wake-up event.

## Sample code: touchAttachInterruptArg()

Passing custom arguments into an interrupt handler is useful when you want to reuse a single ISR function across multiple touch pads without writing duplicate code for each pin.

**Code Example: Managing Multiple Touch Pads with a Single ISR:**

```cpp
#include <Arduino.h>

// 1. Define a struct to hold custom parameters for each touch pad
struct TouchButton {
  uint8_t pin;
  const char* name;
  volatile bool wasTouched;
};

// Instantiating parameters for two separate touch buttons
// Note: ESP32-S3 Touch GPIOs include 1 to 14 (e.g., GPIO 4 and GPIO 5)
TouchButton buttonA = {4, "Button A (GPIO 4)", false};
TouchButton buttonB = {5, "Button B (GPIO 5)", false};

// Threshold setting: S3 readings INCREASE when touched
// Adjust based on your touchRead() baseline (e.g., baseline ~30,000 -> set threshold to ~50,000)
const touch_value_t TOUCH_THRESHOLD = 50000; 

// 2. Multi-purpose ISR accepting a void pointer argument
void ARDUINO_ISR_ATTR handleTouchArg(void* arg) {
  // Cast the generic void pointer back to our struct type
  TouchButton* btn = (TouchButton*)arg;
  btn->wasTouched = true; // Mark the specific button struct as touched
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Initializing Touch Interrupts with Custom Arguments...");

  // 3. Attach touch interrupts and pass addresses of button structs as arguments
  touchAttachInterruptArg(buttonA.pin, handleTouchArg, &buttonA, TOUCH_THRESHOLD);
  touchAttachInterruptArg(buttonB.pin, handleTouchArg, &buttonB, TOUCH_THRESHOLD);
}

void loop() {
  // Check Button A flag
  if (buttonA.wasTouched) {
    buttonA.wasTouched = false; // Reset flag
    Serial.print("Interrupt Fired on: ");
    Serial.println(buttonA.name);
  }

  // Check Button B flag
  if (buttonB.wasTouched) {
    buttonB.wasTouched = false; // Reset flag
    Serial.print("Interrupt Fired on: ");
    Serial.println(buttonB.name);
  }

  vTaskDelay(pdMS_TO_TICKS(10)); // Yield to prevent watchdog triggers
}
```

### Key Highlights of This Approach

- **`void* arg` Casting:** Inside `handleTouchArg(void* arg)`, we cast `(TouchButton*)arg` back to our data type. This grants full access to the specific struct's properties (pin, name, wasTouched).
- **Zero Code Duplication:** Instead of writing `handleButtonA()` and `handleButtonB()` separately, a single ISR dynamically identifies which physical pad was pressed based on the pointer passed into `touchAttachInterruptArg()`.

### vTaskDelay(pdMS_TO_TICKS(10))

The line `vTaskDelay(pdMS_TO_TICKS(10));` is a FreeRTOS API call used on ESP32 microcontrollers to pause execution for a specific duration without wasting CPU cycles.

**Breakdown of Each Component**

1. **`vTaskDelay(...)`**
   - **What it does:** Pauses the current FreeRTOS task and yields control back to the operating system's task scheduler.
   - **How it works:** Instead of freezing the processor core, it places the current task into a Blocked state for a specified number of system ticks.
   - **Why it matters:** While your task is blocked, the ESP32 CPU core is completely freed up to run other tasks (like Wi-Fi, background processing, or internal system maintenance) or go into an idle/power-saving state.

2. **`pdMS_TO_TICKS(10)`**
   - **What it does:** Converts time in milliseconds (10 ms) into FreeRTOS system ticks.
   - **Why it's necessary:** FreeRTOS measures time in internal clock counts called ticks, not milliseconds. The length of a single tick depends on the system setting `configTICK_RATE_HZ`.
     - On ESP32 (where `configTICK_RATE_HZ` is typically set to 1000 Hz), 1 tick = 1 ms.
     - However, using `pdMS_TO_TICKS(10)` ensures your code remains portable and rate-independent across different platforms or FreeRTOS configuration tweaks.

**`vTaskDelay()` vs. Standard `delay()`**

In the ESP32 Arduino Core, `delay(10)` is actually an inline wrapper around `vTaskDelay(pdMS_TO_TICKS(10))`! However, using `vTaskDelay()` directly offers distinct advantages:

| Feature | `vTaskDelay(pdMS_TO_TICKS(10));` | `delay(10);` |
| - | - | - |
| Execution | Non-blocking (Yields CPU to other tasks) | Non-blocking on ESP32, but blocking on standard 8-bit Arduinos |
| API Origin | Native FreeRTOS Real-Time OS | Standard Arduino Framework |
| Context | Preferred inside FreeRTOS custom tasks (`xTaskCreate`) | Standard for simple Arduino `loop()` code |

## Reference

[https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/touch.html](https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/touch.html)

[https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-touch.h](https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-touch.h)
