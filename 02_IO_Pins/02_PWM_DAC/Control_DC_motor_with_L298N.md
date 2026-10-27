# Control DC motor with L298N using LEDC


To control a DC motor's direction and speed on an ESP32-S3, you need an external **H-bridge motor driver** (such as the L298N, TB6612FNG, or DRV8833). The ESP32-S3 cannot drive a motor directly from its GPIO pins because motors demand far more current than the chip can provide.

An H-bridge requires three control lines per motor:

1. **PWM Pin (Speed):** Controls the average voltage using Pulse Width Modulation.  
2. **IN1 Pin (Direction):** Digital high/low to control spin direction.  
3. **IN2 Pin (Direction):** Digital low/high to control spin direction.

Here is a complete, working example using the modern **Arduino ESP32 Core 3.0+ API** (ledcAttach and ledcWrite) for open-loop speed control.

## Hardware Connection Guide 

| ESP32-S3 Pin | Motor Driver Pin | Description |
| :---- | :---- | :---- |
| **GPIO 4** | **PWM / ENA** | Controls motor speed |
| **GPIO 5** | **IN1** | Direction Control Line 1 |
| **GPIO 6** | **IN2** | Direction Control Line 2 |
| **GND** | **GND** | Ground (Must be shared with motor power supply GND) |

## Complete Code Example  

```cpp
// Define physical GPIO Pin Assignments
const int MOTOR_PWM_PIN = 4; // Speed control pin
const int MOTOR_IN1_PIN = 5; // Direction pin 1
const int MOTOR_IN2_PIN = 6; // Direction pin 2

// PWM Configuration Properties (Core 3.0+ style)
const int PWM_FREQ = 10000;  // 10 kHz frequency (prevents audible motor whining)
const int PWM_RES = 8;       // 8-bit resolution (Speed range: 0 to 255)

void setup() {
  // Initialize standard digital outputs for direction control
  pinMode(MOTOR_IN1_PIN, OUTPUT);
  pinMode(MOTOR_IN2_PIN, OUTPUT);

  // Initialize PWM on the speed pin using the Core 3.0+ API
  // This automatically sets the pin direction and handles channel routing
  ledcAttach(MOTOR_PWM_PIN, PWM_FREQ, PWM_RES);

  // Ensure the motor is completely stopped at startup
  stopMotor();
}

void loop() {
  // 1. Spin Forward and gradually accelerate to full speed
  setDirectionForward();
  for (int speed = 0; speed <= 255; speed++) {
    ledcWrite(MOTOR_PWM_PIN, speed);
    delay(15); // Smooth acceleration over ~3.8 seconds
  }
  
  delay(2000); // Maintain full speed forward for 2 seconds

  // 2. Safely brake the motor before changing directions
  stopMotor();
  delay(1000); // Rest for 1 second

  // 3. Spin Backward at a fixed medium speed (roughly 60% power)
  setDirectionBackward();
  ledcWrite(MOTOR_PWM_PIN, 150); 
  delay(3000); // Run backward for 3 seconds

  // 4. Stop again before restarting the loop
  stopMotor();
  delay(2000); // Rest for 2 seconds
}

// --- Motor Control Helper Functions ---

void setDirectionForward() {
  digitalWrite(MOTOR_IN1_PIN, HIGH);
  digitalWrite(MOTOR_IN2_PIN, LOW);
}

void setDirectionBackward() {
  digitalWrite(MOTOR_IN1_PIN, LOW);
  digitalWrite(MOTOR_IN2_PIN, HIGH);
}

void stopMotor() {
  // Pulling both direction lines to the same state creates an electronic brake
  digitalWrite(MOTOR_IN1_PIN, LOW);
  digitalWrite(MOTOR_IN2_PIN, LOW);
  ledcWrite(MOTOR_PWM_PIN, 0); // Set speed duty cycle to zero
}

```

## Code Architecture Breakdown 

- **`ledcAttach(MOTOR_PWM_PIN, 10000, 8)`**: Binds our speed pin to a 10 kHz signal with 8-bit resolution. We use a high 10 kHz frequency because lower frequencies (like 50 Hz) cause the physical motor windings to vibrate mechanically, creating an annoying high-pitched buzzing noise.
- **`ledcWrite(MOTOR_PWM_PIN, speed)`**: Accepts a value from 0 (motor completely off) to 255 (motor receiving full power from your driver board).
- **The Direction Truth Table**: H-bridges read the relative state of IN1 and IN2 to determine how current flows through the motor:
  - HIGH / LOW -> Current flows one way (Forward).
  - LOW / HIGH -> Current reverses (Backward).
  - LOW / LOW -> Motor stops immediately (Short-brake).

# Control DC motor with L298N using MCPWM 

Using **MCPWM** (Motor Control PWM) instead of LEDC is the ideal choice for driving an H-Bridge like the **L298N**.

While LEDC is designed for simple tasks like LED dimming, MCPWM is a dedicated hardware engine built specifically for motor control. It runs efficiently in the background, freeing up the main CPU cores for other tasks.

Because MCPWM is an advanced hardware peripheral, it is typically written using Espressif's official component drivers (IDF style), which work seamlessly within the Arduino IDE for the ESP32-S3.

## Hardware Connection Guide 

The wiring remains identical to the previous example. The L298N requires a speed pin (ENA) and two direction pins (IN1, IN2).

| ESP32-S3 Pin | L298N Driver Pin | Description |
| :---- | :---- | :---- |
| **GPIO 4** | **ENA** (PWM) | Motor Speed Control |
| **GPIO 5** | **IN1** | Direction Control Line 1 |
| **GPIO 6** | **IN2** | Direction Control Line 2 |
| **GND** | **GND** | Common Ground |

## Code Example: MCPWM Open-Loop Motor Control 

This code initializes the MCPWM peripheral using the modern **ESP-IDF v5.0+ / Arduino Core 3.0+** driver structure to control the speed via the ENA pin, while using standard GPIOs for direction.

```cpp
#include "driver/mcpwm_prelude.h"

// Pin Assignments
const int MOTOR_PWM_PIN = 4; // ENA on L298N
const int MOTOR_IN1_PIN = 5; // IN1 on L298N
const int MOTOR_IN2_PIN = 6; // IN2 on L298N

// MCPWM Hardware Handles
mcpwm_timer_handle_t timer = NULL;
mcpwm_operator_handle_t oper = NULL;
mcpwm_cmpr_handle_t comparator = NULL;
mcpwm_gen_handle_t generator = NULL;

// Global settings
const uint32_t PWM_FREQ = 10000;       // 10 kHz frequency
const uint32_t TIMER_RESOLUTION = 10000000; // 10 MHz internal clock resolution
const uint32_t PWM_PERIOD = TIMER_RESOLUTION / PWM_FREQ; // Total ticks per cycle (1000 ticks)

void setup() {
  // 1. Configure standard direction pins
  pinMode(MOTOR_IN1_PIN, OUTPUT);
  pinMode(MOTOR_IN2_PIN, OUTPUT);
  stopMotor();

  // 2. Initialize MCPWM Timer
  mcpwm_timer_config_t timer_config = {
    .group_id = 0,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = TIMER_RESOLUTION,
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    .period_ticks = PWM_PERIOD,
  };
  mcpwm_new_timer(&timer_config, &timer);

  // 3. Initialize MCPWM Operator
  mcpwm_operator_config_t operator_config = {
    .group_id = 0,
  };
  mcpwm_new_operator(&operator_config, &oper);
  mcpwm_operator_connect_timer(oper, timer);

  // 4. Initialize MCPWM Comparator (Manages Duty Cycle threshold)
  mcpwm_comparator_config_t comparator_config = {
    .flags = { .update_cmp_on_tez = true } // Update duty cycle when timer hits zero
  };
  mcpwm_new_comparator(oper, &comparator_config, &comparator);

  // 5. Initialize MCPWM Generator (Binds to the physical GPIO)
  mcpwm_generator_config_t generator_config = {
    .gen_gpio_num = MOTOR_PWM_PIN,
  };
  mcpwm_new_generator(oper, &generator_config, &generator);

  // 6. Set Generator Waveform Actions
  // When timer resets to zero, drive the pin HIGH
  mcpwm_generator_set_action_on_timer_event(generator, 
    MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
  // When timer matches the comparator value, drive the pin LOW
  mcpwm_generator_set_action_on_compare_event(generator, 
    MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW));

  // 7. Enable and Start the Hardware Timer
  mcpwm_timer_enable(timer);
  mcpwm_timer_start(timer);

  // Start at 0% speed
  setMotorSpeed(0);
}

void loop() {
  // Forward Acceleration
  setDirectionForward();
  for (int percent = 0; percent <= 100; percent += 5) {
    setMotorSpeed(percent);
    delay(100);
  }
  delay(2000);

  // Brake
  stopMotor();
  delay(1000);

  // Backward Run at 60% speed
  setDirectionBackward();
  setMotorSpeed(60);
  delay(3000);

  // Brake
  stopMotor();
  delay(2000);
}

// --- Helper Functions ---

void setMotorSpeed(int speedPercent) {
  // Constrain input between 0 and 100%
  speedPercent = constrain(speedPercent, 0, 100);
  
  // Calculate target hardware ticks based on percentage
  uint32_t compare_ticks = (PWM_PERIOD * speedPercent) / 100;
  
  // Directly update the hardware comparator register
  mcpwm_comparator_set_compare_value(comparator, compare_ticks);
}

void setDirectionForward() {
  digitalWrite(MOTOR_IN1_PIN, HIGH);
  digitalWrite(MOTOR_IN2_PIN, LOW);
}

void setDirectionBackward() {
  digitalWrite(MOTOR_IN1_PIN, LOW);
  digitalWrite(MOTOR_IN2_PIN, HIGH);
}

void stopMotor() {
  digitalWrite(MOTOR_IN1_PIN, LOW);
  digitalWrite(MOTOR_IN2_PIN, LOW);
  if (comparator != NULL) {
    setMotorSpeed(0);
  }
}
```
## Why this is highly efficient: 

- **Zero CPU overhead for PWM:** The generator actions are linked directly via internal hardware logic cells. When the timer ticks up and hits the comparator threshold, the hardware switches the physical pin state in nanoseconds without firing an interrupt or consuming CPU cycles.
- **Precise Math (`PWM_PERIOD`):** With a 10 MHz resolution clock, a 10 kHz PWM signal yields exactly `1000` steps of resolution. This allows you to scale your speed parameters flawlessly.


# Charging a capacitor 

<img width="435" height="233" alt="image" src="https://github.com/user-attachments/assets/60748d2e-a03c-4e98-86f6-0974bc35f78e" />


To find the equations for the capacitor voltage $U_C(t)$ *U_C(t)* and the circuit current *i(t)* during the charging phase, we can set up and solve a first-order linear differential equation using **Kirchhoff's Voltage Law (KVL)**.

Here is the step-by-step mathematical breakdown.

## Step 1: Apply Kirchhoff's Voltage Law (KVL) 

Looking at the loop in your circuit diagram, the total voltage supplied by the battery *U_Batt* must equal the sum of the voltage drops across the resistor *U_R(t)* and the capacitor *U_C(t)*:

<img width="196" height="47" alt="image" src="https://github.com/user-attachments/assets/d1dd6a74-179a-40ba-9141-eec49eec5706" />


**Step 2: Express the components in terms of current *i(t)***

Using fundamental electronics relationships:

1. **Ohm's Law** for the resistor:

<img width="139" height="32" alt="image" src="https://github.com/user-attachments/assets/9b65168b-0748-42a6-b69e-14ebeb8223a9" />


2. **Current-voltage relationship** for a capacitor:

<img width="141" height="50" alt="image" src="https://github.com/user-attachments/assets/ada5d21b-5881-4bba-8345-91756067298f" />


Substitute the current equation into Ohm's Law to get *U_R(t)* in terms of *U_C(t)*:

<img width="174" height="58" alt="image" src="https://github.com/user-attachments/assets/fb38cea3-91be-45dd-9a79-71649fef72be" />


## Step 3: Set up the Differential Equation 

Substitute *U_R(t)* back into our original KVL equation from Step 1:

<img width="222" height="61" alt="image" src="https://github.com/user-attachments/assets/1a63b9f3-6434-48b9-b7d0-ea981f8d830b" />


To solve this, let's rearrange it into a standard first-order differential equation form:

<img width="243" height="125" alt="image" src="https://github.com/user-attachments/assets/c71a3882-8ad5-4934-8e28-df91bc460939" />


## Step 4: Integrate with Initial Conditions

Assuming the capacitor is completely uncharged at the beginning (*t=0*), our initial condition is *U_C(0)=0*.

Integrate both sides from time *0* to *t*:

<img width="332" height="65" alt="image" src="https://github.com/user-attachments/assets/34b7aec0-ecff-4bc7-9b1b-e5b01e3c1a29" />


Using substitution (<img width="178" height="41" alt="image" src="https://github.com/user-attachments/assets/33fd3738-45f4-4f77-a340-279be45ed248" />
):

<img width="363" height="121" alt="image" src="https://github.com/user-attachments/assets/8319a471-85c4-4479-a383-bca07962a180" />


Combine the natural logarithms:

<img width="229" height="73" alt="image" src="https://github.com/user-attachments/assets/5af3fb27-02c3-4312-b787-f49c7b9f3837" />


## Step 5: Solve for Capacitor Voltage *U_C(t)* 

Eliminate the natural log by taking the exponential ($e^x$) of both sides:

<img width="224" height="177" alt="image" src="https://github.com/user-attachments/assets/da0889e9-f34c-499a-a908-cfa54e9f3520" />


Factor out *U_Batt* to get your first target formula:

<img width="225" height="52" alt="image" src="https://github.com/user-attachments/assets/fa3ee770-ea59-440a-bfd2-32f2721a7829" />


## Step 6: Find the Current Function *i(t)*

Now that we have *U_C(t)*, we can determine the charging current equation by differentiating *U_C(t)* with respect to time, using the relation <img width="114" height="37" alt="image" src="https://github.com/user-attachments/assets/985ec257-a7d2-4bb0-a826-ae2620d73621" />
:

<img width="300" height="193" alt="image" src="https://github.com/user-attachments/assets/03846709-0d0b-4a87-aa6d-4dfa63184b55" />


The capacitance *C* cancels out, leaving you with your second target formula:

<img width="153" height="57" alt="image" src="https://github.com/user-attachments/assets/cccdcf49-3645-4d2d-9e09-3dc0bcaa30bf" />


# **Discharging a capacitor** 

To find the transfer function for the **capacitor voltage during discharge**, we look at how the stored charge flows out of the capacitor through the resistor when no external power source is connected.

Here is the analytical breakdown and derivation of the discharge equation.

## 1. Understanding the Circuit Behavior 

In a discharging RC circuit, the capacitor acts as a temporary voltage source. As current flows, energy is dissipated as heat in the resistor, causing both the voltage across the capacitor *U_C(t)* and the current *i(t)* to decay exponentially toward zero.

## 2. Setting up the Differential Equation 

By applying **Kirchhoff's Voltage Law (KVL)** around the single closed loop:

<img width="158" height="41" alt="image" src="https://github.com/user-attachments/assets/286c8817-1570-4ef2-8a70-56afee4f0566" />


Using the fundamental component behaviors:

* **Ohm's Law:** <img width="135" height="28" alt="image" src="https://github.com/user-attachments/assets/a80e263d-1521-4d56-b3f8-48daf5568d05" />

* **Capacitor Current:** Since the capacitor is *leaving* charge (discharging), the current flowing out relates to the voltage drop by:  
<img width="120" height="49" alt="image" src="https://github.com/user-attachments/assets/66b33a03-b987-483e-8d3e-f910c48c401a" />


Substituting Ohm's Law and the current equation into the KVL expression gives:

<img width="196" height="58" alt="image" src="https://github.com/user-attachments/assets/b68258fe-cfeb-4da5-902f-313568cd950c" />


## 3. Solving the Differential Equation 

To isolate the variables for integration, rearrange the terms:

<img width="216" height="121" alt="image" src="https://github.com/user-attachments/assets/13a5a2cd-9207-4c78-9cf6-2aba687c49a9" />


Integrate both sides from the start of discharge (*t=0*, where the initial capacitor voltage is *U_0*) to a future time *t*:

<img width="303" height="141" alt="image" src="https://github.com/user-attachments/assets/0f2fe3ef-671f-4992-844f-dceea9a212d4" />


Take the exponential ($e^x$) of both sides to eliminate the natural logarithm:

<img width="121" height="57" alt="image" src="https://github.com/user-attachments/assets/6e24b26d-2e15-4ec4-948c-c64434b65f93" />


Multiplying by *U_0* yields the standard time-domain equation for a discharging capacitor:

<img width="149" height="45" alt="image" src="https://github.com/user-attachments/assets/4f8dd21b-8000-4ca7-b65e-52c09675fcb1" />


## 4. Calculating the Specific Time Constant ($\tau$) 

The speed of the discharge depends entirely on the circuit's **time constant ($\tau$)**, defined as:

<img width="86" height="29" alt="image" src="https://github.com/user-attachments/assets/75249ed6-1d65-4de1-a827-0127988855bf" />


Using the values provided in your schematic:

<img width="352" height="91" alt="image" src="https://github.com/user-attachments/assets/23d2ef2e-e1e4-45d6-b464-3dc4a735698c" />


## Final Discharge Equation 

Plugging your component values into the transfer function reveals exactly how the voltage drops over time:

<img width="172" height="47" alt="image" src="https://github.com/user-attachments/assets/180f6785-cbcc-4f72-9ab3-5d7e63ec8aad" />

>For instance, after roughly 2.21 seconds, the capacitor voltage will have dropped to approximately 36.8% of its initial starting voltage *U_0*.

 

# Continue Charge & Discharging a capacitor (PWM)

Under PWM mode, the charging cycle start from none 0V, below is the universal RC charging equation:

<img width="323" height="64" alt="image" src="https://github.com/user-attachments/assets/ba49f6cb-d8fe-49a2-843c-fc27063f7503" />


Discharging equation:

<img width="158" height="45" alt="image" src="https://github.com/user-attachments/assets/62c452b2-2cbc-4236-a650-51802f74adc2" />


The following spreadsheet simulate the PWM on R-C circuit’s response:

Original doc: [Control DC motor with L298N](https://docs.google.com/document/d/1SvGCzkDtPiJGzOT2NCowSnUFJOa3VR7HOvcXryYpcvc/edit?tab=t.0)

Google Sheet: [RC Circuit](https://docs.google.com/spreadsheets/d/10Ih6O-8kVcTtxQ6LSR1iQ8BdQ8ajPfoUlZrpEv3AAdg/edit?gid=0#gid=0)

