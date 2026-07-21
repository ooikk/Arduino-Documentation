#include <Arduino.h>

// Define ADC1 pins for ESP32-S3 (Continuous mode ONLY supports ADC1 pins: GPIO 1 to 10)
uint8_t adc_pins[] = {4, 5, 6}; 
uint8_t adc_pins_count = sizeof(adc_pins) / sizeof(uint8_t);

// Number of raw conversions per pin taken per cycle (averaged automatically by the driver)
#define CONVERSIONS_PER_PIN 4 

// Target sampling frequency in Hz
#define SAMPLING_FREQ_HZ  1000//  20000 

// ISR Flag to indicate a DMA conversion batch has completed
volatile bool conversion_done = false;

// Buffer pointer structure supplied by the driver
adc_continuous_result_t *result = NULL;

// Interrupt Service Routine triggered when DMA buffer fills
void ARDUINO_ISR_ATTR adcCallback() {
  conversion_done = true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000); // Wait for Serial Monitor

  // 1. (Optional) Set Resolution (9 to 12 bits, default is 12)
  analogContinuousSetWidth(12);

  // 2. (Optional) Set Attenuation for continuous mode (default ADC_11db / ADC_12db)
  analogContinuousSetAtten(ADC_11db);

  // 3. Configure the Continuous ADC Peripheral:
  //    (Pins array, Pin count, Conversions per pin, Sample rate, ISR callback)
  if (analogContinuous(adc_pins, adc_pins_count, CONVERSIONS_PER_PIN, SAMPLING_FREQ_HZ, &adcCallback)) {
    Serial.println("ADC Continuous DMA initialized successfully!");
    
    // 4. Start the continuous conversions in the background
    analogContinuousStart();
  } else {
    Serial.println("Failed to initialize ADC Continuous mode!");
  }
}

void loop() {
  // Check if the ISR flagged new data from DMA
  if (conversion_done) {
    conversion_done = false; // Reset flag

    // Read the processed results into the result buffer pointer (timeout: 0 ms)
    if (analogContinuousRead(&result, 0)) {
      
      // Optionally pause sampling while processing or printing heavy tasks
      analogContinuousStop();

      Serial.println("----------------------------------------");
      for (int i = 0; i < adc_pins_count; i++) {
        Serial.printf("ADC Channel %d : GPIO %2d -> Raw Average: %4d | Calibrated Voltage: %4d mV\n", 
                      result[i].channel,
                      result[i].pin, 
                      result[i].avg_read_raw, 
                      result[i].avg_read_mvolts);
      }

      delay(1000); // Pause briefly for Serial readable output

      // Resume continuous sampling
      analogContinuousStart();
    }
  }

  // CPU is free to execute other non-blocking tasks here
}