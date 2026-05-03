# I2C LCD1602A

The advantage of using an I2C LCD is that the wiring is really simple. You just need to wire the SDA and SCL pins.

Additionally, it comes with a built-in potentiometer you can use to adjust the contrast between the background and the characters on the LCD. 


<img width="702" height="710" alt="image" src="https://github.com/user-attachments/assets/b0b46067-8f01-401b-a87a-c077a97138b9" />
<br>
Adjust the contrast:

<img width="842" height="331" alt="image" src="https://github.com/user-attachments/assets/32d7bda1-d868-4a08-b158-d78e17443d39" />


## Connect to LCD

```
I2C LCD          ESP32 S3
GND              GND
VCC              VIN (2.5V to 6V)
SDA              GPIO 8
SCL              GPIO 9
```

## Character Codes and Character Patterns (ROM Code: A00)


<img width="705" height="873" alt="image" src="https://github.com/user-attachments/assets/de78a54b-a890-4319-9ca3-36a3fb443f6e" />
<br>
<img width="698" height="861" alt="image" src="https://github.com/user-attachments/assets/904ddb6c-3d7e-4ad1-a57b-35879ecd3d60" />


## Reference

https://docs.arduino.cc/learn/electronics/lcd-displays/

https://randomnerdtutorials.com/esp32-esp8266-i2c-lcd-arduino-ide/

https://www.luisllamas.es/en/arduino-lcd-i2c/
