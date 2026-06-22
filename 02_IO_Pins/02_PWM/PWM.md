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

## LED PWM Controller (LEDC)     
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

## Motor Control PWM (MCPWM)     
The Motor Control Pulse Width Modulator (MCPWM) peripheral is intended for motor and power control. It provides six PWM outputs that can be set up to operate in several topologies. One common topology uses a pair of PWM outputs driving an H-bridge to control motor rotation speed and rotation direction.     
The timing and control resources inside are allocated into two major types of submodules: PWM timers and PWM operators. Each PWM timer provides timing references that can either run freely or be synced to other timers or external sources. Each PWM operator has all necessary control resources to generate waveform pairs for one PWM channel. The MCPWM peripheral also contains a dedicated capture submodule that is used in systems where accurate timing of external events is important.     
ESP32-S3 contains two MCPWM peripherals: *MCPWM0* and *MCPWM1*.     

<img width="636" height="367" alt="image" src="https://github.com/user-attachments/assets/faefd3a8-c764-4643-81b9-c2208edd6004" />


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



## The PWM-Capable Pins for user     
The 45 physical GPIO pins available for user assignment are:     
- GPIO 0 through 21
- GPIO 26 through 48     

**Important Pin Restrictions**     
While all 45 pins are technically PWM-capable, and there isn't a fixed set of "PWM pins". You can output up to 8 independent PWM signals simultaneously at the same time (matching the 8 hardware channels). You should prioritize or avoid certain pins based on their primary hardware functions:
1. **Avoid (Internal Memory)**: GPIO 26 through 32 are allocated for communication with the in-package SPI flash/PSRAM. Using these for PWM can cause system failure or data corruption.
2. **Use with Caution (Strapping Pins)**: GPIO 0, 3, 45, and 46 are strapping pins used to determine boot modes and voltage levels at power-up. Connecting external PWM loads to these pins might prevent the chip from starting correctly.
3. **Use with Caution (Debug/USB)**:
- GPIO 43 and 44: Default for UART0 (Serial Debugging).
- GPIO 19 and 20: Default for the Native USB port.
4. **Recommended**: Pins that can be freely used without restrictions include **GPIO 1–2, 4–18, 21, 35–42, 47, 48**.

## LED Control (LEDC) API   

**ledcAttach**     
This function is used to setup LEDC pin with given frequency and resolution. LEDC channel will be selected automatically.     

```
bool ledcAttach(uint8_t pin, uint32_t freq, uint8_t resolution);
```
- *pin* select LEDC pin.
- *freq* select frequency of pwm.
- *resolution* select resolution for LEDC channel.
  - range is 1-14 bits (1-20 bits for ESP32).

This function will return *true* if configuration is successful. If *false* is returned, error occurs and LEDC channel was not configured.     

**ledcAttachChannel**     
This function is used to setup LEDC pin with given frequency, resolution and channel.     
```
bool ledcAttachChannel(uint8_t pin, uint32_t freq, uint8_t resolution, uint8_t channel);
```
- *pin* select LEDC pin.
- *freq* select frequency of pwm.
- *resolution* select resolution for LEDC channel.
- *channel* select LEDC channel.
  - range is 1-14 bits (1-20 bits for ESP32).

This function will return *true* if configuration is successful. If *false* is returned, error occurs and LEDC channel was not configured.     

**ledcWrite**     
This function is used to set duty for the LEDC pin.     
```
void ledcWrite(uint8_t pin, uint32_t duty);
```
- *pin* select LEDC pin.
- *duty* select duty to be set for selected LEDC pin.     

This function will return *true* if setting duty is successful. If *false* is returned, error occurs and duty was not set.     

**ledcRead**     
This function is used to get configured duty for the LEDC pin.     
```
uint32_t ledcRead(uint8_t pin);
```
- *pin* select LEDC pin to read the configured LEDC duty.     

This function will return duty set for selected LEDC pin.

**ledcReadFreq**    
This function is used to get configured frequency for the LEDC channel.     
```
uint32_t ledcReadFreq(uint8_t pin);
```
- *pin* select LEDC pin to read the configured frequency.

This function will return frequency configured for selected LEDC pin.     

**ledcWriteTone**     
This function is used to setup the LEDC pin to 50 % PWM tone on selected frequency.      
```
uint32_t ledcWriteTone(uint8_t pin, uint32_t freq);
```
- *pin* select LEDC pin.
- *freq* select frequency of pwm signal. If frequency is 0, duty will be set to 0.      

This function will return *frequency* set for LEDC pin. If *0* is returned, error occurs and LEDC pin was not configured.      

**ledcWriteNote**     
This function is used to setup the LEDC pin to specific note.     
```
uint32_t ledcWriteNote(uint8_t pin, note_t note, uint8_t octave);
```
- *pin* select LEDC pin.     
- *note* select note to be set.    

|NOTE_C|NOTE_Cs|NOTE_D|NOTE_Eb|NOTE_E|NOTE_F|
|-|-|-|-|-|-|
|NOTE_Fs|NOTE_G|NOTE_Gs|NOTE_A|NOTE_Bb|NOTE_B|

- *octave* select octave for note.   

This function will return *frequency* configured for the LEDC pin according to note and octave inputs. If *0* is returned, error occurs and the LEDC channel was not configured.

**ledcDetach**    
This function is used to detach the pin from LEDC.   
```
bool ledcDetach(uint8_t pin);
```
- *pin* select LEDC pin.

This function returns *true* if detaching was successful. If *false* is returned, an error occurred and the pin was not detached.     

**ledcChangeFrequency**    
This function is used to set frequency for the LEDC pin.     
```
uint32_t ledcChangeFrequency(uint8_t pin, uint32_t freq, uint8_t resolution);
```
- *pin* select LEDC pin.
- *freq* select frequency of pwm.
- *resolution* select resolution for LEDC channel.
  - range is 1-14 bits (1-20 bits for ESP32).

This function will return *frequency* configured for the LEDC channel. If *0* is returned, error occurs and the LEDC channel frequency was not set.    

**ledcFade**     
This function is used to setup and start fade for the LEDC pin.     
```
bool ledcFade(uint8_t pin, uint32_t start_duty, uint32_t target_duty, int max_fade_time_ms);
```
- *pin* select LEDC pin.
- *start_duty* select starting duty of fade.
- *target_duty* select target duty of fade.
- *max_fade_time_ms* select maximum time for fade.

This function will return *true* if configuration is successful. If *false* is returned, error occurs and LEDC fade was not configured / started.

**ledcFadeWithInterrupt**    
This function is used to setup and start fade for the LEDC pin with interrupt.     
```
bool ledcFadeWithInterrupt(uint8_t pin, uint32_t start_duty, uint32_t target_duty, int max_fade_time_ms, void (*userFunc)(void));
```
- *pin* select LEDC pin.
- *start_duty* select starting duty of fade.
- *target_duty* select target duty of fade.
- *max_fade_time_ms* select maximum time for fade.
- *userFunc* funtion to be called when interrupt is triggered.

This function will return *true* if configuration is successful and fade start. If *false* is returned, error occurs and LEDC fade was not configured / started.   

**ledcFadeWithInterruptArg**     
This function is used to setup and start fade for the LEDC pin with interrupt using arguments.    
```
bool ledcFadeWithInterruptArg(uint8_t pin, uint32_t start_duty, uint32_t target_duty, int max_fade_time_ms, void (*userFunc)(void*), void * arg);
```
- *pin* select LEDC pin.
- *start_duty* select starting duty of fade.
- *target_duty* select target duty of fade.
- *max_fade_time_ms* select maximum time for fade.
- *userFunc* funtion to be called when interrupt is triggered.
- *arg* pointer to the interrupt arguments.    

This function will return *true* if configuration is successful and fade start. If *false* is returned, error occurs and LEDC fade was not configured / started.

**analogWrite**    
This function is used to write an analog value (PWM wave) on the pin. It is compatible with Arduinos analogWrite function.
```
void analogWrite(uint8_t pin, int value);
```
- *pin* select the GPIO pin.
- *value* select the duty cycle of pwm. * range is from 0 (always off) to 255 (always on).

**analogWriteResolution**     
This function is used to set resolution for selected analogWrite pin.
```
void analogWriteResolution(uint8_t pin, uint8_t resolution);
```
- *pin* select the GPIO pin.
- *resolution* select resolution for analog channel.

**analogWriteFrequency**     
This function is used to set frequency for selected analogWrite pin.
```
void analogWriteFrequency(uint8_t pin, uint32_t freq);
```
- *pin* select the GPIO pin.
- *freq* select frequency of pwm.     

**Note:** In Core 3.0+, Espressif deprecated *ledcSetup* and *ledcAttachPin*. They introduced a streamlined API where channels are assigned automatically in the background. You no longer need to manage channel numbers (0–7) manually. You simply pass the physical GPIO pin directly into the configuration and write functions.    

## Code Examples (Arduino IDE)       
In the standard ESP32 Arduino core, PWM is handled natively using the **ledc** functions.     

**1. Fading an LED (Basic PWM)**     
This example sets up an 8-bit PWM channel on a GPIO pin to smoothly fade an LED up and down.     
```
// Define the GPIO pin
const int LED_PIN = 4;

// PWM Properties
const int PWM_FREQ = 5000;    // 5 kHz frequency
/*  Old API
const int PWM_CHANNEL = 0;   // Use PWM channel 0 (0-7 available)
*/
const int PWM_RES = 8;       // 8-bit resolution (0 - 255 values)

void setup() {
/* Old API
  // Configure the LEDC PWM channel with frequency and resolution
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  
  // Attach the physical GPIO pin to the configured PWM channel
  ledcAttachPin(LED_PIN, PWM_CHANNEL);
*/

// New API: Automatically configures an underlying channel and attaches the pin
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RES);
}

void loop() {
  // Fade inside loop (increasing brightness)
  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {
/* Old API
    ledcWrite(PWM_CHANNEL, dutyCycle);
*/
  // Pass the PIN directly to ledcWrite instead of the channel
    ledcWrite(LED_PIN, dutyCycle);
    delay(5);
  }

  // Fade outside loop (decreasing brightness)
  for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
/* Old API
    ledcWrite(PWM_CHANNEL, dutyCycle);
*/
  // Pass the PIN directly to ledcWrite instead of the channel
    ledcWrite(LED_PIN, dutyCycle);
    delay(5);
  }
}
```
This example (old code) sets up an 8-bit PWM channel on a GPIO pin to smoothly fade an LED up and down. 

**2. Controlling a RC Servo Motor (Specific Frequency & Resolution)**     
Standard hobby servos require a specific frame rate frequency of 50Hz and a pulse width that varies precisely between roughly 1ms and 2ms to position the motor shaft between 0 and 180 degrees.    

Using a 10-bit resolution (0 to 1023) at 50Hz, a 1ms pulse corresponds to a duty cycle value of roughly ~51, and a 2ms pulse corresponds to roughly ~102.     
```
const int SERVO_PIN = 5;

// Servo PWM Settings
const int SERVO_FREQ = 50;     // 50Hz frequency is standard for servos
const int SERVO_RES = 10;      // 10-bit resolution (0 - 1023)

// Calculated target values for standard 50Hz / 10-bit setup
const int SERVO_MIN = 26;      // ~0.5ms pulse (0 degrees)
const int SERVO_MAX = 123;     // ~2.4ms pulse (180 degrees)

void setup() {
  // NEW API: Bind the pin, frequency, and resolution directly.
  // The system assigns an internal hardware channel automatically.
  ledcAttach(SERVO_PIN, SERVO_FREQ, SERVO_RES);
}

void loop() {
  // Turn to 0 degrees (Pass the physical PIN instead of a channel)
  ledcWrite(SERVO_PIN, SERVO_MIN);
  delay(1000);

  // Turn to 90 degrees (midpoint)
  ledcWrite(SERVO_PIN, (SERVO_MIN + SERVO_MAX) / 2);
  delay(1000);

  // Turn to 180 degrees
  ledcWrite(SERVO_PIN, SERVO_MAX);
  delay(1000);
}
```

## MCPWM API    

Each A/B pair may be clocked by any one of the three timers Timer 0, 1 and 2. The same timer may be used to clock more than one pair of PWM outputs. Each unit is also able to collect inputs such as SYNC SIGNALS, detect FAULT SIGNALS like motor overcurrent or overvoltage, as well as obtain feedback with CAPTURE SIGNALS on e.g. a rotor position.     
Description of this API starts with configuration of MCPWM’s Timer and Generator submodules to provide the basic motor control functionality. Then it discusses more advanced submodules and functionalities of a Fault Handler, signal Capture and Carrier.

**Configure**     
The scope of configuration depends on the motor type, in particular how many outputs and inputs are required, and what will be the sequence of signals to drive the motor.

In this case we will describe a simple configuration to control a brushed DC motor that is using only some of the available MCPWM’s resources. An example circuit is shown below. It includes a H-Bridge to switch polarization of a voltage applied to the motor (M) and to provide sufficient current to drive it.

<img width="611" height="377" alt="image" src="https://github.com/user-attachments/assets/6bf92b0e-26a7-40cc-9984-dd0021bd1253" />

Configuration covers the following steps:
1. Selection of a MCPWM unit that will be used to drive the motor. There are two units available on-board of ESP32-S3 and enumerated in *mcpwm_unit_t*.
2. Initialization of two GPIOs as output signals within selected unit by calling *mcpwm_gpio_init()*. The two output signals are typically used to command the motor to rotate right or left. All available signal options are listed in *mcpwm_io_signals_t*. To set more than a single pin at a time, use function *mcpwm_set_pin()* together with *mcpwm_pin_config_t*.
3. Selection of a timer. There are three timers available within the unit. The timers are listed in *mcpwm_timer_t*.
4. Setting of the timer frequency and initial duty within *mcpwm_config_t* structure.
5. Setting timer resolution if necessary, by calling *mcpwm_group_set_resolution()* and *mcpwm_timer_set_resolution()*
6. Calling of *mcpwm_init()* with the above parameters to make the configuration effective.

**Operate**   
To operate a motor connected to the MCPWM unit, e.g. turn it left or right, or vary the speed, we should apply some control signals to the unit’s outputs. The outputs are organized into three pairs. Within a pair they are labeled “A” and “B” and each driven by a submodule called an “Generator”. To provide a PWM signal, the Operator itself, which contains two Generator, should be clocked by one of three available Timers. To make the API simpler, each Timer is automatically associated by the API to drive an Operator of the same index, e.g. Timer 0 is associated with Operator 0.

There are the following basic ways to control the outputs:     
- We can drive particular signal steady high or steady low with function *mcpwm_set_signal_high()* or *mcpwm_set_signal_low()*. This will make the motor to turn with a maximum speed or stop. Depending on selected output A or B the motor will rotate either right or left.
- Another option is to drive the outputs with the PWM signal by calling *mcpwm_start()* or *mcpwm_stop()*. The motor speed will be proportional to the PWM duty.
- To vary PWM’s duty call *mcpwm_set_duty()* and provide the duty value in %. Optionally, you may call *mcpwm_set_duty_in_us()*, if you prefer to set the duty in microseconds. Checking of currently set value is possible by calling *mcpwm_get_duty()*. Phase of the PWM signal may be altered by calling *mcpwm_set_duty_type()*. The duty is set individually for each A and B output using *mcpwm_generator_t* in specific function calls. The duty value refers either to high or low output signal duration. This is configured when calling *mcpwm_init()*, as discussed in section Configure, and selecting one of options from *mcpwm_duty_type_t*.

**Continue the following topics: Adjust, Synchronize, Capture, Fault Handler, Carrier, Interrupts, Resolution**

Refer to the following links for more details of the APIs    
- https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32s3/api-reference/peripherals/mcpwm.html
- https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/mcpwm.html


## References     

https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/ledc.html

https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/ledc.html

https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32s3/api-reference/peripherals/ledc.html

https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/mcpwm.html

https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32s3/api-reference/peripherals/mcpwm.html

https://www.oceanlabz.in/esp32-pwm/

https://randomnerdtutorials.com/esp32-pwm-arduino-ide/

https://deepbluembedded.com/esp32-pwm-tutorial-examples-analogwrite-arduino/

