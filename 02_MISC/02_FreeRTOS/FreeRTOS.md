# ESP32-S3 Learning Lab: FreeRTOS Multitasking and Synchronization

Advanced embedded systems tutorial · Arduino-ESP32 Core 3.3.x · ESP32-S3 · Practical DC motor control

## 1. Introduction: From CPU cores to concurrent software

Understanding CPU cores is only the first step toward developing reliable embedded applications. In a real ESP32-S3 project, several operations may need to run independently:

- Acquire sensor measurements at a regular interval.
- Calculate motor speed and execute a PID controller.
- Handle user commands through the serial port.
- Update displays, report status and communicate over Wi-Fi.
- Detect faults and stop the motor safely.

A program built around a single `loop()` function can perform all these operations, but the implementation becomes increasingly difficult to maintain when timing requirements differ. For example, a motor control loop might need to run every 10 ms, while a serial interface can tolerate occasional delays of 50–100 ms.

FreeRTOS provides a framework for organizing these operations into independent tasks, scheduling their execution and coordinating access to shared resources.

The ESP32-S3 uses a dual-core Xtensa LX7 processor. FreeRTOS lets us organize software execution across its cores, but multitasking does not mean every task executes simultaneously. Each core executes one task at a time, and the scheduler switches between eligible tasks according to their priorities and states.

The practical objective of this lesson is to build a motor-control architecture in which:

1. A data-acquisition task reads the motor encoder.
2. A control task calculates the motor's speed and PID output.
3. A user-interface task receives commands from the serial port.
4. A reporting task displays measurements and controller status.

The implementation will also demonstrate task priorities, core pinning, queues, mutexes, task notifications, race conditions, and watchdog considerations.

## 2. What FreeRTOS actually does

### 2.1 The scheduler and task execution

<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/b044765f-66a8-4c00-ad91-a240c0fa7120" />

[RTOS Task Scheduling and Prioritization](https://images.openai.com/static-rsc-4/Nbkc_7Moif4qRFrSZbla37pSUtWR4zzU7gpW6XeAgH1FMGWyiVgqHZW_3nYGUbKWR6O3YGiTVtX6-pmzmPukM1eywgJhN2Fta60Hf_hVwfbfmP9fNuc6qJiwNNk-Ei7gll371jphD5bkld1rJlK3SbjbWslLrOcwQCSP1czAZqU?purpose=inline)

[digikey.jp](https://www.digikey.jp/ja/maker/projects/introduction-to-rtos-solution-to-part-3-task-scheduling/8fbb9e0b0eed4279a2dd698f02ce125f)

FreeRTOS manages tasks using a scheduler. Each task has its own execution context, including its stack, priority and saved processor state.

The scheduler determines which ready task should execute on each CPU core. When a task blocks while waiting for a queue, semaphore, notification or time delay, another eligible task can run.

For the ESP32-S3, Arduino-ESP32 uses the ESP-IDF FreeRTOS implementation. Some implementation details differ from a conventional single-core FreeRTOS configuration, so use the ESP-IDF and Arduino-ESP32 documentation corresponding to the installed core version when investigating advanced behavior.

### 2.2 Task states

| State     | Meaning                                   | Motor-control example                          |
| --------- | ----------------------------------------- | ---------------------------------------------- |
| Running   | Currently executing on a CPU core         | PID task calculating the PWM output            |
| Ready     | Can execute but is waiting for CPU time   | Serial task waiting for its turn               |
| Blocked   | Waiting for an event or time condition    | Acquisition task waiting for its next sample   |
| Suspended | Explicitly suspended; not eligible to run | A maintenance task paused by the application   |
| Deleted   | Task has been removed from scheduling     | A temporary diagnostic task that has completed |

A task does not need to run continuously to be useful. Blocking while waiting for new data is usually preferable to continuously polling a condition.

For example:

```cpp
vTaskDelay(pdMS_TO_TICKS(100));
```

This blocks the calling task for approximately 100 milliseconds in scheduler ticks. It does not stop the entire ESP32.

For periodic control work, however, `vTaskDelayUntil()` is often more appropriate because it schedules iterations against a fixed timing reference rather than simply waiting a relative period after each iteration.

### 2.3 Task priorities

A task with a higher priority normally takes precedence over a lower-priority ready task on the same core.

For our application, a reasonable starting design is:

| Task                | Example priority | Reason                                         |
| ------------------- | ---------------- | ---------------------------------------------- |
| Motor control       | 5                | Time-sensitive feedback and output calculation |
| Encoder acquisition | 4                | Regular measurement collection                 |
| Serial interface    | 2                | User input is not time-critical                |
| Status reporting    | 1                | Logging can tolerate delays                    |

These are illustrative priorities, not universal ESP32-S3 recommendations. The actual values should be tested against the system's other tasks, interrupt load and timing requirements.

Important: A high priority does not guarantee an accurate control period. Long critical sections, interrupt latency, blocking operations and other high-priority work can still cause timing problems.

### 2.4 Task pinning: using both cores

ESP32 Arduino exposes `xTaskCreatePinnedToCore()`:

```cpp
xTaskCreatePinnedToCore(
    controlTask,       // Task function
    "MotorControl",    // Debugging name
    4096,              // Stack size in bytes in ESP-IDF
    nullptr,           // Parameter passed to the task
    5,                 // Priority
    &controlHandle,    // Optional task handle
    1                  // Core ID
);
```

On the dual-core ESP32-S3, the available core IDs are normally 0 and 1.

In this example, the control task is pinned to core 1. A serial-interface task might be pinned to core 0. Pinning can reduce competition between selected tasks, but it does not reserve the whole core for one task.

The ESP-IDF system tasks, Wi-Fi, Bluetooth and interrupts may still consume processor time. In Arduino-ESP32, core 1 is commonly used for the Arduino `loopTask`, but this is an implementation detail to verify for the installed framework configuration.

The `xTaskCreatePinnedToCore()` stack-size argument is expressed in bytes in ESP-IDF FreeRTOS. This differs from some upstream FreeRTOS examples that use stack-depth units.

A task can also be created using `xTaskCreate()` when pinning is unnecessary.

## 3. Inter-task communication and synchronization

Tasks execute independently, but they often need to exchange data or share hardware resources. FreeRTOS provides several synchronization mechanisms, each intended for a different purpose.

Queues — transfer data between tasks

An acquisition task sends measured RPM to the control task. The receiving task can block until a measurement arrives, instead of polling continuously.

Mutexes — protect shared resources

A mutex can protect a shared settings or status structure from simultaneous task access. It is also useful when multiple tasks need exclusive access to a resource.

Semaphores — signal events or manage resources

A binary semaphore can signal that an external event has occurred. A counting semaphore can represent multiple available resources or pending events.

Task notifications — signal a particular task efficiently

An encoder or timer-related event can notify a control task that work is ready. Notifications can also carry a small integer value or event bits.

### 3.1 Why shared variables cause race conditions

Suppose one task updates the measured RPM:

```cpp
measuredRPM = newRPM;
```

At the same time, another task reads the value:

```cpp
float rpm = measuredRPM;
```

This simple example might work on many occasions, but it becomes unsafe if the shared state involves multiple related fields, a non-atomic operation or a read-modify-write sequence. A reader might observe an inconsistent combination of values.

Declaring a variable `volatile` does not make an operation atomic and does not provide mutual exclusion.

On ESP32, short critical sections can protect certain shared variables:

```cpp
portMUX_TYPE dataMux = portMUX_INITIALIZER_UNLOCKED;

volatile int32_t encoderCount = 0;

void IRAM_ATTR encoderISR() {
  portENTER_CRITICAL_ISR(&dataMux);
  encoderCount++;
  portEXIT_CRITICAL_ISR(&dataMux);
}
```

The corresponding task must use the matching task-context critical-section functions when reading and resetting the count.

In practice, an encoder ISR should be kept as short as possible. For simple pulse counting, the critical-section example demonstrates the principle, but the exact ISR and counter design should be selected according to the encoder signal rate and hardware. ESP32's PCNT peripheral is often a better solution for high-rate pulse counting.

### 3.2 A queue is often better than a shared variable

Consider this data flow:

Acquisition task

Reads encoder count and calculates RPM

FreeRTOS queue

Transfers the latest measurement

PID control task

Calculates the new motor PWM command

The queue provides a defined transfer mechanism. The producer and consumer do not need to access the same mutable measurement structure at the same time.

For motor control, a queue of length one can be used as a latest-value mailbox. The producer can overwrite the old measurement when the queue is full, preventing an accumulation of stale readings.

```cpp
QueueHandle_t rpmQueue;

void setup() {
  rpmQueue = xQueueCreate(1, sizeof(float));

  if (rpmQueue == nullptr) {
    Serial.println("Failed to create RPM queue");
    return;
  }
}
```

To send the latest RPM:

```cpp
xQueueOverwrite(rpmQueue, &rpm);
```

To receive it:

```cpp
float rpm;

if (xQueueReceive(rpmQueue, &rpm, portMAX_DELAY) == pdPASS) {
  // Process the latest received measurement.
}
```

`xQueueOverwrite()` is intended for queues of length one. It is particularly useful for continuously updated measurements where the newest value matters more than every intermediate value.

### 3.3 Mutexes versus critical sections

A mutex is suitable for protecting shared data when tasks might need to access the same structure for a meaningful period.

```cpp
SemaphoreHandle_t statusMutex;

statusMutex = xSemaphoreCreateMutex();
```

A task can acquire it before updating shared status:

```text
if (xSemaphoreTake(statusMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
  // Update shared status fields.
  xSemaphoreGive(statusMutex);
}
```

A mutex supports priority inheritance, which helps address certain priority-inversion problems. A binary semaphore does not provide the same mutex semantics.

Do not take a mutex from an ISR. Use an ISR-safe semaphore API or task notification when interrupt code needs to signal a task.

Also avoid holding a mutex while waiting for serial input, performing network operations or executing other potentially lengthy actions.

## 4. Application design: closed-loop DC motor speed control

The lab will implement a basic closed-loop speed controller using an ESP32-S3, a DC motor, a motor driver and an encoder.

[Positioning with the N20 miniature geared DC motors — Curious Scientist](https://images.openai.com/static-rsc-4/-0GauxhFfzifdNqm7XHQcx1sdBSfHPZSIkQuqiGCGv-DZMUbgM9OJ18kDtDy8O3Of64ELSAyTSi4VpiTblJ5VHULGrrO3iEHF37yaL4rkQfcBFxyIN4wf9pQ7C_tVyyK6MxuMmNHh86i0dJLCTgkhW0QviaVpVCbb3yA--3NKGQ?purpose=inline)

[curiousscientist.tech](https://curiousscientist.tech/blog/positioning-with-the-n20-miniature-geared-dc-motors)

### 4.1 Hardware assumptions

| Component          | Example configuration                     |
| ------------------ | ----------------------------------------- |
| Controller         | ESP32-S3 DevKitC-1                        |
| Motor              | Low-voltage brushed DC motor with encoder |
| Driver             | TB6612FNG or another compatible H-bridge  |
| PWM                | GPIO 6, 20 kHz, 10-bit duty               |
| Direction          | GPIO 7 and GPIO 15                        |
| Encoder            | GPIO 16, one pulse channel                |
| Encoder resolution | 20 counted pulses per motor revolution    |
| Supply             | Appropriate external motor supply         |

These GPIO assignments are examples, not a guarantee that every ESP32-S3 board exposes the pins in the same way. Confirm the actual board pinout and avoid pins assigned to onboard peripherals or flash/PSRAM. The motor must use an appropriate driver and external supply; never power it directly from an ESP32 GPIO.

The encoder resolution must be calibrated. If the encoder specifies pulses per revolution at the motor shaft versus the gearbox output shaft, account for the gearbox ratio and the selected counting method.

### 4.2 Understanding the PID controller

The controller compares the target speed with the measured speed.

\[ e(t)=r(t)-y(t) \]

where:

- \(r(t)\) is the target RPM.
- \(y(t)\) is the measured RPM.
- \(e(t)\) is the speed error.

The PID control output is:

\[ u(t)=K_p e(t)+K_i\int e(t)\\,dt+ K_d\frac{de(t)}{dt} \]

In a digital controller, the integral and derivative terms are approximated at regular intervals. A simple implementation uses:

\[ I_k=I\_{k-1}+e_k\Delta t \]

\[ D_k=\frac{e_k-e\_{k-1}}{\Delta t} \]

\[ u_k=K_p e_k+K_i I_k+K_d D_k \]

The result is limited to the available PWM range. The integral term must also be managed to prevent excessive accumulation when the motor cannot reach the target speed.

For this introductory lab, the derivative gain is set to zero initially. Proportional-integral (PI) control is often a practical starting point for DC motor speed regulation because encoder measurements can be noisy.

### 4.3 Application architecture

The application divides work into four tasks.

Task 1 — Acquisition (core 1)

Priority 4 · 100 ms sample period

Reads the encoder count, calculates RPM and publishes a measurement through a queue.

Task 2 — PID control (core 1)

Priority 5 · 100 ms nominal period

Receives speed measurements, computes the controller output and adjusts PWM.

Task 3 — Serial interface (core 0)

Priority 2

Receives `START`, `STOP`, `SET` and `STATUS` commands without blocking the control task.

Task 4 — Reporting (core 0)

Priority 1

Periodically reports target RPM, measured RPM and PWM duty to the serial monitor.

The user-interface task sends commands through a queue. The control task owns the motor's control state and PWM updates. This single-owner design is important: it prevents the serial task and PID task from independently changing motor output.

## 5. Complete Arduino application example

This example is designed for Arduino IDE 2.3.x with Arduino-ESP32 Core 3.3.x. It demonstrates the multitasking architecture and uses the LEDC API available in current Arduino-ESP32 releases.

It is a teaching implementation, not a certified motor-control or functional-safety system. Add suitable hardware emergency-stop circuitry, overcurrent protection, appropriate motor-driver fault handling and safe startup behavior before using it on real machinery.

### 5.1 Serial command interface

Open the Arduino IDE Serial Monitor at 115200 baud and enter one command per line.

| Command   | Function                                        |
| --------- | ----------------------------------------------- |
| `START`   | Enable speed control and ramp toward the target |
| `STOP`    | Disable the controller and set PWM to zero      |
| `SET 120` | Set the target speed to 120 RPM                 |
| `SET 300` | Set the target speed to 300 RPM                 |
| `STATUS`  | Request an immediate status report              |
| `HELP`    | Display the command list                        |

The target speed is limited to a configurable maximum. The example starts with the motor stopped, a target of 120 RPM and conservative PI gains that must be tuned for the actual motor.

### 5.2 Full code

```cpp

#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================
// 1. Hardware configuration
// ============================================================

constexpr int MOTOR_PWM_PIN = 6;
constexpr int MOTOR_IN1_PIN = 7;
constexpr int MOTOR_IN2_PIN = 15;
constexpr int MOTOR_STBY_PIN = 4;
constexpr int ENCODER_PIN = 16;

constexpr uint32_t PWM_FREQUENCY = 20000;
constexpr uint8_t PWM_RESOLUTION = 10;
constexpr int PWM_MAX = (1 << PWM_RESOLUTION) - 1;

// Counted encoder pulses per revolution at the measured shaft.
constexpr float PULSES_PER_REV = 20.0f;

constexpr uint32_t SAMPLE_PERIOD_MS = 100;
constexpr float SAMPLE_TIME_S = SAMPLE_PERIOD_MS / 1000.0f;
constexpr float MAX_RPM = 500.0f;

// ============================================================
// 2. Shared objects and data structures
// ============================================================

portMUX_TYPE encoderMux = portMUX_INITIALIZER_UNLOCKED;

volatile uint32_t encoderPulses = 0;

QueueHandle_t rpmQueue = nullptr;
QueueHandle_t commandQueue = nullptr;

SemaphoreHandle_t statusMutex = nullptr;

TaskHandle_t acquisitionHandle = nullptr;
TaskHandle_t controlHandle = nullptr;
TaskHandle_t serialHandle = nullptr;
TaskHandle_t reportHandle = nullptr;

enum CommandType : uint8_t {
  CMD_START,
  CMD_STOP,
  CMD_SET_RPM,
  CMD_STATUS,
  CMD_HELP
};

struct MotorCommand {
  CommandType type;
  float value;
};

struct MotorStatus {
  float targetRPM;
  float measuredRPM;
  float outputPWM;
  bool running;
};

MotorStatus motorStatus = {120.0f, 0.0f, 0.0f, false};

// ============================================================
// 3. Encoder interrupt
// ============================================================

void IRAM_ATTR encoderISR() {
  portENTER_CRITICAL_ISR(&encoderMux);
  encoderPulses++;
  portEXIT_CRITICAL_ISR(&encoderMux);
}

// ============================================================
// 4. Motor output functions
// ============================================================

void setMotorPWM(int duty) {
  duty = constrain(duty, 0, PWM_MAX);
  ledcWrite(MOTOR_PWM_PIN, duty);
}

void stopMotor() {
  setMotorPWM(0);
  digitalWrite(MOTOR_IN1_PIN, LOW);
  digitalWrite(MOTOR_IN2_PIN, LOW);
  digitalWrite(MOTOR_STBY_PIN, LOW);
}

void startMotor() {
  // This example operates in the forward direction only.
  digitalWrite(MOTOR_IN1_PIN, HIGH);
  digitalWrite(MOTOR_IN2_PIN, LOW);
  digitalWrite(MOTOR_STBY_PIN, HIGH);
}

// ============================================================
// 5. Task 1: encoder acquisition and RPM measurement
// ============================================================

void acquisitionTask(void *parameter) {
  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    vTaskDelayUntil(
      &lastWake,
      pdMS_TO_TICKS(SAMPLE_PERIOD_MS)
    );

    uint32_t pulses;

    portENTER_CRITICAL(&encoderMux);
    pulses = encoderPulses;
    encoderPulses = 0;
    portEXIT_CRITICAL(&encoderMux);

    float rpm =
      (pulses / PULSES_PER_REV) *
      (60.0f / SAMPLE_TIME_S);

    // A length-one queue stores the newest measurement.
    xQueueOverwrite(rpmQueue, &rpm);
  }
}

// ============================================================
// 6. Task 2: PI/PID speed controller
// ============================================================

void controlTask(void *parameter) {
  TickType_t lastWake = xTaskGetTickCount();

  float targetRPM = 120.0f;
  float measuredRPM = 0.0f;
  float integral = 0.0f;
  float previousError = 0.0f;
  float output = 0.0f;

  bool running = false;

  // Initial PI gains: illustrative only.
  float Kp = 0.5f;
  float Ki = 0.2f;
  float Kd = 0.0f;

  for (;;) {
    vTaskDelayUntil(
      &lastWake,
      pdMS_TO_TICKS(SAMPLE_PERIOD_MS)
    );

    // Process any pending user commands.
    MotorCommand cmd;

    while (xQueueReceive(commandQueue, &cmd, 0) == pdPASS) {
      switch (cmd.type) {
        case CMD_START:
          running = true;
          integral = 0.0f;
          previousError = 0.0f;
          startMotor();
          break;

        case CMD_STOP:
          running = false;
          output = 0.0f;
          integral = 0.0f;
          stopMotor();
          break;

        case CMD_SET_RPM:
          targetRPM = constrain(cmd.value, 0.0f, MAX_RPM);
          break;

        case CMD_STATUS:
          // The reporting task also publishes status periodically.
          break;

        case CMD_HELP:
          // The serial task prints help directly.
          break;
      }
    }

    // Read the latest available RPM measurement.
    xQueuePeek(rpmQueue, &measuredRPM, 0);

    if (running) {
      float error = targetRPM - measuredRPM;

      float derivative =
        (error - previousError) / SAMPLE_TIME_S;

      // Candidate integral with simple output clamping.
      float candidateIntegral =
        integral + error * SAMPLE_TIME_S;

      float candidateOutput =
        Kp * error +
        Ki * candidateIntegral +
        Kd * derivative;

      // Limit the controller output to the PWM range.
      // Conditional integration reduces integral windup.
      if ((candidateOutput >= 0.0f &&
           candidateOutput <= PWM_MAX) ||
          (candidateOutput < 0.0f && error < 0.0f) ||
          (candidateOutput > PWM_MAX && error > 0.0f)) {
        integral = candidateIntegral;
      }

      output =
        Kp * error +
        Ki * integral +
        Kd * derivative;

      output = constrain(output, 0.0f, (float)PWM_MAX);

      setMotorPWM((int)output);
      previousError = error;
    } else {
      output = 0.0f;
      setMotorPWM(0);
    }

    // Publish a consistent snapshot for reporting.
    if (xSemaphoreTake(
          statusMutex, pdMS_TO_TICKS(5)) == pdTRUE) {

      motorStatus.targetRPM = targetRPM;
      motorStatus.measuredRPM = measuredRPM;
      motorStatus.outputPWM = output;
      motorStatus.running = running;

      xSemaphoreGive(statusMutex);
    }
  }
}

// ============================================================
// 7. Task 3: serial user interface
// ============================================================

void printHelp() {
  Serial.println("\nCommands:");
  Serial.println(" START     - Start motor speed control");
  Serial.println(" STOP      - Stop motor");
  Serial.println(" SET <rpm> - Set target RPM");
  Serial.println(" STATUS    - Request status report");
  Serial.println(" HELP      - Show this help");
}

void serialTask(void *parameter) {
  char line[64];
  size_t index = 0;

  for (;;) {
    while (Serial.available() > 0) {
      char ch = (char)Serial.read();

      if (ch == '\r') {
        continue;
      }

      if (ch == '\n') {
        line[index] = '\0';
        index = 0;

        // Parse complete commands only.
        if (strcmp(line, "START") == 0) {
          MotorCommand cmd = {CMD_START, 0.0f};
          xQueueSend(commandQueue, &cmd, 0);
          Serial.println("Start requested");

        } else if (strcmp(line, "STOP") == 0) {
          MotorCommand cmd = {CMD_STOP, 0.0f};
          xQueueSend(commandQueue, &cmd, 0);
          Serial.println("Stop requested");

        } else if (strcmp(line, "STATUS") == 0) {
          MotorCommand cmd = {CMD_STATUS, 0.0f};
          xQueueSend(commandQueue, &cmd, 0);

          Serial.println("Status is reported periodically.");

        } else if (strcmp(line, "HELP") == 0) {
          printHelp();

        } else if (strncmp(line, "SET ", 4) == 0) {
          char *endPtr = nullptr;
          float value = strtof(line + 4, &endPtr);

          if (endPtr != line + 4 &&
              *endPtr == '\0' &&
              isfinite(value) &&
              value >= 0.0f &&
              value <= MAX_RPM) {

            MotorCommand cmd = {CMD_SET_RPM, value};

            if (xQueueSend(commandQueue, &cmd, 0) == pdPASS) {
              Serial.printf("Target requested: %.1f RPM\n", value);
            } else {
              Serial.println("Command queue full");
            }
          } else {
            Serial.println("Invalid RPM. Use SET 0 to SET 500.");
          }

        } else if (line[0] != '\0') {
          Serial.println("Unknown command. Type HELP.");
        }

      } else if (index < sizeof(line) - 1) {
        line[index++] = ch;
      } else {
        // Reject an overlong input line.
        index = 0;
        Serial.println("Command too long");
      }
    }

    // No busy-waiting while the serial port is idle.
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ============================================================
// 8. Task 4: periodic status reporting
// ============================================================

void reportTask(void *parameter) {
  for (;;) {
    MotorStatus snapshot;
    bool valid = false;

    if (xSemaphoreTake(
          statusMutex, pdMS_TO_TICKS(10)) == pdTRUE) {

      snapshot = motorStatus;
      valid = true;

      xSemaphoreGive(statusMutex);
    }

    if (valid) {
      Serial.printf(
        "State:%s Target:%.1f RPM Measured:%.1f RPM PWM:%.1f%%\n",
        snapshot.running ? "RUN" : "STOP",
        snapshot.targetRPM,
        snapshot.measuredRPM,
        100.0f * snapshot.outputPWM / PWM_MAX
      );
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// ============================================================
// 9. Arduino setup and loop
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(MOTOR_IN1_PIN, OUTPUT);
  pinMode(MOTOR_IN2_PIN, OUTPUT);
  pinMode(MOTOR_STBY_PIN, OUTPUT);

  pinMode(ENCODER_PIN, INPUT_PULLUP);
  attachInterrupt(
    digitalPinToInterrupt(ENCODER_PIN),
    encoderISR,
    RISING
  );

  stopMotor();

  if (!ledcAttach(
        MOTOR_PWM_PIN,
        PWM_FREQUENCY,
        PWM_RESOLUTION)) {
    Serial.println("LEDC initialization failed");
    while (true) {
      delay(1000);
    }
  }

  // Queues and mutex must exist before tasks start.
  rpmQueue = xQueueCreate(1, sizeof(float));
  commandQueue = xQueueCreate(8, sizeof(MotorCommand));
  statusMutex = xSemaphoreCreateMutex();

  if (rpmQueue == nullptr ||
      commandQueue == nullptr ||
      statusMutex == nullptr) {
    Serial.println("FreeRTOS resource allocation failed");
    stopMotor();
    while (true) {
      delay(1000);
    }
  }

  float initialRPM = 0.0f;
  xQueueOverwrite(rpmQueue, &initialRPM);

  BaseType_t result;

  result = xTaskCreatePinnedToCore(
    acquisitionTask, "Acquire", 3072, nullptr, 4,
    &acquisitionHandle, 1
  );
  configASSERT(result == pdPASS);

  result = xTaskCreatePinnedToCore(
    controlTask, "PIDControl", 4096, nullptr, 5,
    &controlHandle, 1
  );
  configASSERT(result == pdPASS);

  result = xTaskCreatePinnedToCore(
    serialTask, "SerialUI", 4096, nullptr, 2,
    &serialHandle, 0
  );
  configASSERT(result == pdPASS);

  result = xTaskCreatePinnedToCore(
    reportTask, "Reporting", 3072, nullptr, 1,
    &reportHandle, 0
  );
  configASSERT(result == pdPASS);

  Serial.println("ESP32-S3 FreeRTOS motor controller ready.");
  Serial.println("Motor is stopped. Type HELP for commands.");
}

void loop() {
  // Application work runs in the FreeRTOS tasks.
  vTaskDelay(pdMS_TO_TICKS(1000));
}

```

### 5.3 Important limitations of this first implementation

Before running the code, understand these limitations:

- Encoder direction: The example counts rising edges from one encoder channel. It does not determine direction or decode quadrature signals. It assumes forward rotation.
- RPM resolution: At 20 pulses per revolution and a 100 ms sample window, one pulse represents 30 RPM. Low-speed readings will therefore be coarse. Use a higher-resolution encoder, a longer measurement window or period-based measurement at low speeds.
- PID tuning: The example gains are placeholders. They must be tuned for the actual motor, load, driver and supply voltage. The simple integral clamp is educational rather than a complete anti-windup strategy.
- Startup ramp: The controller applies its output directly. A production implementation should ramp the target speed from zero and account for acceleration limits.
- Stop latency: `STOP` is processed by the control task, so the response can take up to approximately one control period under normal conditions, and longer under load. It is not a safety-rated emergency stop.
- Command queue: The serial task does not check the queue-send result for every command. Under heavy input, commands such as `STOP` could be lost. A production design should guarantee delivery of critical commands and give stop requests explicit priority.
- Task creation: `configASSERT()` behavior depends on the build configuration. Robust production code should explicitly handle task-creation failures and shut down safely if any required task cannot start.
- Reporting: The status task writes to `Serial`, while the serial task also writes to it. For more robust output, centralize serial transmission in one reporting task and send messages through a logging queue.

The example uses a mutex to publish a coherent status snapshot, a queue to transfer encoder measurements, a command queue to transfer user requests, and a short critical section to protect the encoder counter.

## 6. Step-by-step walkthrough of the code

### Step 1 — Initialize the hardware and RTOS resources

The `setup()` function initializes the motor driver pins, PWM generator, encoder interrupt and FreeRTOS communication objects.

For example:

```cpp
rpmQueue = xQueueCreate(1, sizeof(float));
commandQueue = xQueueCreate(8, sizeof(MotorCommand));
statusMutex = xSemaphoreCreateMutex();
```

These statements create three distinct resources:

- `rpmQueue`: holds one floating-point RPM measurement.
- `commandQueue`: holds up to eight pending motor commands.
- `statusMutex`: protects the shared motor-status structure.

The resources must be created successfully before any task attempts to use them.

The code then creates the four tasks. Each task receives a function pointer, a debugging name, a stack allocation, an optional parameter, a priority, a task handle and a core affinity.

### Step 2 — Acquire the encoder measurements

The encoder generates electrical pulses as the motor rotates. The interrupt service routine increments a counter for each rising edge.

The acquisition task reads and resets this counter every 100 ms.

Suppose the encoder produces 40 pulses during one sample window and the configured resolution is 20 pulses per revolution:

\[ RPM=\frac{40}{20}\times\frac{60}{0.1} =1200\ RPM \]

The task then publishes the calculated value through `rpmQueue`.

This separates hardware event handling from measurement processing. The interrupt performs minimal work, while the task handles the calculation.

### Step 3 — Execute the PID controller

The control task waits for its next scheduled iteration using `vTaskDelayUntil()`.

It then:

1. Retrieves pending commands.
2. Updates the target speed or run/stop state.
3. Reads the latest RPM measurement.
4. Calculates the speed error.
5. Updates the integral and derivative calculations.
6. Limits the output to the available PWM range.
7. Updates the motor driver.
8. Publishes the latest controller status.

The output PWM duty is expressed as a value between 0 and 1023 for 10-bit resolution.

For example, a PWM output of 512 represents approximately 50% duty cycle.

This does not necessarily mean the motor runs at 50% of its maximum RPM. Motor speed depends on load, supply voltage, driver losses, friction and the motor's electrical characteristics. The PID controller adjusts the PWM duty to compensate for these effects.

### Step 4 — Process serial commands

The serial task reads incoming characters and assembles them into a complete command line. It does not execute the PID algorithm or directly update PWM.

For `SET 200`, it parses the requested speed and places a `CMD_SET_RPM` message in the command queue.

The control task subsequently consumes the message and updates its own target-speed variable.

This is an important embedded-software design principle: the task that owns a resource should normally be responsible for modifying that resource.

### Step 5 — Report results

The reporting task copies the status structure while holding the mutex, releases the mutex, and then prints the snapshot.

Releasing the mutex before printing is important because serial output can be relatively slow. Other tasks should not have to wait for the serial transmission to finish before accessing motor status.

Example output:

```text
ESP32-S3 FreeRTOS motor controller ready.
Motor is stopped. Type HELP for commands.
State:STOP Target:120.0 RPM Measured:0.0 RPM PWM:0.0%
State:RUN Target:120.0 RPM Measured:96.0 RPM PWM:18.2%
State:RUN Target:120.0 RPM Measured:115.0 RPM PWM:21.4%
State:RUN Target:120.0 RPM Measured:120.0 RPM PWM:20.1%
```

These lines illustrate the intended behavior; they are not measured results from a physical motor.

## 7. Race conditions and synchronization exercises

The code provides several opportunities to understand common concurrency errors.

| Experiment       | Incorrect implementation                                     | Correct approach                                                      |
| ---------------- | ------------------------------------------------------------ | --------------------------------------------------------------------- |
| Encoder counting | Read and reset the counter without synchronization           | Protect the counter using an appropriate critical section or use PCNT |
| RPM transfer     | Allow multiple tasks to modify a shared measurement          | Transfer measurements through a queue                                 |
| Motor state      | Let the serial task and control task independently write PWM | Keep PWM ownership in the control task                                |
| Status reporting | Read multiple status fields while another task updates them  | Copy the status under a mutex                                         |
| Command delivery | Ignore failed queue sends                                    | Check delivery and define priority for critical commands              |

### What is priority inversion?

Suppose a low-priority reporting task holds a mutex. A high-priority control task needs the same mutex and blocks while waiting for it. Meanwhile, a medium-priority task continues executing and prevents the low-priority task from releasing the mutex promptly.

The high-priority task is now indirectly delayed by lower-priority work. This is called priority inversion.

FreeRTOS mutexes support priority inheritance, which can temporarily raise the priority of the task holding the mutex. This helps mitigate the problem but does not eliminate the need for short, carefully designed critical sections.

For a motor controller, avoid putting mutex-protected operations inside the time-critical path unless necessary. Keep status updates short, avoid lengthy calculations while holding locks, and never perform serial output while holding the status mutex.

## 8. Task notifications: a more efficient event-driven design

The current acquisition task uses a periodic schedule. Another design is to notify the control task when new data is available.

For example, after completing a measurement:

```cpp
xTaskNotifyGive(controlHandle);
```

The receiving task can wait for the notification:

```cpp
ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
```

This blocks the control task until a notification arrives, reducing unnecessary polling.

However, the design must account for the required control frequency. If the controller must execute at a fixed 10 ms interval regardless of encoder events, a purely event-driven notification is not a substitute for the periodic control schedule.

For more advanced implementations, notifications can wake the control task while a hardware timer or periodic scheduler maintains the control deadline. Task notifications are also useful when a task must respond to a single event without requiring a full queue.

## 9. Watchdogs and fault handling

A multitasking application can become unresponsive even when the scheduler itself is working correctly.

Common causes include:

- An infinite loop that never blocks or yields.
- A task that performs lengthy operations without giving other required work a chance to execute.
- Deadlock caused by incorrect locking.
- Repeated high-priority work that starves other tasks.
- A control task that waits indefinitely for a resource or event.

ESP-IDF provides watchdog facilities, including the Task Watchdog Timer (TWDT) and Interrupt Watchdog Timer (IWDT). Their configuration and monitored tasks depend on the installed ESP-IDF and Arduino-ESP32 version.

The task watchdog can detect tasks or monitored execution contexts that fail to yield or reset their watchdog subscription within the configured interval. The interrupt watchdog detects excessively delayed interrupt processing.

For a motor-control system, watchdog recovery should be paired with a defined safe-output policy. A software reset alone does not guarantee that a motor stops safely. Design the hardware so that loss of the control signal, reset or power failure leads to an appropriate safe state.

A useful extension to this lab is to monitor the age of the last valid encoder measurement. If the measurement becomes stale, the application should report a fault and disable motor drive according to the chosen safety strategy.

## 10. Practical lab sequence

Complete the following exercises in order.

## Lab checklist

0 of 7 complete

1\. Create and observe independent tasks

Create acquisition, control and reporting tasks. Print each task's name and core ID using the appropriate ESP-IDF task APIs. Observe how task priorities affect execution.

2\. Add queue-based measurements

Send RPM measurements through a queue. Temporarily slow the consumer and compare a length-one overwrite queue with a normal FIFO queue.

3\. Implement the serial interface

Add START, STOP, SET and STATUS. Verify that malformed commands do not change the target speed and that the serial interface cannot block PID execution.

4\. Tune the PI controller

Start with Kd = 0. Adjust Kp, then increase Ki gradually. Record settling time, overshoot and steady-state error for different target speeds.

5\. Introduce and fix a race condition

Temporarily remove status synchronization. Update related fields from separate tasks, observe inconsistencies where they occur, and restore mutex-protected snapshots.

6\. Measure timing and core loading

Record the elapsed time of each control iteration. Compare the nominal 100 ms period with actual execution intervals and investigate missed deadlines.

7\. Add fault detection

Simulate missing encoder pulses and queue failures. Implement a stale-measurement timeout and a documented safe-stop response.

Reset checklist

## 11. Further improvements for a production-quality motor controller

After completing the basic lab, consider these enhancements:

- Higher-resolution encoder acquisition: Use the ESP32-S3 PCNT peripheral, quadrature decoding or hardware-supported capture methods to improve speed estimation.
- Setpoint ramping: Gradually increase the target RPM instead of requesting an immediate speed change.
- Controller anti-windup: Implement conditional integration or back-calculation to prevent integral accumulation when the output saturates.
- Derivative filtering: If derivative control is required, filter encoder noise before it causes abrupt PWM changes.
- Command prioritization: Ensure STOP requests cannot be silently lost when the command queue is full.
- Timing diagnostics: Measure execution time, jitter, stack high-water marks and task runtime statistics.
- Fault handling: Detect stalled motors, disconnected encoders, driver faults, overcurrent and stale measurements.
- Configuration persistence: Store calibrated encoder resolution, controller gains and maximum speed using the Arduino `Preferences` API, with validated defaults.
- Network integration: Add Wi-Fi or OTA tasks without allowing network activity to block the control loop.

## 12. Key takeaways

FreeRTOS is more than a way to run code on two CPU cores. It provides a structured way to organize concurrent software, define execution priorities, exchange data safely and handle timing requirements.

For this DC motor application, the main lessons are:

1. Use separate tasks for operations with different timing requirements.
2. Use priorities and core pinning deliberately; neither guarantees deterministic timing.
3. Use queues for inter-task data transfer and mutexes for shared resources.
4. Keep interrupt handlers short and protect shared data appropriately.
5. Keep motor-output ownership in one task.
6. Measure actual timing, monitor task health and define safe behavior for faults.

Once these concepts are understood, the same architecture can be adapted to ESP32-S3 applications involving sensor acquisition, display updates, data logging, wireless communications and other real-time embedded systems.
