// Hardware Timer LED Toggle Example
#define LED_PIN 15  // Change to your board's LED pin (e.g., GPIO 2 or 38)

hw_timer_t *timer = NULL;
volatile bool toggleFlag = false;

// ISR triggered every 1 second
void ARDUINO_ISR_ATTR onTimer() {
  toggleFlag = true; // Set flag to handle pin toggle in main loop
}

void setup() {
  pinMode(LED_PIN, OUTPUT);

  // 1. Initialize timer with 1 MHz frequency (1 tick = 1 microsecond)
  timer = timerBegin(1000000);

  // 2. Attach ISR callback to the timer
  timerAttachInterrupt(timer, &onTimer);

  // 3. Set alarm to trigger every 1,000,000 µs (1 sec) with auto-reload (true)
  timerAlarm(timer, 1000000, true, 0);
}

void loop() {
  if (toggleFlag) {
    toggleFlag = false; // Clear flag
    digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Toggle LED state
  }
}
