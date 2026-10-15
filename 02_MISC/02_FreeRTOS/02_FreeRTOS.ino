
#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIMULATION_MODE

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

MotorStatus motorStatus = { 120.0f, 0.0f, 0.0f, false };

#ifdef SIMULATION_MODE
// Initial PI gains: illustrative only.
float Kp = 3.5f;
float Ki = 0.6f;
float Kd = 0.0f;

SemaphoreHandle_t controllerMutex = nullptr;

struct ControllerState {
  float kState;
  float iState;
  float dstate;
};

ControllerState controllerState = { 0.0f, 0.0f, 0.0f };
#endif



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
      pdMS_TO_TICKS(SAMPLE_PERIOD_MS));

    uint32_t pulses;

    portENTER_CRITICAL(&encoderMux);
    pulses = encoderPulses;
    encoderPulses = 0;
    portEXIT_CRITICAL(&encoderMux);

    float rpm =
      (pulses / PULSES_PER_REV) * (60.0f / SAMPLE_TIME_S);

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
#ifdef SIMULATION_MODE
  float myMeasuredRPM = 0.0f;
#endif
  bool running = false;

#ifndef SIMULATION_MODE
  // Initial PI gains: illustrative only.
  float Kp = 3.5f;
  float Ki = 0.6f;
  float Kd = 0.0f;
#endif

  for (;;) {
    vTaskDelayUntil(
      &lastWake,
      pdMS_TO_TICKS(SAMPLE_PERIOD_MS));

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

#ifdef SIMULATION_MODE
      float plantOutput = (float)(output * MAX_RPM) / PWM_MAX;
      float stepChangeRPM = constrain(plantOutput - myMeasuredRPM, -5.0f, 5.0f);

      //float stepChangeRPM = constrain(targetRPM - myMeasuredRPM, -2.0f, 2.0f);

      myMeasuredRPM += stepChangeRPM;

      measuredRPM = myMeasuredRPM + random(-3, 3);
#endif

      float error = targetRPM - measuredRPM;

      float derivative =
        (error - previousError) / SAMPLE_TIME_S;

      // Candidate integral with simple output clamping.
      float candidateIntegral =
        integral + error * SAMPLE_TIME_S;

      float candidateOutput =
        Kp * error + Ki * candidateIntegral + Kd * derivative;

#ifdef SIMULATION_MODE
      integral = candidateIntegral;
#else
      // Limit the controller output to the PWM range.
      // Conditional integration reduces integral windup.
      if ((candidateOutput >= 0.0f && candidateOutput <= PWM_MAX) || (candidateOutput < 0.0f && error < 0.0f) || (candidateOutput > PWM_MAX && error > 0.0f)) {
        integral = candidateIntegral;
      }
#endif
      output =
        Kp * error + Ki * integral + Kd * derivative;

      output = constrain(output, 0.0f, (float)PWM_MAX);

      setMotorPWM((int)output);
      previousError = error;

#ifdef SIMULATION_MODE
      if (xSemaphoreTake(
            controllerMutex, pdMS_TO_TICKS(5))
          == pdTRUE) {

        controllerState.kState = error;
        controllerState.iState = integral;
        controllerState.dstate = derivative;


        xSemaphoreGive(controllerMutex);
      }
#endif

    } else {
      output = 0.0f;
      setMotorPWM(0);
    }

    // Publish a consistent snapshot for reporting.
    if (xSemaphoreTake(
          statusMutex, pdMS_TO_TICKS(5))
        == pdTRUE) {

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
          MotorCommand cmd = { CMD_START, 0.0f };
          xQueueSend(commandQueue, &cmd, 0);
          Serial.println("Start requested");

        } else if (strcmp(line, "STOP") == 0) {
          MotorCommand cmd = { CMD_STOP, 0.0f };
          xQueueSend(commandQueue, &cmd, 0);
          Serial.println("Stop requested");

        } else if (strcmp(line, "STATUS") == 0) {
          MotorCommand cmd = { CMD_STATUS, 0.0f };
          xQueueSend(commandQueue, &cmd, 0);

          Serial.println("Status is reported periodically.");

        } else if (strcmp(line, "HELP") == 0) {
          printHelp();

        } else if (strncmp(line, "SET ", 4) == 0) {
          char *endPtr = nullptr;
          float value = strtof(line + 4, &endPtr);

          if (endPtr != line + 4 && *endPtr == '\0' && isfinite(value) && value >= 0.0f && value <= MAX_RPM) {

            MotorCommand cmd = { CMD_SET_RPM, value };

            if (xQueueSend(commandQueue, &cmd, 0) == pdPASS) {
              Serial.printf("Target requested: %.1f RPM\n", value);
            } else {
              Serial.println("Command queue full");
            }
          } else {
            Serial.println("Invalid RPM. Use SET 0 to SET 500.");
          }
#ifdef SIMULATION_MODE
        } else if (strncmp(line, "P ", 2) == 0) {

          char *endPtr = nullptr;
          float value = strtof(line + 2, &endPtr);

          if (endPtr != line + 2 && *endPtr == '\0' && isfinite(value) && value >= 0.0f && value <= 10) {
            Kp = value;
          }

        } else if (strncmp(line, "I ", 2) == 0) {

          char *endPtr = nullptr;
          float value = strtof(line + 2, &endPtr);

          if (endPtr != line + 2 && *endPtr == '\0' && isfinite(value) && value >= 0.0f && value <= 10) {
            Ki = value;
          }

        } else if (strncmp(line, "D ", 2) == 0) {

          char *endPtr = nullptr;
          float value = strtof(line + 2, &endPtr);

          if (endPtr != line + 2 && *endPtr == '\0' && isfinite(value) && value >= 0.0f && value <= 10) {
            Kd = value;
          }

#endif
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
          statusMutex, pdMS_TO_TICKS(10))
        == pdTRUE) {

      snapshot = motorStatus;
      valid = true;

      xSemaphoreGive(statusMutex);
    }

#ifdef SIMULATION_MODE
    ControllerState tempState;
    if (xSemaphoreTake(
          controllerMutex, pdMS_TO_TICKS(5))
        == pdTRUE) {
      tempState = controllerState;
      xSemaphoreGive(controllerMutex);
    }
#endif

    if (valid) {

#ifdef SIMULATION_MODE
      Serial.printf("Kp:%0.1f Ki:%0.1f Kd:%0.1f ", Kp, Ki, Kd);

      Serial.printf(" err:%.1f int:%.1f der:%.1f ",
                    tempState.kState, tempState.iState, tempState.dstate);


#endif

      Serial.printf(
        "State:%s Target:%.1f RPM Measured:%.1f RPM PWM:%.1f%%\n",
        snapshot.running ? "RUN" : "STOP",
        snapshot.targetRPM,
        snapshot.measuredRPM,
        100.0f * snapshot.outputPWM / PWM_MAX);
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
    RISING);

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

#ifdef SIMULATION_MODE
  controllerMutex = xSemaphoreCreateMutex();
#endif

  if (rpmQueue == nullptr || commandQueue == nullptr || statusMutex == nullptr
#ifdef SIMULATION_MODE
      || controllerMutex == nullptr
#endif
  ) {
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
    &acquisitionHandle, 1);
  configASSERT(result == pdPASS);

  result = xTaskCreatePinnedToCore(
    controlTask, "PIDControl", 4096, nullptr, 5,
    &controlHandle, 1);
  configASSERT(result == pdPASS);

  result = xTaskCreatePinnedToCore(
    serialTask, "SerialUI", 4096, nullptr, 2,
    &serialHandle, 0);
  configASSERT(result == pdPASS);

  result = xTaskCreatePinnedToCore(
    reportTask, "Reporting", 3072, nullptr, 1,
    &reportHandle, 0);
  configASSERT(result == pdPASS);

  Serial.println("ESP32-S3 FreeRTOS motor controller ready.");
  Serial.println("Motor is stopped. Type HELP for commands.");
}

void loop() {
  // Application work runs in the FreeRTOS tasks.
  vTaskDelay(pdMS_TO_TICKS(1000));
}
