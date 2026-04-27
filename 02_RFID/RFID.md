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
Add 10uF across 3.3V and GND for clean signal <br>

# Classic
Import library: MFRC522, 1.4.12 by Github Community
sample code: RFID

# Version 2
Import library: RFID_MFRC522v2. 2.0.6 by Github Community
sample code: RFID_v2

# Writing to the card
Writing to a MIFARE Classic card is more complex than reading because you must authenticate with the correct key (usually 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF for new cards) before the card will allow a write operation.<br>
Why the Safety Measures are Critical<br>
1. Sector 0, Block 0 (The Manufacturer Block)<br>
On standard MIFARE Classic 1K cards, this block is Read-Only. It contains the UID and the manufacturer's ID. Attempting to write here will usually return an error from the card, but it's best practice to block it in code.<br>
2. Block 3 of every Sector (The Sector Trailer)<br>
This is the most dangerous block to write to. It doesn't store data; it stores:<br>
Key A (6 bytes)<br>
Access Bits (4 bytes)<br>
Key B (6 bytes)<br>
If you write "Hello World" to Block 3, you overwrite the keys with random text. Since you won't know the "new key," you will be permanently locked out of that sector.<br>
Technical Tips for your S3 Setup<br>
Buffer Size: The MIFARE_Write function requires exactly 16 bytes. If your string is shorter, the compiler might fill the rest with zeros, but it's safer to define a byte buffer[16] manually.<br>
Authentication: Remember that you must authenticate per sector. If you want to write to Sector 1 and then Sector 2, you have to call PCD_Authenticate again for the new sector.<br>
Timeout: If you still get the "Timeout" error during the authentication step, try lowering the SPI frequency to 1000000 (1MHz).<br>


# Reference:
https://www.oceanlabz.in/esp32-s3-rfid-with-arduino-ide/ <br>
https://randomnerdtutorials.com/esp32-mfrc522-rfid-reader-arduino/#rfid-reader-writer-intro
