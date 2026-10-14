#include <Arduino.h>

void core0Task(void *parameter) {
  while (true) {
    Serial.printf("Task A running on Core %d\n",
                  xPortGetCoreID());
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void core1Task(void *parameter) {
  while (true) {
    Serial.printf("Task B running on Core %d\n",
                  xPortGetCoreID());
    vTaskDelay(pdMS_TO_TICKS(1500));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  xTaskCreatePinnedToCore(
    core0Task, "Core0Task", 4096, nullptr, 1, nullptr, 0);

  xTaskCreatePinnedToCore(
    core1Task, "Core1Task", 4096, nullptr, 1, nullptr, 1);

  Serial.printf("setup/loop task core: %d\n", xPortGetCoreID());
}

void loop() {
  delay(1000);
}
