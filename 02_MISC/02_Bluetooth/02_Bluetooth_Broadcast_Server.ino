/* ESP32-S3 BLE Server - Ultra Low Power Broadcaster */

#include <NimBLEDevice.h>

#define TEST_MODE

#ifdef TEST_MODE
#define SLEEP_DURATION_SEC 10 // 10 seconds
#else
#define SLEEP_DURATION_SEC  1800 // 30 mins interval
#endif
#define BROADCAST_TIME_MS   3000 // Broadcast for 3 seconds

struct SensorPayload {
  float temperature;
  float humidity;
  uint32_t bootCount;
};

RTC_DATA_ATTR uint32_t bootCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000); // Give ESP32-S3 USB Serial time to connect

  bootCount++;
  Serial.println("\n=================================");
  Serial.printf("📡 ESP32-S3 Broadcaster Woke Up! Boot #%u\n", bootCount);

  // 1. Prepare Payload Data
  SensorPayload data;
  data.temperature = 24.5 + (random(-10, 10) / 10.0);
  data.humidity = 60.0 + (random(-20, 20) / 10.0);
  data.bootCount = bootCount;

  // 2. Initialize NimBLE
  NimBLEDevice::init("ESP32_Sensor");
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();

  // 3. Construct Manufacturer Data Payload
  NimBLEAdvertisementData advertData;
  std::string payloadStr((char*)&data, sizeof(data));
  advertData.setManufacturerData("\xFF\xFF" + payloadStr);

  pAdvertising->setAdvertisementData(advertData);

  // 4. Start Broadcasting
  pAdvertising->start();
  Serial.printf("📡 Broadcasting sensor data for %d seconds...\n", BROADCAST_TIME_MS / 1000);
  Serial.printf("Temp: %.2f°C | Hum: %.2f%% | Server Boot: %u\n", data.temperature, data.humidity, data.bootCount);

  delay(BROADCAST_TIME_MS);

  // 5. Stop Advertising and Enter Deep Sleep
  pAdvertising->stop();
  Serial.printf("😴 Going to deep sleep for %d seconds...\n", SLEEP_DURATION_SEC);
  Serial.println("=================================\n");

  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_DURATION_SEC * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {
  // Deep sleep resets the CPU, so loop() is never reached
}
