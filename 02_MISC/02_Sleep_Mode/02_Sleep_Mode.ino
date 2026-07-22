#include <esp_sleep.h>

#define TIME_TO_SLEEP_SEC  10  // Sleep duration in seconds

// Variable stored in RTC memory — persists through Deep Sleep restarts
RTC_DATA_ATTR int bootCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Increment and display boot count
  bootCount++;
  Serial.printf("Boot number: %d\n", bootCount);

  // Print wake-up reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println("Woke up due to timer interrupt.");
  } else {
    Serial.println("Woke up from initial power-on or reset.");
  }

  // Setup timer wakeup trigger
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_SEC * 1000000ULL);

  Serial.println("Entering Deep Sleep now...");
  Serial.flush();

  // Start Deep Sleep (will reboot when timer expires)
  esp_deep_sleep_start();
}

void loop() {
  // This code will never be reached in Deep Sleep
}
