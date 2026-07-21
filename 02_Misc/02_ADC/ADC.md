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

*Note:* Standard analogRead() functions are designed for single, on-demand readings and are relatively slow (typically ~1–5 kHz sampling rate).     


**Continuous Sampling / DMA API (High Speed)**     
For high-speed background sampling, the ESP32 Arduino core offers a continuous DMA-backed API:       
- ```bool analogContinuous(uint8_t pins[], size_t pins_count, uint32_t conversions_per_pin, uint32_t sampling_freq_hz, void (*userFunc)(void))```       
  Configures multi-pin continuous DMA sampling.     
  - pins[]: Array of GPIO pins to measure.
  - pins_count: Number of pins in the array.
  - conversions_per_pin: Samples taken per pin per conversion cycle.
  - sampling_freq_hz: Sampling rate in Hertz (e.g., 20000 for 20kHz, range from 1000Hz up to 83kHz).
  - userFunc: Callback function triggered when a batch buffer fills.
- ```bool analogContinuousStart()```        
  Starts background DMA conversion.       
- ```bool analogContinuousRead(adc_continuous_data_t **buffer, uint32_t timeout_ms)```       
  Populates the target buffer pointer with the continuous sample results.       
- ```bool analogContinuousStop()```        
  Stops continuous sampling.        

*Important Limitations for ESP32-S3:*
- ADC1 Only: Continuous/DMA mode is hardwired only to ADC1 (GPIO 1 to GPIO 10). You cannot use ADC2 pins (GPIO 11-20) for continuous sampling.
- Resolution: In continuous mode, the supported resolutions are typically 9-bit, 10-bit, 11-bit, or 12-bit. (13-bit is not supported in the standard continuous wrapper).
- To use Continuous Sampling (DMA) on the ESP32-S3, you must use the Arduino-ESP32 Core version 3.0.0 or newer. Compilation Errors: If you see errors like "analogContinuous was not declared", you are using an outdated version of the ESP32 Board Package. Go to Tools > Board > Boards Manager, search for "esp32", and update to version 3.0.0 or higher.     


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
Convert ADC to actual unit such as resistance:      
```
 // Read the raw analog value (0 - 4095 for 12-bit)
  int rawValue = analogRead(adcPin);
  // map(value, fromLow, fromHigh, toLow, toHigh)
  long resistance = map(rawValue, 0, 4095, 0, 10000);  //map to resistance
  long resistance_Scale = rawValue*10000/4095;
 // Print results to Serial Monitor
  Serial.printf("Resistance %d ohm\n", resistance);
  Serial.printf("Resistance (scaled) %d ohm\n", resistance_Scale);  
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
**Continuous ADC / DMA API**      
In continuous mode, the ADC hardware runs autonomously in the background via DMA. An Interrupt Service Routine (ISR) callback notifies your program when a set number of samples is ready, allowing the CPU to perform other tasks in loop() without blocking.      
```
#include <Arduino.h>

// Define ADC1 pins for ESP32-S3 (Continuous mode ONLY supports ADC1 pins: GPIO 1 to 10)
uint8_t adc_pins[] = {1, 2, 3, 4}; 
uint8_t adc_pins_count = sizeof(adc_pins) / sizeof(uint8_t);

// Number of raw conversions per pin taken per cycle (averaged automatically by the driver)
#define CONVERSIONS_PER_PIN 10 

// Target sampling frequency in Hz
#define SAMPLING_FREQ_HZ    20000 

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
        Serial.printf("GPIO %2d -> Raw Average: %4d | Calibrated Voltage: %4d mV\n", 
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
```
**How It Works**    
1. Hardware Selection: Continuous mode only supports ADC1 pins (GPIO 1 through GPIO 10 on the ESP32-S3). ADC2 pins cannot be used for continuous background sampling.
2. ```ARDUINO_ISR_ATTR``` Callback: The ```adcCallback()``` function runs inside an Interrupt Service Routine context when the hardware DMA engine finishes collecting a chunk of samples. It sets ```conversion_done = true``` so main execution logic knows new data is ready.
3. ```adc_continuous_result_t``` Structure: The ```analogContinuousRead()``` function populates an array of structures containing:
   - ```result[i].pin```: The GPIO pin number.
   - ```result[i].channel```: Internal ADC channel number.
   - ```result[i].avg_read_raw```: The hardware-averaged raw 12-bit value.
   - ```result[i].avg_read_mvolts```: The hardware-averaged value automatically converted into factory-calibrated millivolts.

## Why #include <Arduino.h>?      
Including ```#include <Arduino.h>``` is a fundamental best practice in C/C++ development for microcontrollers, though its necessity depends slightly on how and where you are writing your code.      
Here is exactly why it is there and what it does:     
1. It is the "Master" Header File     
Arduino.h is the central header file for the Arduino core. It contains the declarations for all standard Arduino functions (Serial, delay, digitalWrite, analogRead), standard data types (uint8_t, uint16_t), and essential macros. Without it, the compiler wouldn't know what Serial.println or uint8_t means.
2. The Arduino IDE "Magic" (Why you might think it's optional)     
If you are writing code in a standard .ino file in the Arduino IDE, the IDE actually automatically prepends #include <Arduino.h> to your code behind the scenes before sending it to the compiler. Because of this "magic," many beginners never see it and assume it isn't needed.
3. Why it was explicitly included in the example     
Even though the Arduino IDE adds it automatically to .ino files, explicitly including it is highly recommended (and sometimes strictly required) for several reasons:
- PlatformIO / Advanced IDEs: If you use PlatformIO, VS Code, or Eclipse, your code is often compiled as standard .cpp (C++) files, not .ino files. The IDE "magic" does not happen here. If #include <Arduino.h> is missing, the code will immediately fail to compile with errors like "Serial was not declared in this scope".
- Multi-file Projects: If you split your code into separate .h and .cpp files (e.g., sensor.cpp), you must include Arduino.h in those files to access Arduino functions.
- ESP32-Specific Macros: The example code uses ARDUINO_ISR_ATTR. This is an ESP32-specific macro that tells the compiler to place the Interrupt Service Routine in fast IRAM memory. This macro is defined within the ESP32 Arduino core headers, which are loaded alongside or via Arduino.h.
- Code Portability: Explicitly stating your dependencies makes the code robust. If you copy-paste the code into a different environment or rename the file extension, it will still compile without modification.      

**Summary**     
If you copy-paste that code into a blank .ino file in the standard Arduino IDE, you could technically delete #include <Arduino.h> and it would still compile. However, leaving it in is a professional best practice that guarantees the code will work everywhere, prevents weird compilation errors in advanced setups, and makes the code's dependencies perfectly clear.


## References     

https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-adc.h

https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html     

https://www.oceanlabz.in/esp32-analog-input/

https://www.luisllamas.es/esp32-adc/

https://randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/

https://deepbluembedded.com/esp32-adc-tutorial-read-analog-voltage-arduino/    


