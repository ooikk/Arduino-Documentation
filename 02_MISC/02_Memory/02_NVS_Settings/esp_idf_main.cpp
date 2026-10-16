/**
ESP-IDF equivalent using nvs_flash
*/

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

struct DeviceSettings {
  int brightness;               // 0-100
  bool autoMode;
  char deviceName[32];           // Up to 31 bytes plus '\0'
  float threshold;
};

const DeviceSettings DEFAULT_SETTINGS = {50, true, "ESP32-S3", 25.0f};

// Stable format for flash. Do not deserialize arbitrary flash bytes into bool.
struct StoredSettings {
  uint32_t version;
  int32_t brightness;
  uint8_t autoMode;              // Must be 0 or 1
  uint8_t reserved[3];
  char deviceName[32];
  float threshold;
  uint32_t crc;
};

constexpr uint32_t SETTINGS_VERSION = 1;
static_assert(sizeof(float) == 4, "This format requires a 32-bit float");
static_assert(offsetof(StoredSettings, crc) == 48, "Storage layout changed");
static_assert(sizeof(StoredSettings) == 52, "Storage layout changed");

// Application rule: threshold must be finite and between -1000 and +1000.
bool validateSettings(const DeviceSettings &s) {
  return s.brightness >= 0 && s.brightness <= 100 &&
         s.deviceName[0] != '\0' &&
         memchr(s.deviceName, '\0', sizeof(s.deviceName)) != nullptr &&
         isfinite(s.threshold) &&
         s.threshold >= -1000.0f && s.threshold <= 1000.0f;
}

uint32_t crc32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xFFFFFFFFu;
  while (length--) {
    crc ^= *data++;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
    }
  }
  return ~crc;
}

StoredSettings makeRecord(const DeviceSettings &s) {
  StoredSettings r;
  memset(&r, 0, sizeof(r));       // Initialize reserved bytes deterministically
  r.version = SETTINGS_VERSION;
  r.brightness = s.brightness;
  r.autoMode = s.autoMode ? 1 : 0;
  // Call only after validateSettings() succeeds.
  memcpy(r.deviceName, s.deviceName, strlen(s.deviceName));
  r.threshold = s.threshold;
  r.crc = crc32(reinterpret_cast<const uint8_t *>(&r),
                offsetof(StoredSettings, crc));
  return r;
}

bool decodeRecord(const StoredSettings &r, DeviceSettings &out) {
  if (r.version != SETTINGS_VERSION || r.autoMode > 1 ||
      r.crc != crc32(reinterpret_cast<const uint8_t *>(&r),
                     offsetof(StoredSettings, crc))) {
    return false;
  }
  DeviceSettings candidate{};
  candidate.brightness = r.brightness;
  candidate.autoMode = (r.autoMode == 1);
  memcpy(candidate.deviceName, r.deviceName, sizeof(candidate.deviceName));
  candidate.threshold = r.threshold;
  if (!validateSettings(candidate)) return false;
  out = candidate;               // Update RAM only after all checks pass
  return true;
}

#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "settings";

esp_err_t loadSettings(nvs_handle_t handle, DeviceSettings &out) {
  size_t length = 0;
  esp_err_t err = nvs_get_blob(handle, "settings", nullptr, &length);
  if (err != ESP_OK) return err;
  if (length != sizeof(StoredSettings)) return ESP_ERR_INVALID_SIZE;

  StoredSettings record{};
  err = nvs_get_blob(handle, "settings", &record, &length);
  if (err != ESP_OK) return err;
  if (record.version != SETTINGS_VERSION) return ESP_ERR_NOT_SUPPORTED;
  return decodeRecord(record, out) ? ESP_OK : ESP_ERR_INVALID_CRC;
}

esp_err_t saveSettings(nvs_handle_t handle, const DeviceSettings &settings) {
  if (!validateSettings(settings)) return ESP_ERR_INVALID_ARG;
  const StoredSettings record = makeRecord(settings);
  esp_err_t err = nvs_set_blob(handle, "settings", &record, sizeof(record));
  if (err != ESP_OK) return err;
  return nvs_commit(handle);     // Explicit commit is required in ESP-IDF
}

esp_err_t factoryReset(nvs_handle_t handle, DeviceSettings &settings) {
  esp_err_t err = nvs_erase_all(handle);  // Only this handle's namespace
  if (err != ESP_OK) return err;
  err = nvs_commit(handle);
  if (err != ESP_OK) return err;
  settings = DEFAULT_SETTINGS;
  return saveSettings(handle, settings);
}

extern "C" void app_main(void) {
  esp_err_t err = nvs_flash_init();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(err));
    // ESP_ERR_NVS_NO_FREE_PAGES / ESP_ERR_NVS_NEW_VERSION_FOUND
    // need deliberate recovery. This example preserves the partition.
    return;
  }

  nvs_handle_t handle;
  err = nvs_open("my-app", NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(err));
    return;
  }

  DeviceSettings settings = DEFAULT_SETTINGS;
  err = loadSettings(handle, settings);
  if (err == ESP_ERR_NVS_NOT_FOUND || err == ESP_ERR_INVALID_CRC) {
    ESP_LOGI(TAG, "Missing/invalid record; saving defaults");
    err = saveSettings(handle, settings);
  }
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Load/save failed: %s; keeping RAM defaults",
             esp_err_to_name(err));
    nvs_close(handle);           // Preserve incompatible/unreadable record
    return;
  }

  ESP_LOGI(TAG, "brightness=%d auto=%d name=%s threshold=%.2f",
           settings.brightness, settings.autoMode, settings.deviceName,
           static_cast<double>(settings.threshold));

  // In an application, call this when a user requests a save:
  // settings.brightness = 80;
  // err = saveSettings(handle, settings);
  // Check err before reporting success or calling esp_restart().

  // An explicit factory-reset action can call:
  // err = factoryReset(handle, settings);

  nvs_close(handle);             // Closing is not a substitute for commit
}
