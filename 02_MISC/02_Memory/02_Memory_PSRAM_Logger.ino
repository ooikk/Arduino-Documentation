#include <Arduino.h>
#include "esp_heap_caps.h"
#include "esp_memory_utils.h"
#include "esp_system.h"

// ============================================================
// Configuration
// ============================================================

// Set to 4096 bytes (4 KiB) for the normal experiment.
// For a faster test, try 160 bytes = 10 records.
//#define BUFFER_BYTES (4U * 1024U)
#define BUFFER_BYTES (4U*20U)   // test with smaller buffer

#define SAMPLE_INTERVAL_MS 10000UL  // 10 seconds
#define SERIAL_BAUD        115200

// ============================================================
// Sensor data record
// ============================================================

struct SensorRecord {
  uint32_t sampleNo;       // Monotonically increasing counter
  uint32_t elapsedMs;      // Time since boot, in milliseconds
  float temperatureC;      // Simulated temperature
  float humidityPct;       // Simulated relative humidity
};

static_assert(
  BUFFER_BYTES >= sizeof(SensorRecord),
  "BUFFER_BYTES must fit at least one SensorRecord"
);

constexpr size_t RECORD_CAPACITY =
    BUFFER_BYTES / sizeof(SensorRecord);

constexpr size_t ACTUAL_BUFFER_BYTES =
    RECORD_CAPACITY * sizeof(SensorRecord);

// ============================================================
// Global state
// ============================================================

// Pointer to a buffer allocated explicitly from PSRAM.
static SensorRecord *sensorBuffer = nullptr;

static size_t recordsStored = 0;
static uint32_t sampleCounter = 0;
static uint32_t lastSampleMs = 0;

// ============================================================
// Simulated sensor
// ============================================================

float readSimulatedTemperature() {
  // Range: -10.0 to +49.9 degrees C
  return random(-100, 500) / 10.0f;
}

float readSimulatedHumidity() {
  // Range: 20.0 to 99.9 percent RH
  return random(200, 1000) / 10.0f;
}

// ============================================================
// Buffer diagnostics
// ============================================================

void printMemoryStatus() {
  Serial.printf("Configured buffer: %u bytes\n",
                (unsigned)BUFFER_BYTES);

  Serial.printf("Record size:       %u bytes\n",
                (unsigned)sizeof(SensorRecord));

  Serial.printf("Record capacity:   %u records\n",
                (unsigned)RECORD_CAPACITY);

  Serial.printf("Buffer storage:    %u bytes\n",
                (unsigned)ACTUAL_BUFFER_BYTES);

  Serial.printf("PSRAM free now:    %u bytes\n",
                (unsigned)heap_caps_get_free_size(
                  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

  Serial.printf("Internal heap free:%u bytes\n",
                (unsigned)heap_caps_get_free_size(
                  MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));

  Serial.printf("Buffer address:    %p\n",
                (void *)sensorBuffer);

  Serial.printf("In external RAM:   %s\n",
                esp_ptr_external_ram(sensorBuffer)
                  ? "YES" : "NO");
}

// ============================================================
// Send full buffer to Serial
// Later, replace this function with ESP-NOW transmission.
// ============================================================

void sendBufferToSerial() {
  Serial.println();
  Serial.println("========================================");
  Serial.println("BUFFER FULL - SENDING RECORDS TO SERIAL");
  Serial.printf("Records: %u\n", (unsigned)recordsStored);
  Serial.println("No,Elapsed_ms,Temperature_C,Humidity_pct");

  for (size_t i = 0; i < recordsStored; i++) {
    const SensorRecord &r = sensorBuffer[i];

    Serial.printf("%lu,%lu,%.1f,%.1f\n",
                  (unsigned long)r.sampleNo,
                  (unsigned long)r.elapsedMs,
                  r.temperatureC,
                  r.humidityPct);
  }

  Serial.println("END OF BUFFER");
  Serial.println("Buffer will now be reused.");
  Serial.println("========================================");

  // Wait until the serial output has actually been transmitted.
  Serial.flush();

  // Reuse the same PSRAM allocation; do not allocate again.
  recordsStored = 0;

  Serial.println("Buffer cleared logically; PSRAM allocation retained.");
}

// ============================================================
// Acquire one sample and store it in PSRAM
// ============================================================

void acquireAndStoreSample() {
  if (recordsStored >= RECORD_CAPACITY) {
    // Defensive check: do not write outside the buffer.
    sendBufferToSerial();
  }

  SensorRecord &r = sensorBuffer[recordsStored];

  r.sampleNo = ++sampleCounter;
  r.elapsedMs = millis();
  r.temperatureC = readSimulatedTemperature();
  r.humidityPct = readSimulatedHumidity();

  recordsStored++;

  Serial.printf(
    "[SAMPLE] No=%lu  Temp=%.1f C  Humidity=%.1f %%  "
    "Buffer=%u/%u\n",
    (unsigned long)r.sampleNo,
    r.temperatureC,
    r.humidityPct,
    (unsigned)recordsStored,
    (unsigned)RECORD_CAPACITY
  );

  // Send as soon as the buffer becomes full.
  if (recordsStored >= RECORD_CAPACITY) {
    sendBufferToSerial();
  }
}

// ============================================================
// Arduino setup
// ============================================================

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);

  Serial.println();
  Serial.println("ESP32-S3 PSRAM Sensor Logger");
  Serial.println("----------------------------------------");

  Serial.printf("PSRAM detected: %s\n",
                psramFound() ? "YES" : "NO");

  if (!psramFound()) {
    Serial.println("ERROR: PSRAM is unavailable.");
    Serial.println("Check the Arduino IDE PSRAM setting.");
    while (true) {
      delay(1000);
    }
  }

  // Allocate the entire buffer from external PSRAM.
  sensorBuffer = (SensorRecord *)heap_caps_malloc(
      BUFFER_BYTES,
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
  );

  if (sensorBuffer == nullptr) {
    Serial.println("ERROR: Could not allocate PSRAM buffer.");
    while (true) {
      delay(1000);
    }
  }

  if (!esp_ptr_external_ram(sensorBuffer)) {
    Serial.println("ERROR: Buffer is not in external RAM.");
    heap_caps_free(sensorBuffer);
    sensorBuffer = nullptr;

    while (true) {
      delay(1000);
    }
  }

  randomSeed(esp_random());

  printMemoryStatus();

  Serial.println("----------------------------------------");
  Serial.printf("Sampling interval: %lu ms\n",
                SAMPLE_INTERVAL_MS);
  Serial.println("Waiting for first sample...");
  Serial.println();

  lastSampleMs = millis();
}

// ============================================================
// Arduino loop
// ============================================================

void loop() {
  uint32_t now = millis();

  // Unsigned subtraction safely handles millis() rollover.
  if ((uint32_t)(now - lastSampleMs) >= SAMPLE_INTERVAL_MS) {
    // Schedule the next sample interval from this acquisition.
    lastSampleMs = now;

    acquireAndStoreSample();
  }

  delay(5);
}