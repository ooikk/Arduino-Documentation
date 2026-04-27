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

# Card information
<img width="806" height="384" alt="image" src="https://github.com/user-attachments/assets/0b785230-987c-4df8-a9fa-d2a86216a89e" />
:
<img width="758" height="283" alt="image" src="https://github.com/user-attachments/assets/10ecf676-9f54-4fcf-b768-fd5af6d7aad5" />

Based on the dump provided above, we can look at the Sector Trailers to identify the keys.<br>
In a MIFARE Classic 1K card, the last block of every sector (Block 3, 7, 11,...55, 59, 63) is the Sector Trailer. It is formatted as:<br>
[Key A (6 bytes)] [Access Bits (4 bytes)] [Key B (6 bytes)]

1. Breaking down the Sector Trailer (Block 3, 7, or 11)<br>
Looking at the dump data for Block 3, 7, and 11, the data is:<br>
00 00 00 00 00 00 FF 07 80 69 FF FF FF FF FF FF<br>

2. Identifying the Keys<br>
From that data string, we can extract the following:<br>
Key A: 00 00 00 00 00 00<br>
Note: In many dumps, if Key A is private, the reader will show it as zeros or hide it. However, if this is a custom card you've worked with, it’s possible the key was literally set to all zeros.<br>
Key B: FF FF FF FF FF FF<br>
This is the "Factory Default" key. It is fully visible at the end of the block.<br>
Access Bits: FF 07 80 69<br>
These bits tell the card that Key A is used for reading and Key B is used for writing/changing the keys.<br>

3. Which is the "Authentication Key"?<br>
The "Authentication Key" isn't a third key; it simply refers to whichever key (A or B) you choose to use to prove to the card that you have permission to access that sector.<br>
To Read: You should try authenticating with Key A (00 00 00 00 00 00).<br>
To Write: You should try authenticating with Key B (FF FF FF FF FF FF).<br>

4. Important Observation: Block 0
In the dump, Block 0 (the Manufacturer Block) contains:<br>
2E 69 29 07 69 08 04 00 62 63 64 65 66 67 68 69<br>
The first 4 bytes (2E 69 29 07) are the card's UID. If you ever need to identify this specific card in your code, those are the numbers to look for.<br>

In MIFARE Classic cards, whether you use Key A or Key B depends on the Access Bits stored in the Sector Trailer. <br>
In the above card's dump, the Access Bits were FF 07 80 69. This is the "factory default" configuration, and here is why it leads to using Key B for both:

1. The Role of Access BitsEvery sector has its own "rulebook" (the 4 bytes in the middle of the Sector Trailer). These bits define:<br>
- Which key can Read data.
- Which key can Write data.
- Which key can Change the keys themselves.
2. Your Card's ConfigurationWith your current settings (FF 07 80 69), the card follows these rules:
- **Key A**: Is marked as "Internal/Protected." It can often be used for reading, but the card is configured to hide it (which is why it showed up as 00 in your dump).
- **Key B**: Is marked as the "Master" for that sector. It has been given permission to both Read and Write to all data blocks in that sector.
3. Why use the same key?
  It simplifies the workflow. Instead of needing to manage two different keys (one for the "Reader" device and one for the "Writer" device), you can use Key B as a single password that grants full access.

4. Can they be different?Yes. In a professional system (like a canteen payment card), the configuration is usually different:
- **Key A (Read Only)**: Given to the "Balance Checker" machine so it can see how much money you have, but cannot change it.
- **Key B (Read/Write)**: Given only to the "Top-up" machine so it can add money to the card.

Summary of your Card
```
Task       Allowed Key     Why?
Read Data  Key A or Key B  Access bits allow both.
Write Data Key B Only      Access bits restrict writing to the "Higher" key (Key B).
````
# Reading from the card
**The 18-Byte Buffer**: Even though a block is only 16 bytes, the MIFARE_Read function requires an 18-byte buffer to accommodate the two extra bytes used for the Cyclic Redundancy Check (CRC)

**Absolute Block Calculation**: The card sees memory as a flat list of blocks (0 to 63). Since each sector has 4 blocks, the formula is always:
- (Sector * 4) + Block

**Authentication Command**: Used PICC_CMD_MF_AUTH_KEY_B because the card image showed Key B was active and accessible. If you ever use a card where Key B is "hidden," you would switch this back to PICC_CMD_MF_AUTH_KEY_A.

# Writing to the card
Writing to a MIFARE Classic card is more complex than reading because you must authenticate with the correct key (usually 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF for new cards) before the card will allow a write operation.<br>
Why the Safety Measures are Critical<br>
1. Sector 0, Block 0 (The Manufacturer Block)<br>
On standard MIFARE Classic 1K cards, this block is Read-Only. It contains the UID and the manufacturer's ID. Attempting to write here will usually return an error from the card, but it's best practice to block it in code.<br>
2. Block 3 of every Sector (The Sector Trailer)<br>
This is the most dangerous block to write to. It doesn't store data; it stores:<br>
Key A (6 bytes) | Access Bits (4 bytes) | Key B (6 bytes)<br>
If you write "Hello World" to Block 3, you overwrite the keys with random text. Since you won't know the "new key," you will be permanently locked out of that sector.<br>
Technical Tips for your S3 Setup<br>
Buffer Size: The MIFARE_Write function requires exactly 16 bytes. If your string is shorter, the compiler might fill the rest with zeros, but it's safer to define a byte buffer[16] manually.<br>
Authentication: Remember that you must authenticate per sector. If you want to write to Sector 1 and then Sector 2, you have to call PCD_Authenticate again for the new sector.<br>
Timeout: If you still get the "Timeout" error during the authentication step, try lowering the SPI frequency to 1000000 (1MHz).<br>


# Reference:
https://www.oceanlabz.in/esp32-s3-rfid-with-arduino-ide/ <br>
https://randomnerdtutorials.com/esp32-mfrc522-rfid-reader-arduino/#rfid-reader-writer-intro
