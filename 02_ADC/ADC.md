# ESP32 S3 ADC     
The ESP32-S3 features two independent SAR (Successive Approximation Register) ADC blocks: ADC1 and ADC2. Both support up to 12-bit resolution (configurable up to 13-bit in ESP-IDF, but Arduino defaults to 12-bit).     

## ESP32-S3 ADC Pinout     
|	ADC Unit	|	Channels	|	Mapped GPIO Pins	|	Notes	|
|	-	|	-	|	-	|	-	|
|	ADC1	|	10 Channels (CH0 - CH9)	|	GPIO1 to GPIO10	|	Can be used freely, even when Wi-Fi/Bluetooth is active.	|
|	ADC2	|	10 Channels (CH0 - CH9)	|	GPIO11 to GPIO20	|	Shared with Wi-Fi. Cannot be used if Wi-Fi is enabled.	|

*Note:* 
- The ESP32-S3 does not have a dedicated internal Hall Sensor or internal DAC like the original ESP32.
- ADC2 is shared with the Wi-Fi and Bluetooth subsystems. If Wi-Fi or Bluetooth is active, stick to ADC1 to prevent hardware conflicts or unexpected readings.    

## Conceptual Block Diagram     

<img width="1083" height="677" alt="image" src="https://github.com/user-attachments/assets/e2276f97-02b5-44e7-aa06-64f704933997" />

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
- analogReadResolution(bits): Sets the resolution. Default is 12 (0-4095). Can be set to 9, 10, 11, 12, or 13.
- analogSetAttenuation(attenuation): Sets the input voltage range for all ADC pins.
  - ADC_0db: ~0 - 750mV
  - ADC_2_5db: ~0 - 1050mV
  - ADC_6db: ~0 - 1300mV
  - ADC_11db: ~0 - 3100mV (Default, covers most 3.3V logic)
- analogSetPinAttenuation(pin, attenuation): Sets the attenuation for a specific pin.

**Reading Functions**
- analogRead(pin): Reads the analog value from the specified GPIO. Returns an integer based on the configured resolution.
- analogReadMilliVolts(pin): Reads the pin and returns the calculated voltage in millivolts (mV). Highly recommended for accurate voltage readings.

- uint16_t analogRead(uint8_t pin)
  Reads the raw analog value from the specified GPIO pin.
  Parameters:
  - pin – The GPIO pin number (e.g., 4 for GPIO4 / ADC1_CH3).
  - Returns: Raw integer scaled according to the current resolution (default 0 to 4095 for 12-bit).
- uint32_t analogReadMilliVolts(uint8_t pin)
  Reads the raw ADC value and automatically applies factory eFuse calibration to calculate real millivolts.
  Parameters:
  - pin – The GPIO pin number.
  - Returns: Calibrated voltage value in millivolts (mV).       



## References     

https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html     

https://www.oceanlabz.in/esp32-analog-input/

https://www.luisllamas.es/esp32-adc/

https://randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/

https://deepbluembedded.com/esp32-adc-tutorial-read-analog-voltage-arduino/    


