
#include <Arduino.h>
#include "esp_heap_caps.h"

#include "esp_memory_utils.h"


struct Allocation {
  const char *label;
  void *ptr;
};

static Allocation allocations[8];
static size_t allocationCount = 0;

void printMemory(const char *tag) {
  Serial.printf("\n--- %s ---\n", tag);

  Serial.printf("Flash:           %u bytes\n",
                (unsigned)ESP.getFlashChipSize());
  Serial.printf("PSRAM detected:  %s\n",
                psramFound() ? "YES" : "NO");
  Serial.printf("PSRAM total:     %u bytes\n",
                (unsigned)ESP.getPsramSize());

  Serial.printf("Internal free:   %u bytes\n",
                (unsigned)heap_caps_get_free_size(
                  MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  Serial.printf("Internal largest:%u bytes\n",
                (unsigned)heap_caps_get_largest_free_block(
                  MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));

  Serial.printf("PSRAM free:      %u bytes\n",
                (unsigned)heap_caps_get_free_size(
                  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  Serial.printf("PSRAM largest:   %u bytes\n",
                (unsigned)heap_caps_get_largest_free_block(
                  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

  Serial.printf("Default free:    %u bytes\n",
                (unsigned)ESP.getFreeHeap());
  Serial.printf("Default max alloc:%u bytes\n",
                (unsigned)ESP.getMaxAllocHeap());
  Serial.printf("PSRAM max alloc: %u bytes\n",
                (unsigned)ESP.getMaxAllocPsram());
}



void reportPointer(const char *label, void *p) {
  if (p == nullptr) {
    Serial.printf("%s: NULL\n", label);
    return;
  }

  Serial.printf("%s at %p: ", label, p);

  if (esp_ptr_external_ram(p)) {
    Serial.println("EXTERNAL RAM (PSRAM)");
  } else if (esp_ptr_internal(p)) {
    Serial.println("INTERNAL RAM");
  } else {
    Serial.println("OTHER MEMORY REGION");
  }
}

void attempt(const char *label, size_t bytes, bool usePsram) {
  void *p = usePsram ? ps_malloc(bytes) : malloc(bytes);

  Serial.printf("%s %u bytes: %s\n",
                label, (unsigned)bytes,
                p ? "SUCCESS" : "FAILED");

  reportPointer(label, p);

  if (p && allocationCount < 8) {
    allocations[allocationCount++] = { label, p };
  } else if (p) {
    free(p);
  }

  printMemory(label);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\nESP32-S3 N16R8 memory lab");
  printMemory("START");

  const size_t sizes[] = {
    1024, 100 * 1024, 1024 * 1024, 4 * 1024 * 1024
  };

  for (size_t n : sizes) {
    attempt("malloc", n, false);
  }

  for (size_t n : sizes) {
    attempt("ps_malloc", n, true);
  }

  Serial.println("\nAll tests finished. Allocations retained.");
  printMemory("FINAL");
}

void loop() {
  delay(1000);
}
