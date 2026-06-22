# PWM

## What is a PWM Pin on the ESP32-S3?
**PWM (Pulse Width Modulation)** is a technique used to simulate an analog output using a digital pin. Since digital pins can only output 3.3V (HIGH) or 0V (LOW), PWM achieves intermediate voltages by flipping the pin between HIGH and LOW at a very fast frequency.

By varying the **Duty Cycle** (the percentage of time the signal remains HIGH during one full cycle), you control the average power delivered to a component. This allows you to dim LEDs, control the speed of DC motors, or drive servo motors.

**How Many PWM Channels & Modes Can Be Configured?**
Unlike older microcontrollers (like the Arduino Uno) which have specific hardware pins dedicated to PWM, the ESP32-S3 features an internal peripheral called the **LED Control (LEDC)** PWM controller.

Through an internal routing matrix, any of the user-accessible digital GPIO pins can be configured to output a PWM signal.

**The System Structure:**
- **Channels**: The ESP32-S3 provides 8 independent PWM channels.
- **Speed Mode**: It operates entirely in High-Speed Mode (the low-speed mode from older ESP32 chips was removed in the S3).
- **Timers**: There are 4 independent timers. Multiple channels can share the same timer if they need to operate at identical frequencies, but they can still maintain different duty cycles.
- **Resolution**: The resolution is highly configurable, ranging from 1-bit to 14-bit (and up to 20-bit depending on the frequency). For instance, an 8-bit resolution gives you 256 distinct steps of power control (0 to 255), while a 10-bit resolution gives you 1024 steps (0 to 1023).     

## Total Number of PWM Pins for the User       
Because of the ESP32-S3’s internal routing flexibility, there isn't a fixed set of "PWM pins."
- **Maximum Simultaneous PWM Outputs**: You can output up to 8 independent PWM signals at the same time (matching the 8 hardware channels).
- **Which Pins Can Be Used**: You can assign those 8 channels to any of the ~23 to 28 safe general-purpose GPIO pins detailed previously (e.g., GPIO 1–2, 4–18, 21, 38–42).

## Code Examples (Arduino IDE)       
In the standard ESP32 Arduino core, PWM is handled natively using the **ledc** functions.     

**1. Fading an LED (Basic PWM)**     
This example sets up an 8-bit PWM channel on a GPIO pin to smoothly fade an LED up and down.     
```
// Define the GPIO pin
const int LED_PIN = 4;

// PWM Properties
const int PWM_FREQ = 5000;    // 5 kHz frequency
const int PWM_CHANNEL = 0;   // Use PWM channel 0 (0-7 available)
const int PWM_RES = 8;       // 8-bit resolution (0 - 255 values)

void setup() {
  // Configure the LEDC PWM channel with frequency and resolution
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  
  // Attach the physical GPIO pin to the configured PWM channel
  ledcAttachPin(LED_PIN, PWM_CHANNEL);
}

void loop() {
  // Fade inside loop (increasing brightness)
  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {
    ledcWrite(PWM_CHANNEL, dutyCycle);
    delay(5);
  }

  // Fade outside loop (decreasing brightness)
  for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
    ledcWrite(PWM_CHANNEL, dutyCycle);
    delay(5);
  }
}
```
**2. Controlling a RC Servo Motor (Specific Frequency & Resolution)**     
Standard hobby servos require a specific frame rate frequency of 50Hz and a pulse width that varies precisely between roughly 1ms and 2ms to position the motor shaft between 0 and 180 degrees.    

Using a 10-bit resolution (0 to 1023) at 50Hz, a 1ms pulse corresponds to a duty cycle value of roughly ~51, and a 2ms pulse corresponds to roughly ~102.     
```
const int SERVO_PIN = 5;

// Servo PWM Settings
const int SERVO_FREQ = 50;     // 50Hz frequency is standard for servos
const int SERVO_CHANNEL = 1;   // Use channel 1 to avoid conflicts
const int SERVO_RES = 10;      // 10-bit resolution (0 - 1023)

// Calculated target values for standard 50Hz / 10-bit setup
const int SERVO_MIN = 26;      // ~0.5ms pulse (0 degrees)
const int SERVO_MAX = 123;     // ~2.4ms pulse (180 degrees)

void setup() {
  ledcSetup(SERVO_CHANNEL, SERVO_FREQ, SERVO_RES);
  ledcAttachPin(SERVO_PIN, SERVO_CHANNEL);
}

void loop() {
  // Turn to 0 degrees
  ledcWrite(SERVO_CHANNEL, SERVO_MIN);
  delay(1000);

  // Turn to 90 degrees (midpoint)
  ledcWrite(SERVO_CHANNEL, (SERVO_MIN + SERVO_MAX) / 2);
  delay(1000);

  // Turn to 180 degrees
  ledcWrite(SERVO_CHANNEL, SERVO_MAX);
  delay(1000);
}
```

## References     

https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/ledc.html

https://www.oceanlabz.in/esp32-pwm/

https://randomnerdtutorials.com/esp32-pwm-arduino-ide/

https://deepbluembedded.com/esp32-pwm-tutorial-examples-analogwrite-arduino/

