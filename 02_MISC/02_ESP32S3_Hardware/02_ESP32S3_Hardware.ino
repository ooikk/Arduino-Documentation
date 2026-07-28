/*
Troubleshooting / Notes:
1. PSRAM shows 0 bytes? If PSRAM Total Size prints 0 MB, it means PSRAM is not enabled in your IDE. 
   In the Arduino IDE, go to Tools > PSRAM and change it to OPI PSRAM (since the R8 variant uses Octal SPI, not Quad SPI).
2. Flash Size shows 4MB or 8MB? If the physical flash size prints lower than 16MB, your IDE is restricting it. 
   Go to Tools > Flash Size and ensure it is set to 16MB (128Mb).
3. Internal RAM: The ESP32-S3 has roughly 512KB of internal SRAM. You will see it fluctuates slightly based on 
   what the core initializes on boot.
*/

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial monitor time to open
  
  Serial.println("\n\n========== ESP32-S3 Hardware Information ==========");
  
  // 1. Basic Chip Information
  Serial.printf("Chip Model:        %s\n", ESP.getChipModel());
  Serial.printf("Chip Revision:     %d\n", ESP.getChipRevision());
  Serial.printf("CPU Cores:         %d\n", ESP.getChipCores());
  Serial.printf("CPU Frequency:     %d MHz\n", ESP.getCpuFreqMHz());
  
  // 2. Flash Memory Information
  uint32_t flashSize = ESP.getFlashChipSize();
  Serial.printf("Flash Size:        %d MB (%d bytes)\n", flashSize / (1024 * 1024), flashSize);
  Serial.printf("Flash Speed:       %d MHz\n", ESP.getFlashChipSpeed() / 1000000);
  
  // Map Flash Mode integer to readable string
  int flashMode = ESP.getFlashChipMode();
  const char* flashModeStr = "Unknown";
  switch(flashMode) {
      case 0: flashModeStr = "QIO (Quad I/O)"; break;
      case 1: flashModeStr = "QOUT (Quad Output)"; break;
      case 2: flashModeStr = "DIO (Dual I/O)"; break;
      case 3: flashModeStr = "DOUT (Dual Output)"; break;
      case 4: flashModeStr = "Fast Read"; break;
      case 5: flashModeStr = "Slow Read"; break;
  }
  Serial.printf("Flash Mode:        %s\n", flashModeStr);

  // 3. PSRAM (External RAM) Information
  uint32_t psramSize = ESP.getPsramSize();
  Serial.printf("PSRAM Total Size:  %d MB (%d bytes)\n", psramSize / (1024 * 1024), psramSize);
  Serial.printf("PSRAM Free:        %d bytes\n", ESP.getFreePsram());
  
  // 4. Internal Heap (RAM) Information
  Serial.printf("Total Internal RAM:%d bytes\n", ESP.getHeapSize());
  Serial.printf("Free Internal RAM: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Min Free Heap:     %d bytes\n", ESP.getMinFreeHeap()); // Lowest free heap since boot
  Serial.printf("Max Alloc Heap:    %d bytes\n", ESP.getMaxAllocHeap()); // Largest contiguous block

  // 5. Sketch (Firmware) Information
  Serial.printf("Sketch Size:       %d bytes\n", ESP.getSketchSize());
  Serial.printf("Free Sketch Space: %d bytes\n", ESP.getFreeSketchSpace());

  // 6. Unique Chip ID (Derived from MAC address / eFuse)
  uint64_t chipId = ESP.getEfuseMac();
  // Format as a 12-character hex string (standard MAC format without colons)
  Serial.printf("Chip ID (eFuse MAC): %04X%08X\n", (uint16_t)(chipId >> 32), (uint32_t)chipId);
  
  Serial.println("==================================================\n");
}

void loop() {
  // Put your main code here, to run repeatedly.
  // We only need to run the hardware check once on boot.
}