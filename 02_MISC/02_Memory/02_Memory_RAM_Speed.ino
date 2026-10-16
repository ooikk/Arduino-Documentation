
#include <Arduino.h>
#include "esp_heap_caps.h"

static void runCopyTest(const char *name, uint8_t *src,
                        uint8_t *dst, size_t bytes) {
  for (size_t i = 0; i < bytes; ++i) {
    src[i] = (uint8_t)(i * 17U);
  }

  // Warm up caches and memory paths.
  memcpy(dst, src, bytes);

  const uint32_t start = micros();

  for (int i = 0; i < 10; ++i) {
    memcpy(dst, src, bytes);
  }

  const uint32_t elapsed = micros() - start;

  volatile uint8_t checksum = 0;
  for (size_t i = 0; i < bytes; i += 4096) {
    checksum ^= dst[i];
  }

  const float totalMB = (float)bytes * 10.0f / 1000000.0f;
  const float seconds = elapsed / 1000000.0f;
  const float mbps = seconds > 0 ? totalMB / seconds : 0;

  Serial.printf("%s: %u bytes x 10, %u us, %.2f MB/s, checksum=%u\n",
                name, (unsigned)bytes, (unsigned)elapsed,
                mbps, (unsigned)checksum);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  if (!psramFound()) {
    Serial.println("PSRAM not found. Check OPI PSRAM configuration.");
    return;
  }

  const size_t internalBytes = 64 * 1024;
  const size_t psramBytes = 1024 * 1024;
  //const size_t psramBytes = 64 * 1024;


  uint8_t *isrc = (uint8_t *)heap_caps_malloc(
    internalBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  uint8_t *idst = (uint8_t *)heap_caps_malloc(
    internalBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  uint8_t *psrc = (uint8_t *)heap_caps_malloc(
    psramBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  uint8_t *pdst = (uint8_t *)heap_caps_malloc(
    psramBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  if (!isrc || !idst || !psrc || !pdst) {
    Serial.println("Allocation failed.");
    free(isrc);
    free(idst);
    free(psrc);
    free(pdst);
    return;
  }

  Serial.println("Copy speed test (placement is explicit)");
  runCopyTest("Internal 64 KB", isrc, idst, internalBytes);
  runCopyTest("PSRAM 1 MB", psrc, pdst, psramBytes);

  free(isrc);
  free(idst);
  free(psrc);
  free(pdst);
}

void loop() {}
