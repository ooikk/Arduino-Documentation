# 0.96" I2C OLED SDD1306 

OLEDs are one of the types of screens available for use in our electronics and Arduino projects.

An OLED (Organic light-emitting diode) is a type of LED in which the emissive layer is made of an organic compound that emits light in response to electricity.

Like other types of displays, OLEDs need a specific controller that converts the received data into electronic signals to control the display.

In the field of home electronics, small 0.96” OLED displays are commercially available, ready to connect to Arduino . These displays incorporate the SDD1306 controller and have a very small size of 25mm x 14mm. They are monochrome and have a resolution of 128x64 pixels.

OLED screens have the advantage of very low power consumption , around 20mA, since only the necessary pixels are illuminated and they don't require a backlight. This is especially useful in battery-powered applications.

Furthermore, they offer better visibility in bright environments , such as sunlight. However, the small size of these 0.96” OLED screens can pose a challenge for proper viewing.

Depending on the model, communication can be via SPI or I2C bus , making it easy to obtain measured data. The supply voltage supports 3.3V and 5V.

These types of OLED screens are interesting for projects where we can take advantage of their small size. For other applications, we'll generally prefer an LCD display like the Nokia 5110 or a TFT screen.    

<img width="389" height="380" alt="image" src="https://github.com/user-attachments/assets/e0b72553-91ba-464b-aea8-919d0d8056f3" />


## Connect to OLED     

```
0.96 OLED        ESP32 S3
GND              GND
VCC              3.3V
SDA              GPIO 8    // default I2C
SCL              GPIO 9    // default I2C
```

## Wiring Diagram     

<img width="580" height="587" alt="image" src="https://github.com/user-attachments/assets/9dc00342-cde8-47ed-a046-8a25d050572c" />

## Library for OLED     

Insatll libraries by Adafruit:
1. Adafruit SSD1306     
  https://github.com/adafruit/Adafruit_SSD1306

2. Adafruit GFX Library
  https://github.com/adafruit/Adafruit-GFX-Library

3. Adafruit BusIO
  https://github.com/adafruit/Adafruit_BusIO




## Reference

https://www.oceanlabz.in/project-3-oled-display-with-esp32-displaying-sensor-data/

https://www.luisllamas.es/conectar-arduino-a-una-pantalla-oled-de-0-96/

