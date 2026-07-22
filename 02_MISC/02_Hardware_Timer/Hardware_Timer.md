# Hardware Timer     

In the context of the Arduino IDE for the ESP32-S3, Hardware Timers are dedicated peripheral counters that operate independently of the main CPU. They count clock cycles at a precise, predefined rate and can trigger an interrupt when they reach a specific target value (an "alarm").     
Using hardware timers is the most robust way to execute tasks at exact time intervals without blocking the main program (unlike delay()) and with much higher precision than the standard millis() function.      
Here is a detailed breakdown of how hardware timers work on the ESP32-S3 within the Arduino ecosystem.      
**1. ESP32-S3 Hardware Timer Architecture**     
Unlike simpler microcontrollers (like the Arduino Uno) that might only have two 16-bit timers, the ESP32-S3 is equipped with a highly capable timer subsystem:     
- Quantity: It has four general-purpose hardware timers.
- Bit-width: They are 64-bit timers, meaning they can count up to 2^64 - 1 before overflowing, which is practically infinite for human-scale timeframes.
- Grouping: They are divided into two Timer Groups (Group 0 and Group 1), each containing two timers (Timer 0 and Timer 1).
- Clock Source: They are driven by the APB (Advanced Peripheral Bus) clock, which typically runs at 80 MHz.         

**2. The Arduino API: A Major Shift (Important!)**     
If you are searching for ESP32 timer tutorials online, be careful. The Arduino core for ESP32 underwent a massive API rewrite in version 2.0.0 (and further refined in 3.0.0) to align closer to the underlying Espressif ESP-IDF.     
- The Legacy API (v1.x - Deprecated): Used prescalers and raw ticks.
  *(e.g., timerBegin(0, 80, true) -> Timer 0, prescaler 80, count up).* Do not use this if you are on a modern Arduino core.
- The Modern API (v2.x & v3.x): Uses direct frequency in Hertz. The prescaler is calculated automatically under the hood.
  *(e.g., timerBegin(1000000) -> Sets the timer to tick at 1,000,000 Hz / 1 MHz).*     

*The rest of this guide focuses on the Modern API, which is standard for current Arduino IDE installations.*     

**3. Core Functions of the Modern Timer API**     
**Initialization and Cleanup**     
- ```hw_timer_t * timerBegin(uint32_t frequency)```: Initializes a timer and sets its ticking frequency in Hz. The maximum frequency is usually 80,000,000 (80 MHz), and the minimum is 1 Hz. It returns a pointer to the timer object.
- ```void timerEnd(hw_timer_t *timer)```: Stops and deallocates the timer.
**Interrupt Management**     
- ```void timerAttachInterrupt(hw_timer_t *timer, void (*fn)(void))```: Binds your Interrupt Service Routine (ISR) to the timer. When the timer hits the alarm value, this function is called.
- ```void timerDetachInterrupt(hw_timer_t *timer)```: Removes the ISR from the timer.     
**Alarm Configuration (The Trigger)**      
- ```void timerAlarm(hw_timer_t *timer, uint64_t alarm_value, bool autoreload, uint64_t reload_count)```:
  - alarm_value: The number of ticks at which the interrupt should fire. (If frequency is 1MHz, an alarm value of 1,000,000 equals 1 second).
  - autoreload: If true, the timer automatically resets and starts counting again after firing. If false, it fires once and stops.
  - reload_count: The value the timer resets to after firing (usually 0).
**Control and Reading**    
- ```void timerStart(hw_timer_t *timer)``` / ```void timerStop(hw_timer_t *timer)```: Pauses or resumes the timer without deleting its configuration.
- ```uint64_t timerRead(hw_timer_t *timer)```: Reads the current tick count of the timer.
- ```void timerWrite(hw_timer_t *timer, uint64_t val)```: Manually forces the timer to a specific tick value.     

**4. Code Example: Modern Hardware Timer**     
This example configures a timer to trigger an interrupt exactly every 1 second (1,000,000 microseconds at a 1MHz frequency).
```
// Pointer to the timer object
hw_timer_t *myTimer = NULL;

// Volatile flag to communicate between ISR and main loop
volatile bool timerFlag = false;

// Spinlock for dual-core safety (ESP32-S3 has two cores)
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// ---------------------------------------------------------
// INTERRUPT SERVICE ROUTINE (ISR)
// ---------------------------------------------------------
// MUST have IRAM_ATTR to prevent cache-miss crashes
void IRAM_ATTR onTimerInterrupt() {
  // Enter critical section to protect shared variable
  portENTER_CRITICAL_ISR(&timerMux);
  timerFlag = true;
  portEXIT_CRITICAL_ISR(&timerMux);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.println("Initializing Hardware Timer...");

  // 1. Initialize timer at 1,000,000 Hz (1 MHz)
  // This means 1 tick = 1 microsecond
  myTimer = timerBegin(1000000); 

  // 2. Attach the ISR to the timer
  timerAttachInterrupt(myTimer, &onTimerInterrupt);

  // 3. Set the alarm
  // We want 1 second. At 1MHz, 1 second = 1,000,000 ticks.
  // autoreload = true (repeat every second)
  // reload_count = 0 (reset counter to 0 after firing)
  timerAlarm(myTimer, 1000000, true, 0);

  Serial.println("Timer started. Interrupt will fire every 1 second.");
}

void loop() {
  // Check if the timer interrupt has fired
  if (timerFlag) {
    // Safely read and reset the flag
    portENTER_CRITICAL(&timerMux);
    timerFlag = false;
    portEXIT_CRITICAL(&timerMux);

    // Perform the time-critical task here
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    
    // Note: Serial.print is okay here in the main loop, 
    // but NEVER inside the ISR!
    Serial.printf("Timer fired! Current millis: %lu\n", millis());
  }

  // The CPU is free to do other non-blocking tasks here
  // ...
}
```
**5. Best Practices & Pitfalls for ESP32-S3 Timers**      
- **The IRAM_ATTR Rule is Non-Negotiable:**     
If you forget to put IRAM_ATTR before your timer ISR function name, the ESP32 will execute the ISR from Flash memory. If a Wi-Fi/Bluetooth interrupt occurs at the exact same time, the flash cache will be disabled, the CPU will fail to read the ISR code, and the ESP32 will panic and reboot.
- **Keep the ISR Blazing Fast:**    
The ISR should do nothing but set a flag or toggle a pin. Do not use delay(), Serial.print(), malloc(), or read/write to I2C/SPI inside the ISR. Handle the heavy lifting in the loop() by checking the volatile flag.
- **Dual-Core Safety (portMUX_TYPE):**    
The ESP32-S3 has two cores (Core 0 and Core 1). Your timer interrupt might fire on Core 1 while your loop() is running on Core 0. If both try to read/write the same variable at the exact same nanosecond, data corruption occurs. Always use portENTER_CRITICAL and portEXIT_CRITICAL (or their _ISR variants) to protect shared variables.
- **Timer Resolution vs. Frequency:**     
While you can set the timer frequency up to 80MHz, doing so means the 64-bit counter will overflow much faster. For most Arduino applications, a frequency of 1000000 (1 MHz) is the sweet spot, giving you 1-microsecond resolution.
- **Don't use Timers for PWM:**     
If your goal is to dim an LED or control a motor speed, do not use general-purpose hardware timers. Use the LEDC (LED Control) peripheral via ledcAttach() and ledcWrite(), which is specifically hardware-optimized for PWM on the ESP32-S3. 

