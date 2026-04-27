# MFRC522 Setup

<img width="727" height="373" alt="image" src="https://github.com/user-attachments/assets/faad8f38-8abe-43f3-98f1-e2c2a0261664" />

# Pins assignment
```
MFRC522 RFID Reader	ESP32-S3 Pin            Description
SDA	                GPIO 10/ FSPICS0        SPI signal input, I2C data line, or UART data input
SCK	                GPIO 12/ FSPICLK        SPI clock
MOSI                GPIO 11/ FSPID          SPI data input
MISO                GPIO 13/ FSPIQ          SPI master-in-slave-out, I2C serial clock, or UART serial output
IRQ                 Don’t connect           Interrupt pin; signals the microcontroller when an RFID tag is nearby
GND	                GND	
RST	                GPIO 5                  LOW signal to put the module in power-down mode; send a HIGH signal to reset the module
3.3V                3.3V                    Power supply (2.5-3.3V)
```
Add 10uF across 3.3V and GND for clean signal

# Reference:
https://www.oceanlabz.in/esp32-s3-rfid-with-arduino-ide/
