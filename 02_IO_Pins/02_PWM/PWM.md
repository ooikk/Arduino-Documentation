# PWM

## What is a PWM Pin on the ESP32-S3?
**PWM (Pulse Width Modulation)** is a technique used to simulate an analog output using a digital pin. Since digital pins can only output 3.3V (HIGH) or 0V (LOW), PWM achieves intermediate voltages by flipping the pin between HIGH and LOW at a very fast frequency.

By varying the **Duty Cycle** (the percentage of time the signal remains HIGH during one full cycle), you control the average power delivered to a component. This allows you to dim LEDs, control the speed of DC motors, or drive servo motors.

On the ESP32-S3, there are 45 physical GPIO pins that can be used for PWM output. Because the chip utilizes a flexible GPIO Matrix, any of the internal PWM peripheral signals can be routed to any of these 45 programmable GPIOs.

**Available PWM Resources**     
While any pin can output PWM, the total number of independent PWM signals you can generate simultaneously is determined by the internal controllers:
- **LED PWM Controller (LEDC)**: Provides up to *8 independent channels*. It supports gradual duty cycle fading and resolutions up to 14 bits.
- **Motor Control PWM (MCPWM)**: There are *two MCPWM units*, each with 6 output signals, providing a total of *12 outputs*.
  - MCPWM0 (Unit 0): pwm0_out0a, pwm0_out0b, pwm0_out1a, pwm0_out1b, pwm0_out2a, and pwm0_out2b
  - MCPWM1 (Unit 1): pwm1_out0a, pwm1_out0b, pwm1_out1a, pwm1_out1b, pwm1_out2a, and pwm1_out2b
- **Sigma-Delta Modulated Output (SDM)**: Provides *8 channels* of 1-bit second-order sigma-delta modulation, which can produce Pulse Density Modulation (PDM) signals.     

**LED PWM Controller (LEDC)**     
The LED PWM Controller is a peripheral designed to generate PWM signals for LED control. It has specialized features such as automatic duty cycle fading. However, the LED PWM Controller can also be used to generate PWM signals for other purposes.    

The LED PWM Controller has the following features:     
- **Channels**: Eight independent PWM generators (i.e., eight channels)
- **Timers**: Four independent timers that support division by fractions. Multiple channels can share the same timer if they need to operate at identical frequencies, but they can still maintain different duty cycles.
- **Automatic Fading**: Automatic duty cycle fading (i.e., gradual increase/decrease of a PWM’s duty cycle without interference from the processors) with interrupt generation on fade completion
- **Phase**: Adjustable phase of PWM signal output
- **Low Power Mode**: PWM signal output in low-power mode (Light-sleep mode)
- **Speed Mode**: It operates entirely in High-Speed Mode (the low-speed mode from older ESP32 chips was removed in the S3).
- **Resolution**: The resolution is highly configurable, ranging from 1-bit to 14-bit (and up to 20-bit depending on the frequency). For instance, an 8-bit resolution gives you 256 distinct steps of power control (0 to 255), while a 10-bit resolution gives you 1024 steps (0 to 1023).


LED PWM Architecture:      
<img width="319" height="311" alt="image" src="https://github.com/user-attachments/assets/bd677d8f-d366-4d79-b79a-e7cac52484cb" />

**Motor Control PWM (MCPWM)**     
The Motor Control Pulse Width Modulator (MCPWM) peripheral is intended for motor and power control. It provides six PWM outputs that can be set up to operate in several topologies. One common topology uses a pair of PWM outputs driving an H-bridge to control motor rotation speed and rotation direction.     
The timing and control resources inside are allocated into two major types of submodules: PWM timers and PWM operators. Each PWM timer provides timing references that can either run freely or be synced to other timers or external sources. Each PWM operator has all necessary control resources to generate waveform pairs for one PWM channel. The MCPWM peripheral also contains a dedicated capture submodule that is used in systems where accurate timing of external events is important.     
ESP32-S3 contains two MCPWM peripherals: *MCPWM0* and *MCPWM1*.     

<img width="719" height="535" alt="image" src="https://github.com/user-attachments/assets/012b99b7-568f-4b51-893c-47fa011fedb6" />

An overview of the submodules’ function in Figure 36.2-1 is shown below:     
- **PWM Timers 0, 1 and 2**     
  - Every PWM timer has a dedicated 8-bit clock prescaler.
  - The 16-bit counter in the PWM timer can work in count-up mode, count-down mode or count-up-down mode.
  - A hardware sync or software sync can trigger a reload on the PWM timer with a phase register. It will also trigger the prescaler’s restart, so that the timer’s clock can also be synced. The source of the hard sync can come from any GPIO or any other PWM timer’s sync_out. The source of the soft sync comes from writing toggle value to the MCPWM_TIMERx_SYNC_SW bit.    
- **PWM Operators 0, 1 and 2**
  - Every PWM operator has two PWM outputs: PWMxA and PWMxB. They can work independently, in symmetric and asymmetric configuration.
  - Software, asynchronously override control of PWM signals.
  - Configurable dead-time on rising and falling edges; each set up independently.
  - All events can trigger CPU interrupts.
  - Modulating of PWM output by high-frequency carrier signals, useful when gate drivers are insulated with a transformer.
  - Period, time stamps and important control registers have shadow registers with flexible updating methods.
- **Fault Detection Module**   
  - Programmable fault handling allocated on fault condition in both cycle-by-cycle mode and one-shot mode.
  - A fault condition can force the PWM output to either high or low logic levels.
- **Capture Module**    
  - Speed measurement of rotating machinery (for example, toothed sprockets sensed with Hall sensors)
  - Measurement of elapsed time between position sensor pulses
  - Period and duty-cycle measurement of pulse train signals
  - Decoding current or voltage amplitude derived from duty-cycle-encoded signals of current/voltage sensors
  - Three individual capture channels, each of which has a time-stamp register (32 bits)
  - Selection of edge polarity and prescaling of input capture signal
  - The capture timer can sync with a PWM timer or external signals.
  - Interrupt on each of the three capture channels

**The PWM-Capable Pins**     
The 45 physical GPIO pins available for user assignment are:     
- GPIO 0 through 21
- GPIO 26 through 48     

**Important Pin Restrictions**     
While all 45 pins are technically PWM-capable, you should prioritize or avoid certain pins based on their primary hardware functions:
1. **Avoid (Internal Memory)**: GPIO 26 through 32 are allocated for communication with the in-package SPI flash/PSRAM. Using these for PWM can cause system failure or data corruption.
2. **Use with Caution (Strapping Pins)**: GPIO 0, 3, 45, and 46 are strapping pins used to determine boot modes and voltage levels at power-up. Connecting external PWM loads to these pins might prevent the chip from starting correctly.
3. **Use with Caution (Debug/USB)**:
- GPIO 43 and 44: Default for UART0 (Serial Debugging).
- GPIO 19 and 20: Default for the Native USB port.
4. **Recommended (Priority 2)**: Pins that can be freely used without restrictions include **GPIO 1, 2, 4 through 8, 15 through 18, and 21**.





## Total Number of PWM Pins for the User       
Because of the ESP32-S3’s internal routing flexibility, there isn't a fixed set of "PWM pins."
- **Maximum Simultaneous PWM Outputs**: You can output up to 8 independent PWM signals at the same time (matching the 8 hardware channels).
- **Which Pins Can Be Used**: You can assign those 8 channels to any of the ~23 to 28 safe general-purpose GPIO pins detailed previously (e.g., **GPIO 1–2, 4–18, 21, 35–42, 47, 48**).

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

