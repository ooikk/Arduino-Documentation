## I2C Communication Protocol

The ESP32 has two I2C bus interfaces that can serve as I2C master or slave.

I²C means **I**nter **I**ntegrated **C**ircuit (it’s pronounced I-squared-C), and it is a synchronous, multi-master, multi-slave communication protocol. You can connect :
- multiple slaves to one master: for example, your ESP32 reads from a BME280 sensor using I2C and writes the sensor readings in an I2C OLED display.
- multiple masters controlling the same slave: for example, two ESP32 boards writing data to the same I2C OLED display.

<img width="1008" height="595" alt="image" src="https://github.com/user-attachments/assets/36fe91e4-1227-4a22-9cd1-9e8760c9d596" />

The ESP32 supports I2C communication through its two I2C bus interfaces that can serve as I2C master or slave, depending on the user’s configuration. Accordingly to the ESP32 datasheet, the I2C interfaces of the ESP32 supports:
- Standard mode (100 Kbit/s) 
- Fast mode (400 Kbit/s) 
- Up to 5 MHz, yet constrained by SDA pull-up strength 
- 7-bit/10-bit addressing mode 
- Dual addressing mode. Users can program command registers to control I²C interfaces, so that they have more flexibility



## Reference:

https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/
