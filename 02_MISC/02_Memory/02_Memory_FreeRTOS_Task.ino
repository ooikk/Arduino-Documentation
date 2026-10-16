
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static TaskHandle_t workerHandle = nullptr;

void workerTask(void *parameter) {
  for (;;) {
    // Simulate local data usage. Keep this modest.
    volatile uint32_t scratch[128];

    for (size_t i = 0; i < 128; ++i) {
      scratch[i] = (uint32_t)i * 3U;
    }

    Serial.printf("[Worker] Stack high-water mark: %u bytes remaining\n",
                  (unsigned)uxTaskGetStackHighWaterMark(nullptr));

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // ESP-IDF / Arduino-ESP32 xTaskCreate stack depth is specified
  // in bytes in this ESP-IDF configuration.
  BaseType_t result = xTaskCreate(
      workerTask,
      "MemoryWorker",
      4096,
      nullptr,
      1,
      &workerHandle
  );

  if (result != pdPASS) {
    Serial.println("Failed to create worker task");
  }
}

void loop() {
  Serial.printf("[Loop] free internal heap: %u bytes\n",
                (unsigned)heap_caps_get_free_size(
                  MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  delay(3000);
}
