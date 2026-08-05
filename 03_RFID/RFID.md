# MFRC522 Setup

<img width="727" height="373" alt="image" src="https://github.com/user-attachments/assets/faad8f38-8abe-43f3-98f1-e2c2a0261664" />

## Pins assignment

```
MFRC522 RFID Reader	ESP32-S3 Pin            Description
SDA	                GPIO 10/ FSPICS0        SPI CS
SCK	                GPIO 12/ FSPICLK        SPI clock
MOSI                GPIO 11/ FSPID          SPI Data Out: master-out-slave-in
MISO                GPIO 13/ FSPIQ          SPI Data In: master-in-slave-out
IRQ                 Don’t connect           Interrupt pin; signals the microcontroller when an RFID tag is nearby
GND	                GND	
RST	                GPIO 5                  LOW signal to put the module in power-down mode; send a HIGH signal to reset the module
3.3V                3.3V                    Power supply (2.5-3.3V)
```

Add 10uF across 3.3V and GND for clean signal.

## Classic

### **Library**

Import library: **MFRC522**, version **1.4.12** by Github Community.

Sample code: **RFID**

https://github.com/miguelbalboa/rfid

## Version 2

### **Library**

Import library: **RFID_MFRC522v2**, version **2.0.6** by Github Community.

Sample code: **RFID_v2**

https://github.com/OSSLibraries/Arduino_MFRC522v2

# Card information

<img width="806" height="384" alt="image" src="https://github.com/user-attachments/assets/0b785230-987c-4df8-a9fa-d2a86216a89e" />

<img width="758" height="283" alt="image" src="https://github.com/user-attachments/assets/10ecf676-9f54-4fcf-b768-fd5af6d7aad5" />

Based on the dump provided above, we can look at the Sector Trailers to identify the keys.

In a MIFARE Classic 1K card, the last block of every sector (Block 3, 7, 11, ..., 55, 59, 63) is the Sector Trailer. It is formatted as:

`[Key A (6 bytes)] [Access Bits (4 bytes)] [Key B (6 bytes)]`

### **Breaking Down the Sector Trailer**

Looking at the dump data for Block 3, 7, and 11, the data is:

`00 00 00 00 00 00 FF 07 80 69 FF FF FF FF FF FF`

### **Identifying the Keys**

From the data string, we can extract the following:

- **Key A:** `00 00 00 00 00 00`

  **Note:** In many dumps, if Key A is private, the reader will show it as zeros or hide it. However, if this is a custom card you've worked with, it's possible the key was literally set to all zeros.

- **Key B:** `FF FF FF FF FF FF`

  This is the factory default key. It is fully visible at the end of the block.

- **Access Bits:** `FF 07 80 69`

  These bits tell the card that Key A is used for reading and Key B is used for writing/changing the keys.

### **Which is the Authentication Key?**

The authentication key isn't a third key. It simply refers to whichever key (A or B) you choose to use to prove to the card that you have permission to access that sector.

- **To Read:** Authenticate with **Key A** (`00 00 00 00 00 00`).
- **To Write:** Authenticate with **Key B** (`FF FF FF FF FF FF`).

### **Important Observation: Block 0**

In the dump, Block 0 (the Manufacturer Block) contains:

`2E 69 29 07 69 08 04 00 62 63 64 65 66 67 68 69`

The first four bytes (`2E 69 29 07`) are the card's UID. If you ever need to identify this specific card in your code, those are the numbers to look for.

### **Understanding the Access Bits**

In MIFARE Classic cards, whether you use Key A or Key B depends on the Access Bits stored in the Sector Trailer.

In the above card dump, the Access Bits are `FF 07 80 69`. This is the factory default configuration.

#### **1. Role of the Access Bits**

Every sector has its own rulebook (the 4 bytes in the middle of the Sector Trailer). These bits define:

- Which key can read data.
- Which key can write data.
- Which key can change the keys themselves.

#### **2. Card Configuration**

With the current settings (`FF 07 80 69`), the card follows these rules:

- **Key A:** Marked as *Internal/Protected*. It can often be used for reading, but the card is configured to hide it (which is why it appeared as `00` in the dump).
- **Key B:** Acts as the master key for that sector. It has permission to both read and write all data blocks within the sector.

#### **3. Why Use the Same Key?**

Using Key B for both reading and writing simplifies the workflow. Instead of managing two different keys (one for a reader and another for a writer), a single key grants full access.

#### **4. Can They Be Different?**

Yes. In professional systems (such as canteen payment cards), the configuration is usually different:

- **Key A (Read Only):** Used by balance-checking machines.
- **Key B (Read/Write):** Used only by authorized top-up machines.

### **Summary of the Card**

```
Task         Allowed Key       Why?
Read Data    Key A or Key B    Access bits allow both.
Write Data   Key B Only        Access bits restrict writing to the "Higher" key (Key B).
```

# Reading from the card

### **18-Byte Buffer**

Even though a block is only 16 bytes, the `MIFARE_Read()` function requires an 18-byte buffer to accommodate the two extra bytes used for the Cyclic Redundancy Check (CRC).

### **Absolute Block Calculation**

The card sees memory as a flat list of blocks (0 to 63). Since each sector has 4 blocks, the formula is always:

- `(Sector × 4) + Block`

### **Authentication Command**

Use `PICC_CMD_MF_AUTH_KEY_B` because the card image showed that Key B was active and accessible.

If you ever use a card where Key B is hidden, switch back to `PICC_CMD_MF_AUTH_KEY_A`.

# Writing to the card

Writing to a MIFARE Classic card is more complex than reading because you must authenticate with the correct key (usually `FF FF FF FF FF FF` for new cards) before the card will allow a write operation.

### **Why the Safety Measures are Critical**

#### **1. Sector 0, Block 0 (Manufacturer Block)**

On standard MIFARE Classic 1K cards, this block is read-only. It contains the UID and manufacturer ID.

Attempting to write to this block usually returns an error, but it's still best practice to block this operation in code.

#### **2. Block 3 of Every Sector (Sector Trailer)**

This is the most dangerous block to write.

Instead of user data, it stores:

`Key A (6 bytes) | Access Bits (4 bytes) | Key B (6 bytes)`

If you write arbitrary data such as "Hello World" into Block 3, you overwrite the authentication keys. Since you won't know the new keys, you will permanently lose access to that sector.

### **Technical Tips for ESP32-S3 Setup**

#### **Buffer Size**

`MIFARE_Write()` requires exactly **16 bytes**.

If your string is shorter, the compiler may fill the remaining bytes with zeros, but it's safer to define a `byte buffer[16]` manually.

#### **Authentication**

Authentication is performed **per sector**.

If writing to multiple sectors (for example, Sector 1 followed by Sector 2), call `PCD_Authenticate()` again before writing to the next sector.

#### **Timeout**

If authentication still results in a timeout, try reducing the SPI frequency to **1 MHz (1000000 Hz)**.

# Reference

https://www.oceanlabz.in/esp32-s3-rfid-with-arduino-ide/

https://randomnerdtutorials.com/esp32-mfrc522-rfid-reader-arduino/#rfid-reader-writer-intro
