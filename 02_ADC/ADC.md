# ESP32 S3 ADC     
The ESP32-S3 features two independent SAR (Successive Approximation Register) ADC blocks: ADC1 and ADC2. Both support up to 12-bit resolution (configurable up to 13-bit in ESP-IDF, but Arduino defaults to 12-bit).     

## ESP32-S3 ADC Pinout     
|	ADC Unit	|	Channels	|	Mapped GPIO Pins	|	Notes	|
|	-	|	-	|	-	|	-	|
|	ADC1	|	10 Channels (CH0 - CH9)	|	GPIO1 to GPIO10	|	Can be used freely, even when Wi-Fi/Bluetooth is active.	|
|	ADC2	|	10 Channels (CH0 - CH9)	|	GPIO11 to GPIO20	|	Shared with Wi-Fi. Cannot be used if Wi-Fi is enabled.	|

*Note:* The ESP32-S3 does not have a dedicated internal Hall Sensor or internal DAC like the original ESP32.     

## Conceptual Block Diagram     



## References     

https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html     

https://www.oceanlabz.in/esp32-analog-input/

https://www.luisllamas.es/esp32-adc/

https://randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/

https://deepbluembedded.com/esp32-adc-tutorial-read-analog-voltage-arduino/    


