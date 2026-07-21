# ESP32 S3 ADC     
The ESP32-S3 features two independent SAR (Successive Approximation Register) ADC blocks: ADC1 and ADC2. Both support up to 12-bit resolution (configurable up to 13-bit in ESP-IDF, but Arduino defaults to 12-bit).     

## ESP32-S3 ADC Pinout     
|	ADC Unit	|	Channels	|	Mapped GPIO Pins	|	Notes	|
|	-	|	-	|	-	|	-	|
|	ADC1	|	10 Channels (CH0 - CH9)	|	GPIO1 to GPIO10	|	Can be used freely, even when Wi-Fi/Bluetooth is active.	|
|	ADC2	|	10 Channels (CH0 - CH9)	|	GPIO11 to GPIO20	|	Shared with Wi-Fi. Cannot be used if Wi-Fi is enabled.	|

*Note:*       
- ADC2 is shared with the Wi-Fi and Bluetooth subsystems. If Wi-Fi or Bluetooth is active, stick to ADC1 to prevent hardware conflicts or unexpected readings.    

## Conceptual Block Diagram     

<img alt="image" style="width: 50%; height: auto;"  src="https://github.com/user-attachments/assets/e2276f97-02b5-44e7-aa06-64f704933997" />

**Architecture Highlights**    
- **Analog Domain:** Receives inputs into SARADC1 and SARADC2 blocks.
- **RTC Domain:** Allows ADC operation during Deep Sleep for low-power monitoring.
- **Digital Domain:** Manages digital filtering, continuous sampling (DMA), and arbitration for ADC2 between RTC, Wi-Fi, and the digital controller.

## ADC Resolution     
The ESP32-S3 ADC supports a total of five configurable resolutions: 9-bit, 10-bit, 11-bit, 12-bit, and 13-bit. By default, the raw 12-bit output ranges from 0 to 4095.       
However, it is important to understand how these resolutions are achieved, as the physical hardware is natively only 12-bit.      
The Resolutions and Their Ranges:      

|	Resolution	|	Max Digital Value	|	How it is Achieved	|	Effect on Sampling Speed	|
|	-	|	-	|	-	|	-	|
|	9-bit	|	511	|	Truncates the 3 least significant bits (LSBs) of the 12-bit hardware result.	|	Fastest	|
|	10-bit	|	1,023	|	Truncates the 2 LSBs.	|	Very Fast	|
|	11-bit	|	2,047	|	Truncates the 1 LSB.	|	Fast	|
|	12-bit	|	4,095	|	Native Hardware Resolution. Uses the full 12-bit SAR ADC output.	|	Standard / Default	|
|	13-bit	|	8,191	|	Digital Filtering / Oversampling. Takes multiple 12-bit samples and accumulates/averages them to calculate a 13th bit.	|	Slowest	|

**Hardware vs. Software Resolution**     
- **Native Hardware (12-bit):** The physical Successive Approximation Register (SAR) ADC inside the ESP32-S3 is a true 12-bit converter.
- **Lower Resolutions (9, 10, 11-bit):** The chip does not actually convert at a lower bit-depth. It performs a full 12-bit conversion and then simply shifts the bits to the right (discarding the LSBs). This is done to increase the maximum sampling rate when high precision isn't needed.
- **Higher Resolution (13-bit):** The hardware cannot physically resolve 13 bits in a single conversion cycle. The 13-bit mode is achieved via the internal digital filter, which oversamples the 12-bit ADC and averages the results. This reduces noise and gives you an extra bit of resolution, but it significantly lowers the maximum sampling rate.       
**How to Set the Resolution in Arduino IDE**      
```
void setup() {
  Serial.begin(115200);
  
  // Set to 12-bit (Default, 0 - 4095)
  analogReadResolution(12); 
  
  // Set to 13-bit for higher precision (0 - 8191)
  // analogReadResolution(13); 
  
  // Set to 10-bit for faster reading (0 - 1023)
  // analogReadResolution(10); 
}
```
## Attenuation & Dynamic Range     
Setting the attenuation scales the input voltage range:      


```
Attenuation          Measurable input voltage range
ADC_0db              0 mV ~ 950 mV
ADC_2_5db            0 mV ~ 1250 mV
ADC_6db              0 mV ~ 1750 mV
ADC_11db             0 mV ~ 3100 mV
``` 
**How to Set the Attenuation in Arduino IDE**       
```
const int adcPin = 1; 
void setup() {
  Serial.begin(115200);
  
  // 1. Set to 12-bit (Default, 0 - 4095)
  analogReadResolution(12); 
  
  // 2. Set Attenuation (Default is 11dB, allows reading up to ~3.1V)
  // Options: ADC_0db, ADC_2_5db, ADC_6db, ADC_11db
  analogSetAttenuation(ADC_11db); 
  
  // Optional: Set attenuation for a specific pin only
  // analogSetPinAttenuation(adcPin, ADC_11db);
}
```

## Arduino IDE API Functions

**Configuration Functions**   
- ```void analogReadResolution(uint8_t bits)```       
  Sets the bit-width of the value returned by analogRead().
  - Parameters: bits – Resolution in bits (Range: 1 to 16, Default: 12).
  - Note: The ESP32-S3 hardware ADC operates natively at 12-bit resolution. Values outside 12-bit are bit-shifted up or down.
- ```void analogSetAttenuation(adc_attenuation_t attenuation)```       
  Sets the voltage attenuation globally across all ADC pins.     
  - Parameters:
    - ADC_0db: Full-scale voltage approx. 0 to 950 mV
    - ADC_2_5db: Full-scale voltage approx. 0 to 1250 mV
    - ADC_6db: Full-scale voltage approx. 0 to 1750 mV
    - ADC_11db (or ADC_12db depending on core version): Full-scale voltage approx. 0 to 3100 mV
- ```void analogSetPinAttenuation(uint8_t pin, adc_attenuation_t attenuation)```        
  Sets the attenuation for a specific pin rather than applying it globally.
  - Parameters: pin – GPIO pin number, attenuation – One of the four attenuation options listed above.  


**Reading Functions**
- ```uint16_t analogRead(uint8_t pin)```       
  Reads the raw analog value from the specified GPIO pin.       
  - Parameters: pin – The GPIO pin number (e.g., 4 for GPIO4 / ADC1_CH3).
  - Returns: Raw integer scaled according to the current resolution (default 0 to 4095 for 12-bit).
- ```uint32_t analogReadMilliVolts(uint8_t pin)```       
  Reads the raw ADC value and automatically applies factory eFuse calibration to calculate real millivolts.      
  - Parameters: pin – The GPIO pin number.
  - Returns: Calibrated voltage value in millivolts (mV).       

**Continuous Sampling / DMA API (High Speed)**     
For high-speed background sampling, the ESP32 Arduino core offers a continuous DMA-backed API:       
- ```bool analogContinuous(uint8_t pins[], size_t pins_count, uint32_t conversions_per_pin, uint32_t sampling_freq_hz, void (*userFunc)(void))```       
  Configures multi-pin continuous DMA sampling.     
  - pins[]: Array of GPIO pins to measure.
  - pins_count: Number of pins in the array.
  - conversions_per_pin: Samples taken per pin per conversion cycle.
  - sampling_freq_hz: Sampling rate in Hertz (e.g., 20000 for 20kHz, up to 80kHz).
  - userFunc: Callback function triggered when a batch buffer fills.
- ```bool analogContinuousStart()```        
  Starts background DMA conversion.       
- ```bool analogContinuousRead(adc_continuous_data_t **buffer, uint32_t timeout_ms)```       
  Populates the target buffer pointer with the continuous sample results.       
- ```bool analogContinuousStop()```        
  Stops continuous sampling.        

## Code Implementation & Setup Guide     
- Pin Selection: Always use ADC1 pins (GPIO 1-10) if you are using Wi-Fi or Bluetooth. If you must use ADC2 (GPIO 11-20), ensure Wi-Fi is completely disabled.     
  - ADC2 + Wi-Fi Conflict: If your code uses WiFi.begin() and you try to read from GPIO 11-20, analogRead() will return 0 or fail. Always use GPIO 1-10 for analog reads in wireless projects.
- Voltage Limits: The ESP32-S3 is a 3.3V device. **Never apply more than 3.3V to an ADC pin**, or you will destroy the microcontroller. With the default 11dB attenuation, the maximum readable voltage is roughly 3.1V.
- Noise: The ESP32 ADC can be noisy. If you need high precision, take multiple readings in code and average them, or use the analogReadMilliVolts() function which applies internal calibration.
- Non-linearity: The ESP32-S3 ADC is slightly non-linear at the very bottom (near 0V) and very top (near 3.1V) of its range. If your application requires extreme precision, use software calibration or map the usable range (e.g., 150 to 3900) to your expected physical values.

## Example code

**Reading a Potentiometer**
```
// Define the ADC pin (Using GPIO 1, which is ADC1_CH0)
const int adcPin = 1; 

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32-S3 ADC Example");

  // 1. Set ADC Resolution (Default is 12-bit, max value 4095)
  analogReadResolution(12); 
  
  // 2. Set Attenuation (Default is 11dB, allows reading up to ~3.1V)
  // Options: ADC_0db, ADC_2_5db, ADC_6db, ADC_11db
  analogSetAttenuation(ADC_11db); 
  
  // Optional: Set attenuation for a specific pin only
  // analogSetPinAttenuation(adcPin, ADC_11db);
}

void loop() {
  // Read the raw analog value (0 - 4095 for 12-bit)
  int rawValue = analogRead(adcPin);
  
  // Read the calibrated voltage in millivolts
  int voltageMV = analogReadMilliVolts(adcPin);
  
  // Calculate voltage in Volts (float)
  float voltageV = voltageMV / 1000.0;

  // Print results to Serial Monitor
  Serial.print("Raw ADC Value: ");
  Serial.print(rawValue);
  Serial.print("\t | Voltage: ");
  Serial.print(voltageMV);
  Serial.print(" mV (");
  Serial.print(voltageV, 3);
  Serial.println(" V)");

  // Wait before next reading
  delay(500); 
}
```

**Low-Noise Averaging Example**
```
const int adcPin = 4;
const int numSamples = 16; // Number of samples for moving average

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
}

uint32_t getSmoothedVoltageMV(int pin) {
  uint32_t totalMV = 0;
  for (int i = 0; i < numSamples; i++) {
    totalMV += analogReadMilliVolts(pin);
    delayMicroseconds(100); // Brief delay between samples
  }
  return totalMV / numSamples;
}

void loop() {
  uint32_t avgVoltageMV = getSmoothedVoltageMV(adcPin);
  
  Serial.print("Smoothed Voltage: ");
  Serial.print(avgVoltageMV / 1000.0, 3);
  Serial.println(" V");

  delay(500);
}
```


## References     

https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html     

https://www.oceanlabz.in/esp32-analog-input/

https://www.luisllamas.es/esp32-adc/

https://randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/

https://deepbluembedded.com/esp32-adc-tutorial-read-analog-voltage-arduino/    


