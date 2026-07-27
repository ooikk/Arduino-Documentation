/* ESP32-S3 BLE Client - Synchronized Low Power Observer */

#include <NimBLEDevice.h>

#define TEST_MODE

#ifdef TEST_MODE

#define CYCLE_INTERVAL_SEC  13     // Sleep + Broadcast time. Target: 30 minutes
#define SAFETY_MARGIN_SEC   2      // Wake 20s early to catch drift
#define SCAN_TIMEOUT_MS     6000   // Timeout if server missed

#else
#define CYCLE_INTERVAL_SEC  1800    // Target: 30 minutes
#define SAFETY_MARGIN_SEC   20      // Wake 20s early to catch drift
#define SCAN_TIMEOUT_MS     45000   // Timeout if server missed
#endif

RTC_DATA_ATTR bool isSynced = false;
bool dataReceived = false;

struct SensorPayload {
  float temperature;
  float humidity;
  uint32_t bootCount;
};

class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
    if (advertisedDevice->haveManufacturerData()) {
      std::string mData = advertisedDevice->getManufacturerData();
      
      if (mData.length() == (2 + sizeof(SensorPayload))) {
        if ((uint8_t)mData[0] == 0xFF && (uint8_t)mData[1] == 0xFF) {
          SensorPayload data;
          memcpy(&data, mData.data() + 2, sizeof(SensorPayload));

          Serial.println("\n✅ Broadcast Captured!");
          Serial.printf("Temp: %.2f°C | Hum: %.2f%% | Server Boot: %u\n", 
                        data.temperature, data.humidity, data.bootCount);

          dataReceived = true;
          NimBLEDevice::getScan()->stop();
        }
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(1000);

  NimBLEDevice::init("ESP32_Observer");
  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setScanCallbacks(new ScanCallbacks());
  pScan->setActiveScan(false);

  if (!isSynced) {
    Serial.println("⚠️ Unsynced! Scanning continuously until first broadcast...");
    pScan->start(0, false); // 0 = Scan indefinitely in background
  } else {
    Serial.println("🔍 Synced wake-up! Scanning for server broadcast...");
    pScan->start(SCAN_TIMEOUT_MS, false); // Pass milliseconds!
  }

  // Active wait loop for both continuous (0) and windowed (6000ms) scans
  while (pScan->isScanning() && !dataReceived) {
    delay(10); // Yield CPU to let NimBLE process background packets
  }

  // Handle Scan Outcome
  if (dataReceived) {
    isSynced = true;
    
    int64_t calculatedSleep = (int64_t)CYCLE_INTERVAL_SEC - (int64_t)SAFETY_MARGIN_SEC;
    if (calculatedSleep < 1) calculatedSleep = 1;
    
    uint64_t sleepTimeSec = (uint64_t)calculatedSleep;
    Serial.printf("😴 Sleeping for %llu seconds until next window...\n\n", sleepTimeSec);
    
    NimBLEDevice::deinit(true);
    esp_sleep_enable_timer_wakeup(sleepTimeSec * 1000000ULL);
    esp_deep_sleep_start();

  } else {
    Serial.println("\n❌ Broadcast missed or timed out! Resetting sync...");
    isSynced = false;
    
    NimBLEDevice::deinit(true);
    esp_sleep_enable_timer_wakeup(2 * 1000000ULL);
    esp_deep_sleep_start();
  }
}

void loop() {}
