#include <Arduino.h>
#include <Keypad.h>

// =====================================================
// 1. KEYPAD CONFIGURATION
// =====================================================

#define ROWS 4
#define COLS 4

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// ESP32-S3 GPIO assignments
byte rowPins[ROWS] = {15, 16, 17, 18};
byte colPins[COLS] = {19, 20, 21, 47};

Keypad keypad = Keypad(
  makeKeymap(keys),
  rowPins,
  colPins,
  ROWS,
  COLS
);

// =====================================================
// 2. FREERTOS CONFIGURATION
// =====================================================

#define KEYPAD_SCAN_INTERVAL  10
#define QUEUE_LENGTH          10

// Keypad event message
struct KeypadMessage {
  char key;
  KeyState state;
};

// FreeRTOS queue handle
QueueHandle_t keypadQueue = NULL;

// Task handles
TaskHandle_t keypadTaskHandle = NULL;
TaskHandle_t appTaskHandle = NULL;

// =====================================================
// 3. KEYPAD EVENT CALLBACK
// =====================================================

void keypadEvent(KeypadEvent key) {

  // Obtain current state of the key
  KeypadMessage msg;

  msg.key = key;
  msg.state = keypad.getState();

  // Send event to FreeRTOS queue
  // Do not block the keypad scanning task
  if (xQueueSend(keypadQueue, &msg, 0) != pdPASS) {

    // Queue full - event was dropped
    // Avoid Serial printing inside callback
  }
}

// =====================================================
// 4. KEYPAD SCANNING TASK
// =====================================================

void keypadTask(void *pvParameters) {

  TickType_t lastWakeTime = xTaskGetTickCount();

  const TickType_t scanPeriod =
      pdMS_TO_TICKS(KEYPAD_SCAN_INTERVAL);

  Serial.println("Keypad Task Started");

  while (1) {

    // Scan keypad and update key states.
    // Registered callback is invoked for key events.
    keypad.getKeys();

    // Maintain a periodic 10 ms scanning interval
    vTaskDelayUntil(&lastWakeTime, scanPeriod);
  }
}

// =====================================================
// 5. APPLICATION TASK
// =====================================================

void applicationTask(void *pvParameters) {

  KeypadMessage receivedMsg;

  Serial.println("Application Task Started");

  while (1) {

    // Wait for keypad event.
    // Blocks this task until an event arrives.
    if (xQueueReceive(
          keypadQueue,
          &receivedMsg,
          portMAX_DELAY) == pdPASS) {

      switch (receivedMsg.state) {

        case PRESSED:

          Serial.print("Key PRESSED: ");
          Serial.println(receivedMsg.key);
          break;

        case HOLD:

          Serial.print("Key HOLD: ");
          Serial.println(receivedMsg.key);
          break;

        case RELEASED:

          Serial.print("Key RELEASED: ");
          Serial.println(receivedMsg.key);
          break;

        case IDLE:
          break;
      }
    }
  }
}

// =====================================================
// 6. SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(500);

  Serial.println();
  Serial.println("ESP32-S3 FreeRTOS Keypad Demo");

  // Configure keypad timing
  keypad.setDebounceTime(20);
  keypad.setHoldTime(1000);

  // Create FreeRTOS queue
  keypadQueue = xQueueCreate(
    QUEUE_LENGTH,
    sizeof(KeypadMessage)
  );

  if (keypadQueue == NULL) {

    Serial.println("Queue creation failed!");
    while (1) {
      delay(1000);
    }
  }

  // Register keypad event callback
  keypad.addEventListener(keypadEvent);

  // Create keypad scanning task
  BaseType_t result1 = xTaskCreate(
    keypadTask,
    "KeypadTask",
    4096,
    NULL,
    2,
    &keypadTaskHandle
  );

  // Create application processing task
  BaseType_t result2 = xTaskCreate(
    applicationTask,
    "ApplicationTask",
    4096,
    NULL,
    1,
    &appTaskHandle
  );

  if (result1 != pdPASS || result2 != pdPASS) {

    Serial.println("Task creation failed!");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("FreeRTOS tasks created successfully");
}

// =====================================================
// 7. MAIN LOOP
// =====================================================

void loop() {

  // Application processing is handled by FreeRTOS tasks.
  vTaskDelay(pdMS_TO_TICKS(1000));
}
